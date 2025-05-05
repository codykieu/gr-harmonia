/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "usrp_radar_all_impl.h"
#include <gnuradio/io_signature.h>

namespace gr
{
  namespace harmonia
  {

    usrp_radar_all::sptr usrp_radar_all::make(const std::string &args_1,
                                              const std::string &args_2,
                                              const double sdr1_rate,
                                              const double sdr2_rate,
                                              const double sdr1_freq,
                                              const double sdr2_freq,
                                              const double sdr1_gain,
                                              const double sdr2_gain,
                                              const double start_delay,
                                              const double cap_length,
                                              const double TDMA_time_slot,
                                              const bool verbose)
    {
      return gnuradio::make_block_sptr<usrp_radar_all_impl>(args_1,
                                                            args_2,
                                                            sdr1_rate,
                                                            sdr2_rate,
                                                            sdr1_freq,
                                                            sdr2_freq,
                                                            sdr1_gain,
                                                            sdr2_gain,
                                                            start_delay,
                                                            cap_length,
                                                            TDMA_time_slot,
                                                            verbose);
    }

    /*
     * The private constructor
     */
    usrp_radar_all_impl::usrp_radar_all_impl(const std::string &args_1,
                                             const std::string &args_2,
                                             const double sdr1_rate,
                                             const double sdr2_rate,
                                             const double sdr1_freq,
                                             const double sdr2_freq,
                                             const double sdr1_gain,
                                             const double sdr2_gain,
                                             const double start_delay,
                                             const double cap_length,
                                             const double TDMA_time_slot,
                                             const bool verbose)
        : gr::block("usrp_radar_all",
                    gr::io_signature::make(0, 0, 0),
                    gr::io_signature::make(0, 0, 0)),
          usrp_args_sdr1(args_1),
          usrp_args_sdr2(args_2),
          sdr1_rate(sdr1_rate),
          sdr2_rate(sdr2_rate),
          sdr1_freq(sdr1_freq),
          sdr2_freq(sdr2_freq),
          sdr1_gain(sdr1_gain),
          sdr2_gain(sdr2_gain),
          start_delay(start_delay),
          cap_length(cap_length),
          TDMA_time_slot(TDMA_time_slot),
          verbose(verbose)
    {
      // Additional parameters. I have the hooks in to make them configurable, but we don't
      // need them right now.
      this->sdr1_subdev = "";
      this->sdr2_subdev = "";
      this->sdr1_cpu_format = this->sdr2_cpu_format = "fc32";
      this->sdr1_otw_format = this->sdr2_otw_format = "sc16";
      this->sdr1_device_addr = "";
      this->sdr2_device_addr = "";
      this->sdr1_channel_nums = std::vector<size_t>(1, 0);
      this->sdr2_channel_nums = std::vector<size_t>(1, 0);
      this->tx_buffs = std::vector<std::vector<gr_complex>>(sdr1_channel_nums.size());

      this->n_tx_total = 0;
      this->new_msg_received = false;
      this->meta = pmt::make_dict();

      config_usrp(this->usrp_1,
                  this->usrp_2,
                  this->usrp_args_sdr1,
                  this->usrp_args_sdr2,
                  this->sdr1_rate,
                  this->sdr2_rate,
                  this->sdr1_freq,
                  this->sdr2_freq,
                  this->sdr1_gain,
                  this->sdr2_gain,
                  this->sdr1_subdev,
                  this->sdr2_subdev,
                  this->verbose);

      n_delay = 0;

      message_port_register_in(PMT_HARMONIA_IN);
      message_port_register_out(PMT_HARMONIA_OUT);
      set_msg_handler(PMT_HARMONIA_IN, [this](pmt::pmt_t msg)
                      { this->handle_message(msg); });
    }

    /*
     * Our virtual destructor.
     */
    usrp_radar_all_impl::~usrp_radar_all_impl() {}

    bool usrp_radar_all_impl::start()
    {
      finished = false;
      main_thread = gr::thread::thread(&usrp_radar_all_impl::run, this);
      return block::start();
    }

    bool usrp_radar_all_impl::stop()
    {
      finished = true;
      return block::stop();
    }

