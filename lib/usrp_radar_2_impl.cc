/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "usrp_radar_2_impl.h"
#include <gnuradio/io_signature.h>

float BURST_WARMUP_TIME = 3e-6;
float BURST_COOLDOWN_TIME = 0.5e-6;

namespace gr
{
  namespace harmonia
  {

    usrp_radar_2::sptr usrp_radar_2::make(const std::string &args_tx,
                                          const double tx_rate,
                                          const double rx_rate,
                                          const double tx_freq,
                                          const double rx_freq,
                                          const double tx_gain,
                                          const double rx_gain,
                                          const double start_delay,
                                          const bool elevate_priority,
                                          const std::string &cal_file,
                                          const bool verbose,
                                          double cap_length)
    {
      return gnuradio::make_block_sptr<usrp_radar_2_impl>(args_tx,
                                                          tx_rate,
                                                          rx_rate,
                                                          tx_freq,
                                                          rx_freq,
                                                          tx_gain,
                                                          rx_gain,
                                                          start_delay,
                                                          elevate_priority,
                                                          cal_file,
                                                          verbose,
                                                          cap_length);
    }

    usrp_radar_2_impl::usrp_radar_2_impl(const std::string &args_tx,
                                         const double tx_rate,
                                         const double rx_rate,
                                         const double tx_freq,
                                         const double rx_freq,
                                         const double tx_gain,
                                         const double rx_gain,
                                         const double start_delay,
                                         const bool elevate_priority,
                                         const std::string &cal_file,
                                         const bool verbose,
                                         double cap_length)
        : gr::block(
              "usrp_radar_2", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
          usrp_args_tx(args_tx),
          tx_rate(tx_rate),
          rx_rate(rx_rate),
          tx_freq(tx_freq),
          rx_freq(rx_freq),
          tx_gain(tx_gain),
          rx_gain(rx_gain),
          start_delay(start_delay),
          elevate_priority(elevate_priority),
          calibration_file(cal_file),
          verbose(verbose),
          cap_length(cap_length)
    {
      // Additional parameters. I have the hooks in to make them configurable, but we don't
      // need them right now.
      this->tx_subdev = this->rx_subdev = "";
      // this->rx_subdev = "";
      this->tx_cpu_format = this->rx_cpu_format = "fc32";
      this->tx_otw_format = this->rx_otw_format = "sc16";
      this->tx_device_addr = this->rx_device_addr = "";
      // this->rx_device_addr = "";
      this->tx_channel_nums = std::vector<size_t>(1, 0);
      this->rx_channel_nums = std::vector<size_t>(1, 0);
      this->tx_buffs = std::vector<std::vector<gr_complex>>(tx_channel_nums.size());

      this->n_tx_total = 0;
      this->new_msg_received = false;
      this->meta = pmt::make_dict();

      config_usrp(this->usrp_tx,
                  this->usrp_args_tx,
                  this->tx_rate,
                  this->rx_rate,
                  this->tx_freq,
                  this->rx_freq,
                  this->tx_gain,
                  this->rx_gain,
                  this->tx_subdev,
                  this->rx_subdev,
                  this->verbose);

      n_delay = 0;
      if (not cal_file.empty())
      {
        read_calibration_file(cal_file);
      }

      message_port_register_in(PMT_HARMONIA_IN);
      message_port_register_out(PMT_HARMONIA_OUT);
      set_msg_handler(PMT_HARMONIA_IN, [this](pmt::pmt_t msg)
                      { this->handle_message(msg); });
    }

    usrp_radar_2_impl::~usrp_radar_2_impl() {}

    bool usrp_radar_2_impl::start()
    {
      finished = false;
      main_thread = gr::thread::thread(&usrp_radar_2_impl::run, this);
      return block::start();
    }

    bool usrp_radar_2_impl::stop()
    {
      finished = true;
      return block::stop();
    }

    void usrp_radar_2_impl::handle_message(const pmt::pmt_t &msg)
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
            rx_buff_size = round(rx_rate / prf);
          }
        }

