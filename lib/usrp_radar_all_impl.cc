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
                                              const double wait_time,
                                              const double TDMA_time,
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
                                                            wait_time,
                                                            TDMA_time,
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
                                             const double wait_time,
                                             const double TDMA_time,
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
          wait_time(wait_time),
          TDMA_time(TDMA_time),
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

    void usrp_radar_all_impl::transmit_all(
        uhd::usrp::multi_usrp::sptr usrp,
        uhd::tx_streamer::sptr tx_stream,
        const std::vector<double> &times)
    {
      for (const auto &tx_time : times)
      {
        this->transmit_bursts(usrp, tx_stream, tx_time);
      }
    }

    void usrp_radar_all_impl::receive_all(
        uhd::usrp::multi_usrp::sptr usrp,
        uhd::rx_streamer::sptr rx_stream,
        const std::vector<double> &times,
        const std::vector<int> &rx_tags,
        const std::vector<bool> &final_flags)
    {
      for (size_t i = 0; i < times.size(); ++i)
      {
        this->receive(usrp, rx_stream, times[i], rx_tags[i], final_flags[i]);
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
      // std::cout << "USRP1 Time: " << usrp_1->get_time_now().get_real_secs() << std::endl;
      // std::cout << "USRP2 Time: " << usrp_2->get_time_now().get_real_secs() << std::endl;

      double sdr1_begin = usrp_1->get_time_now().get_real_secs() + start_delay;
      double sdr2_begin = usrp_2->get_time_now().get_real_secs() + start_delay;

      double sdr1_tx1 = sdr1_begin + TDMA_time;
      double sdr2_rx1 = sdr2_begin + TDMA_time - wait_time;

      double sdr1_rx1 = sdr1_begin + TDMA_time * 2 - wait_time;
      double sdr2_tx1 = sdr2_begin + TDMA_time * 2;

      double sdr1_tx2 = sdr1_begin + TDMA_time * 3;
      double sdr2_rx2 = sdr2_begin + TDMA_time * 3 - wait_time;

      double sdr1_rx2 = sdr1_begin + TDMA_time * 4 - wait_time;
      double sdr2_tx2 = sdr2_begin + TDMA_time * 4;

      double sdr1_tx3 = sdr1_begin + TDMA_time * 5;
      double sdr2_rx3 = sdr2_begin + TDMA_time * 5 - wait_time;

      double sdr1_rx3 = sdr1_begin + TDMA_time * 6 - wait_time;
      double sdr2_tx3 = sdr2_begin + TDMA_time * 6;

      double sdr1_tx4 = sdr1_begin + TDMA_time * 6;
      double sdr2_rx4 = sdr2_begin + TDMA_time * 6 - wait_time;

      // std::cout << "[SDR1] Scheduled start at: " << sdr1_begin << std::endl;
      // std::cout << "[SDR2] Scheduled start at: " << sdr2_begin << std::endl;

      // std::cout << "[RX1] Scheduled RX at: " << sdr2_rx1 << std::endl;
      // std::cout << "[TX1] Scheduled TX at: " << sdr1_tx1 << std::endl;

      // std::cout << "[RX2] Scheduled RX at: " << sdr1_rx1 << std::endl;
      // std::cout << "[TX2] Scheduled TX at: " << sdr2_tx1 << std::endl;

      // std::cout << "[RX3] Scheduled RX at: " << sdr2_rx2 << std::endl;
      // std::cout << "[TX3] Scheduled TX at: " << sdr1_tx2 << std::endl;

      // std::cout << "[RX4] Scheduled RX at: " << sdr1_rx2 << std::endl;
      // std::cout << "[TX4] Scheduled TX at: " << sdr2_tx2 << std::endl;

      // std::cout << "[RX5] Scheduled RX at: " << sdr2_rx3 << std::endl;
      // std::cout << "[TX5] Scheduled TX at: " << sdr1_tx3 << std::endl;

      // std::cout << "[RX6] Scheduled RX at: " << sdr1_rx3 << std::endl;
      // std::cout << "[TX6] Scheduled TX at: " << sdr2_tx3 << std::endl;

      // std::cout << "[RX6 (True)] Scheduled RX at: " << sdr2_rx4 << std::endl;
      // std::cout << "[TX6 (true)] Scheduled TX at: " << sdr1_tx4 << std::endl;

      // Threads
      std::vector<double> sdr1_tx_times = {sdr1_tx1, sdr1_tx2, sdr1_tx3, sdr1_tx4};
      std::vector<double> sdr1_rx_times = {sdr1_rx1, sdr1_rx2};
      std::vector<double> sdr2_tx_times = {sdr2_tx1, sdr2_tx2};
      std::vector<double> sdr2_rx_times = {sdr2_rx1, sdr2_rx2, sdr2_rx3, sdr2_rx4};

      std::vector<int> sdr1_rx_tags = {2, 2};
      std::vector<int> sdr2_rx_tags = {1, 1, 3, 3};

      std::vector<bool> sdr1_rx_final = {false, false};
      std::vector<bool> sdr2_rx_final = {false, false, false, true};

      sdr1_tx_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_all, this, usrp_1, tx1_stream, sdr1_tx_times);

      sdr1_rx_thread = gr::thread::thread(
          &usrp_radar_all_impl::receive_all,
          this, usrp_1, rx1_stream, sdr1_rx_times, sdr1_rx_tags, sdr1_rx_final);

      sdr2_tx_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_all, this, usrp_2, tx2_stream, sdr2_tx_times);

      sdr2_rx_thread = gr::thread::thread(
          &usrp_radar_all_impl::receive_all,
          this, usrp_2, rx2_stream, sdr2_rx_times, sdr2_rx_tags, sdr2_rx_final);

      sdr1_tx_thread.join();
      sdr1_rx_thread.join();
      sdr2_tx_thread.join();
      sdr2_rx_thread.join();

      // One Pulse Test
      // // Transmit and Receive Threads
      // tx1_thread = gr::thread::thread(
      //     &usrp_radar_all_impl::transmit_bursts, this, usrp_1, tx1_stream, sdr1_tx1);

      // rx2_thread =
      //     gr::thread::thread(&usrp_radar_all_impl::receive, this, usrp_2, rx2_stream, sdr2_rx1, 1, true);
      // tx1_thread.join();
      // rx2_thread.join();
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
      // usrp_1->set_time_now(uhd::time_spec_t(0.0));
      // usrp_2->set_time_now(uhd::time_spec_t(0.0));

      usrp_1->set_clock_source("external");
      usrp_2->set_clock_source("external");
      usrp_1->set_time_unknown_pps(uhd::time_spec_t(0.0));
      usrp_2->set_time_unknown_pps(uhd::time_spec_t(0.0));

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
                                      double start_time, int sdr_id, bool TDMA_done)
    {
      // Setup variables
      uhd::rx_metadata_t md;
      // Total samples to receive based on capture time
      size_t total_samps_to_rx = cap_length * sdr1_rate;
      size_t samps_received = 0;

      // Set up UHD receive mode
      uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
      cmd.num_samps = total_samps_to_rx;
      cmd.stream_now = usrp_rx->get_time_now().get_real_secs() >= start_time;
      double rx_time_secs = usrp_rx->get_time_now().get_real_secs();
      pmt::pmt_t rx_time = pmt::from_double(rx_time_secs);

      // Outputs RX time
      // std::cout << "Current time: " << rx_time << ", Start time: " << start_time << std::endl;

      cmd.time_spec = uhd::time_spec_t(start_time);
      rx_stream->issue_stream_cmd(cmd);

      // Allocate buffer
      pmt::pmt_t rx_data_pmt = pmt::make_c32vector(total_samps_to_rx, 0);
      gr_complex *rx_data_ptr = pmt::c32vector_writable_elements(rx_data_pmt, total_samps_to_rx);

      // Tells USRP to wait for incoming samples before giving up
      double timeout = 0.5;

      // Counts number of samples received until end of capture time
      while (samps_received < total_samps_to_rx)
      {
        size_t samps_to_recv = total_samps_to_rx - samps_received;
        size_t num_rx = rx_stream->recv(rx_data_ptr + samps_received, samps_to_recv, md, timeout);

        samps_received += num_rx;
      }

      if (pmt::length(this->meta) > 0)
      {
        this->meta = pmt::dict_add(
            this->meta, pmt::intern("rx_time"), rx_time);
        if (sdr_id == 1)
        {
          // GR_LOG_WARN(d_logger, "SDR ID: 1 Confirmed");
          this->meta = pmt::dict_add(this->meta, pmt::intern("src"), PMT_HARMONIA_SDR1);
        }
        else if (sdr_id == 2)
        {
          // GR_LOG_WARN(d_logger, "SDR ID: 2 Confirmed");
          this->meta = pmt::dict_add(this->meta, pmt::intern("src"), PMT_HARMONIA_SDR2);
        }
        else if (sdr_id == 3)
        {
          // GR_LOG_WARN(d_logger, "SDR ID: 2 Confirmed");
          this->meta = pmt::dict_add(this->meta, pmt::intern("src"), PMT_HARMONIA_SDR3);
        }
      }

      if (TDMA_done)
      {
        this->meta = pmt::dict_add(this->meta, pmt::intern("TDMA_Done"), pmt::PMT_T);
      }
      // Outputs metadata
      message_port_pub(PMT_HARMONIA_OUT, pmt::cons(this->meta, rx_data_pmt));
      // this->meta = pmt::make_dict();
    }

    void usrp_radar_all_impl::transmit_bursts(uhd::usrp::multi_usrp::sptr usrp_tx,
                                              uhd::tx_streamer::sptr tx_stream,
                                              double start_time)
    {
      // Create the metadata, and populate the time spec at the latest possible moment
      uhd::tx_metadata_t md;

      // Outputs TX time
      // double current_time = usrp_tx->get_time_now().get_real_secs();
      // std::cout << "Current time: " << current_time << ", Start time: " << start_time << std::endl;

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
      double timeout = 0.5;
      md.start_of_burst = true;
      md.end_of_burst = true;
      md.has_time_spec = true;
      md.time_spec = uhd::time_spec_t(start_time);
      // std::cout << "[TX burst] Sending at " << start_time << std::endl;
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