    void usrp_radar_all_impl::handle_message(const pmt::pmt_t &msg)
    {
      if (pmt::is_pdu(msg))
      {
        meta = pmt::dict_update(meta, pmt::car(msg));
        tx_data = pmt::cdr(msg);
        tx_buff_size = pmt::length(tx_data);

        if (pmt::dict_has_key(meta, pmt::intern(prf_key)))
        {
          prf = pmt::to_double(
              pmt::dict_ref(meta, pmt::intern(prf_key), pmt::from_double(0.0)));

          if (prf == 0)
          {
            rx_buff_size = tx_buff_size;
          }
          else
          {
            rx_buff_size = round(sdr2_rate / prf);
          }
        }

        new_msg_received = true;
      }
    }

    void usrp_radar_all_impl::run()
    {
      while (not new_msg_received)
      { // Wait for Tx data
        if (finished)
        {
          return;
        }
        else
        {
          std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
      }

      /***********************************************************************
       * Receive thread
       **********************************************************************/
      // USRP 1
      uhd::stream_args_t rx1_stream_args(sdr1_cpu_format, sdr1_otw_format);
      rx1_stream_args.channels = sdr1_channel_nums;
      rx1_stream_args.args = uhd::device_addr_t(sdr1_device_addr);
      uhd::rx_streamer::sptr rx1_stream = usrp_1->get_rx_stream(rx1_stream_args);

      // USRP 2
      uhd::stream_args_t rx2_stream_args(sdr2_cpu_format, sdr2_otw_format);
      rx2_stream_args.channels = sdr2_channel_nums;
      rx2_stream_args.args = uhd::device_addr_t(sdr2_device_addr);
      uhd::rx_streamer::sptr rx2_stream = usrp_2->get_rx_stream(rx2_stream_args);

      /***********************************************************************
       * Transmit thread
       **********************************************************************/
      // USRP 1
      uhd::stream_args_t tx1_stream_args(sdr1_cpu_format, sdr1_otw_format);
      tx1_stream_args.channels = sdr1_channel_nums;
      tx1_stream_args.args = uhd::device_addr_t(sdr1_device_addr);
      uhd::tx_streamer::sptr tx1_stream = usrp_1->get_tx_stream(tx1_stream_args);

      // USRP 2
      uhd::stream_args_t tx2_stream_args(sdr2_cpu_format, sdr2_otw_format);
      tx2_stream_args.channels = sdr2_channel_nums;
      tx2_stream_args.args = uhd::device_addr_t(sdr2_device_addr);
      uhd::tx_streamer::sptr tx2_stream = usrp_2->get_tx_stream(tx2_stream_args);

      /***********************************************************************
       * Thread Implementations
       **********************************************************************/
      double start_time_sdr1_tx = usrp_1->get_time_now().get_real_secs() + start_delay + TDMA_time_slot;
      std::cout << "USRP1 Time: " << start_time_sdr1_tx << std::endl;
      double start_time_sdr2_rx = usrp_2->get_time_now().get_real_secs();
      std::cout << "USRP2 Time: " << start_time_sdr2_rx << std::endl;

      double start_time_sdr1_rx = start_time_sdr1_tx + TDMA_time_slot;
      // std::cout << "USRP1 Time 2: " << start_time_sdr1_rx << std::endl;
      double start_time_sdr2_tx = start_time_sdr2_rx + TDMA_time_slot * 2.0;
      // std::cout << "USRP2 Time 2: " << start_time_sdr2_tx << std::endl;

      // Transmit and Receive Threads
      tx1_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_bursts, this, usrp_1, tx1_stream, start_time_sdr1_tx);

      rx2_thread =
          gr::thread::thread(&usrp_radar_all_impl::receive, this, usrp_2, rx2_stream, start_time_sdr2_rx);

      tx1_thread.join();
      rx2_thread.join();

      // // Transmit bursts
      // tx2_thread = gr::thread::thread(
      //     &usrp_radar_all_impl::transmit_bursts, this, usrp_2, tx2_stream, start_time_sdr2_tx);

      // rx1_thread =
      //     gr::thread::thread(&usrp_radar_all_impl::receive, this, usrp_1, rx1_stream, start_time_sdr1_rx);

      // tx2_thread.join();
      // rx1_thread.join();
    }