        new_msg_received = true;
      }
    }

    void usrp_radar_2_impl::run()
    {
      double start_time = usrp_tx->get_time_now().get_real_secs() + start_delay;

      /***********************************************************************
       * Receive thread
       **********************************************************************/
      // create a receive streamer
      uhd::stream_args_t rx_stream_args(rx_cpu_format, rx_otw_format);
      rx_stream_args.channels = rx_channel_nums;
      rx_stream_args.args = uhd::device_addr_t(rx_device_addr);
      uhd::rx_streamer::sptr rx_stream = usrp_rx->get_rx_stream(rx_stream_args);
      rx_thread =
          gr::thread::thread(&usrp_radar_2_impl::receive, this, usrp_rx, rx_stream, start_time);

      /***********************************************************************
       * Transmit thread
       **********************************************************************/
      uhd::stream_args_t tx_stream_args(tx_cpu_format, tx_otw_format);
      tx_stream_args.channels = tx_channel_nums;
      tx_stream_args.args = uhd::device_addr_t(tx_device_addr);
      uhd::tx_streamer::sptr tx_stream = usrp_tx->get_tx_stream(tx_stream_args);

      tx_thread = gr::thread::thread(
          &usrp_radar_2_impl::transmit_bursts, this, usrp_tx, tx_stream, start_time);

      tx_thread.join();
      rx_thread.join();
    }

    void usrp_radar_2_impl::config_usrp(uhd::usrp::multi_usrp::sptr &usrp_tx,
                                        const std::string &args_tx,
                                        const double tx_rate,
                                        const double rx_rate,
                                        const double tx_freq,
                                        const double rx_freq,
                                        const double tx_gain,
                                        const double rx_gain,
                                        const std::string &tx_subdev,
                                        const std::string &rx_subdev,
                                        bool verbose)
    {
      usrp_tx = uhd::usrp::multi_usrp::make(args_tx);
      if (not tx_subdev.empty())
      {
        usrp_tx->set_tx_subdev_spec(tx_subdev);
      }
      if (not rx_subdev.empty())
      {
        usrp_tx->set_rx_subdev_spec(rx_subdev);
      }
      usrp_tx->set_tx_rate(tx_rate);
      usrp_tx->set_rx_rate(rx_rate);
      usrp_tx->set_tx_freq(tx_freq);
      usrp_tx->set_rx_freq(rx_freq);
      usrp_tx->set_tx_gain(tx_gain);
      usrp_tx->set_rx_gain(rx_gain);

      usrp_tx->set_time_now(uhd::time_spec_t(0.0));
      // usrp_rx->set_time_now(uhd::time_spec_t(0.0));

      if (verbose)
      {
        std::cout << boost::format("Using Device: %s") % usrp_tx->get_pp_string()
                  << std::endl;
        std::cout << boost::format("Actual TX Rate: %f Msps") %
                         (usrp_tx->get_tx_rate() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual RX Rate: %f Msps") %
                         (usrp_tx->get_rx_rate() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual TX Freq: %f MHz") % (usrp_tx->get_tx_freq() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual RX Freq: %f MHz") % (usrp_tx->get_rx_freq() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual TX Gain: %f dB") % usrp_tx->get_tx_gain()
                  << std::endl;
        std::cout << boost::format("Actual RX Gain: %f dB") % usrp_tx->get_rx_gain()
                  << std::endl;
      }
    }

    void usrp_radar_2_impl::receive(uhd::usrp::multi_usrp::sptr usrp_tx,
                                    uhd::rx_streamer::sptr rx_stream,
                                    double start_time)
    {
      if (elevate_priority)
      {
        uhd::set_thread_priority(1.0);
      }
      // setup variables
      uhd::rx_metadata_t md;
      // Total samples to receive based on capture time
      size_t total_samps_to_rx = cap_length * tx_rate;
      size_t samps_received = 0;

      // Set up UHD receive mode
      uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
      cmd.num_samps = total_samps_to_rx;
      cmd.stream_now = usrp_tx->get_time_now().get_real_secs() >= start_time;

      cmd.time_spec = uhd::time_spec_t(start_time);
      rx_stream->issue_stream_cmd(cmd);

      // Allocate buffer
      pmt::pmt_t rx_data_pmt = pmt::make_c32vector(total_samps_to_rx, 0);
      gr_complex *rx_data_ptr = pmt::c32vector_writable_elements(rx_data_pmt, total_samps_to_rx);

      double timeout = 0.1;
      // Counts number of samples received until end of capture time
      while (samps_received < total_samps_to_rx)
      {
        size_t samps_to_recv = total_samps_to_rx - samps_received;
        size_t num_rx = rx_stream->recv(rx_data_ptr + samps_received, samps_to_recv, md, timeout);

        samps_received += num_rx;
      }

      message_port_pub(PMT_HARMONIA_OUT, pmt::cons(this->meta, rx_data_pmt));
      this->meta = pmt::make_dict();
    }

    void usrp_radar_2_impl::transmit_bursts(uhd::usrp::multi_usrp::sptr usrp_tx,
                                            uhd::tx_streamer::sptr tx_stream,
                                            double start_time)
    {
      if (elevate_priority)
      {
        uhd::set_thread_priority(1.0);
      }
      // Create the metadata, and populate the time spec at the latest possible moment
      uhd::tx_metadata_t md;

      if (new_msg_received)
      {
        std::vector<gr_complex> tx_data_vector = pmt::c32vector_elements(tx_data);
        tx_buffs[0] = tx_data_vector;
        meta =
            pmt::dict_add(meta, pmt::intern(tx_freq_key), pmt::from_double(tx_freq));
        meta = pmt::dict_add(
            meta, pmt::intern(sample_start_key), pmt::from_long(n_tx_total));
        new_msg_received = false;
      }

      // Populate metadata
      md.start_of_burst = true;
      md.end_of_burst = true;
      md.has_time_spec = true;
      md.time_spec = uhd::time_spec_t(start_time);

      double timeout = 0.1;
      tx_stream->send(tx_buffs[0].data(), tx_buffs[0].size(), md, timeout);
    }

    void usrp_radar_2_impl::read_calibration_file(const std::string &filename)
    {
      std::ifstream file(filename);
      nlohmann::json json;
      if (file)
      {
        file >> json;
        std::string radio_type = usrp_tx->get_mboard_name();
        for (auto &config : json[radio_type])
        {
          if (config["samp_rate"] == usrp_tx->get_tx_rate() and
              config["master_clock_rate"] == usrp_tx->get_master_clock_rate())
          {
            n_delay = config["delay"];
            break;
          }
        }
        if (n_delay == 0)
          UHD_LOG_INFO("USRP Radar",
                       "Calibration file found, but no data exists for this "
                       "combination of radio, master clock rate, and sample rate");
      }
      else
      {
        UHD_LOG_INFO("USRP Radar", "No calibration file found");
      }

      file.close();
    }

    void usrp_radar_2_impl::set_metadata_keys(const std::string &tx_freq_key,
                                              const std::string &rx_freq_key,
                                              const std::string &sample_start_key,
                                              const std::string &prf_key)
    {
      this->tx_freq_key = tx_freq_key;
      this->rx_freq_key = rx_freq_key;
      this->sample_start_key = sample_start_key;
      this->prf_key = prf_key;
    }

  } /* namespace harmonia */
} /* namespace gr */