    void usrp_radar_all_impl::config_usrp(uhd::usrp::multi_usrp::sptr &usrp_1,
                                          uhd::usrp::multi_usrp::sptr &usrp_2,
                                          const std::string &args_1,
                                          const std::string &args_2,
                                          const double sdr1_rate,
                                          const double sdr2_rate,
                                          const double sdr1_freq,
                                          const double sdr2_freq,
                                          const double sdr1_gain,
                                          const double sdr2_gain,
                                          const std::string &sdr1_subdev,
                                          const std::string &sdr2_subdev,
                                          bool verbose)
    {
      usrp_1 = uhd::usrp::multi_usrp::make(args_1);
      if (not sdr1_subdev.empty())
      {
        usrp_1->set_tx_subdev_spec(sdr1_subdev);
      }
      if (not sdr2_subdev.empty())
      {
        usrp_1->set_rx_subdev_spec(sdr1_subdev);
      }
      usrp_1->set_tx_rate(sdr1_rate);
      usrp_1->set_rx_rate(sdr1_rate);
      usrp_1->set_tx_freq(sdr1_freq);
      usrp_1->set_rx_freq(sdr1_freq);
      usrp_1->set_tx_gain(sdr1_gain);
      usrp_1->set_rx_gain(sdr1_gain);

      usrp_2 = uhd::usrp::multi_usrp::make(args_2);
      if (not sdr1_subdev.empty())
      {
        usrp_2->set_tx_subdev_spec(sdr2_subdev);
      }
      if (not sdr2_subdev.empty())
      {
        usrp_2->set_rx_subdev_spec(sdr2_subdev);
      }
      usrp_2->set_tx_rate(sdr2_rate);
      usrp_2->set_rx_rate(sdr2_rate);
      usrp_2->set_tx_freq(sdr2_freq);
      usrp_2->set_rx_freq(sdr2_freq);
      usrp_2->set_tx_gain(sdr2_gain);
      usrp_2->set_rx_gain(sdr2_gain);

      // std::cout << "USRP1 Time: " << usrp_1->get_time_now().get_real_secs() << std::endl;
      // std::cout << "USRP2 Time: " << usrp_2->get_time_now().get_real_secs() << std::endl;

      // Sets USRPs to start at time = 0.0
      usrp_1->set_time_now(uhd::time_spec_t(0.0));
      usrp_2->set_time_now(uhd::time_spec_t(0.0));

      // std::cout << "USRP1 Time Post: " << usrp_1->get_time_now().get_real_secs() << std::endl;
      // std::cout << "USRP2 Time Post: " << usrp_2->get_time_now().get_real_secs() << std::endl;

      if (verbose)
      {
        std::cout << boost::format("Using Device 1: %s") % usrp_1->get_pp_string()
                  << std::endl;
        std::cout << boost::format("Using Device 2: %s") % usrp_2->get_pp_string()
                  << std::endl;
        std::cout << boost::format("Actual TX Rate: %f Msps") %
                         (usrp_1->get_tx_rate() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual RX Rate: %f Msps") %
                         (usrp_2->get_rx_rate() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual TX Freq: %f MHz") % (usrp_1->get_tx_freq() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual RX Freq: %f MHz") % (usrp_2->get_rx_freq() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual TX Gain: %f dB") % usrp_1->get_tx_gain()
                  << std::endl;
        std::cout << boost::format("Actual RX Gain: %f dB") % usrp_2->get_rx_gain()
                  << std::endl;
      }
    }

    void usrp_radar_all_impl::receive(uhd::usrp::multi_usrp::sptr usrp_rx,
                                      uhd::rx_streamer::sptr rx_stream,
                                      double start_time)
    {
      // Setup variables
      uhd::rx_metadata_t md;
      // Total samples to receive based on capture time
      size_t total_samps_to_rx = cap_length * sdr1_rate;
      size_t samps_received = 0;

      // Outputs RX time
      std::cout << "USRP RX start time: " << usrp_rx->get_time_now().get_real_secs() << std::endl;

      // Set up UHD receive mode
      uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
      cmd.num_samps = total_samps_to_rx;
      cmd.stream_now = usrp_rx->get_time_now().get_real_secs() >= start_time;
      cmd.time_spec = uhd::time_spec_t(start_time);
      rx_stream->issue_stream_cmd(cmd);

      // Allocate buffer
      pmt::pmt_t rx_data_pmt = pmt::make_c32vector(total_samps_to_rx, 0);
      gr_complex *rx_data_ptr = pmt::c32vector_writable_elements(rx_data_pmt, total_samps_to_rx);

      // Tells USRP to wait for incoming samples before giving up
      double timeout = 0.1;

      // Counts number of samples received until end of capture time
      while (samps_received < total_samps_to_rx)
      {
        size_t samps_to_recv = total_samps_to_rx - samps_received;
        size_t num_rx = rx_stream->recv(rx_data_ptr + samps_received, samps_to_recv, md, timeout);

        samps_received += num_rx;
      }

      // Tags metadata
      if (pmt::length(this->meta) > 0)
      {
        this->meta = pmt::dict_add(
            this->meta, pmt::intern(sdr2_freq_key), pmt::from_double(sdr2_freq));
        if (usrp_rx == usrp_1)
        {
          this->meta = pmt::dict_add(this->meta, pmt::intern("src"), PMT_HARMONIA_SDR1);
        }
        else if (usrp_rx == usrp_2)
        {
          this->meta = pmt::dict_add(this->meta, pmt::intern("src"), PMT_HARMONIA_SDR2);
        }
      }

      // Outputs metadata
      message_port_pub(PMT_HARMONIA_OUT, pmt::cons(this->meta, rx_data_pmt));
      this->meta = pmt::make_dict();
    }

    void usrp_radar_all_impl::transmit_bursts(uhd::usrp::multi_usrp::sptr usrp_tx,
                                              uhd::tx_streamer::sptr tx_stream,
                                              double start_time)
    {
      // Create the metadata, and populate the time spec at the latest possible moment
      uhd::tx_metadata_t md;

      // Outputs TX time
      std::cout << "USRP TX start time: " << usrp_tx->get_time_now().get_real_secs() << std::endl;

      if (new_msg_received)
      {
        // Assigns TX data to a vector
        const std::vector<gr_complex> tx_data_vector = pmt::c32vector_elements(tx_data);

        // Assigns TX data to buffer vector
        tx_buffs[0] = tx_data_vector;

        // Updates metadata with new information
        meta = pmt::dict_add(meta, pmt::intern(sdr1_freq_key), pmt::from_double(sdr1_freq));
        meta = pmt::dict_add(meta, pmt::intern(sample_start_key), pmt::from_long(n_tx_total));

        // Turns new message off
        new_msg_received = false;

        // ------- Testing -------
        // Save to-be-transmitted waveform to file
        // std::ofstream outfile("tx_data_vector.cfile", std::ios::binary);
        // outfile.write(reinterpret_cast<const char *>(tx_data_vector.data()),
        //               tx_data_vector.size() * sizeof(gr_complex));
        // outfile.close();
        // std::cout << "[usrp_radar] Saved " << tx_data_vector.size() << " samples to tx_data_vector.cfile" << std::endl;

        // Get maximum buffer size and TX data length
        // size_t max_samples = tx_stream->get_max_num_samps();
        // size_t tx_data_len = tx_data_vector.size();
        // std::cout << "[usrp_radar] Pulse size: " << tx_data_len << " samples" << std::endl;
        // std::cout << "Max samples per buffer (fragment size): " << max_samples << std::endl;
      }

      // Transmits waveform to UHD
      double timeout = 0.1;
      md.start_of_burst = true;
      md.end_of_burst = true;
      md.has_time_spec = true;
      md.time_spec = uhd::time_spec_t(start_time);
      tx_stream->send(tx_buffs[0].data(), tx_buffs[0].size(), md, timeout);
    }

    void usrp_radar_all_impl::set_metadata_keys(const std::string &sdr1_freq_key,
                                                const std::string &sdr2_freq_key,
                                                const std::string &sample_start_key,
                                                const std::string &prf_key)
    {
      this->sdr1_freq_key = sdr1_freq_key;
      this->sdr2_freq_key = sdr2_freq_key;
      this->sample_start_key = sample_start_key;
      this->prf_key = prf_key;
    }

  } /* namespace harmonia */
} /* namespace gr */
