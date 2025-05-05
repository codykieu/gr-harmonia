/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "usrp_radar_rx_impl.h"
#include <gnuradio/io_signature.h>

namespace gr
{
  namespace harmonia
  {

    usrp_radar_rx::sptr usrp_radar_rx::make(const std::string &args_rx,
                                            const double rx_rate,
                                            const double rx_freq,
                                            const double rx_gain,
                                            const double start_delay,
                                            const bool elevate_priority,
                                            const bool verbose)
    {
      return gnuradio::make_block_sptr<usrp_radar_rx_impl>(args_rx,
                                                           rx_rate,
                                                           rx_freq,
                                                           rx_gain,
                                                           start_delay,
                                                           elevate_priority,
                                                           verbose);
    }

    /*
     * The private constructor
     */
    usrp_radar_rx_impl::usrp_radar_rx_impl(const std::string &args_rx,
                                           const double rx_rate,
                                           const double rx_freq,
                                           const double rx_gain,
                                           const double start_delay,
                                           const bool elevate_priority,
                                           const bool verbose)
        : gr::block("usrp_radar_rx",
                    gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
          usrp_args_rx(args_rx),
          rx_rate(rx_rate),
          rx_freq(rx_freq),
          rx_gain(rx_gain),
          start_delay(start_delay),
          elevate_priority(elevate_priority),
          verbose(verbose)
    {
      // Additional parameters. I have the hooks in to make them configurable, but we don't
      // need them right now.
      this->rx_subdev = "";
      this->rx_cpu_format = "fc32";
      this->rx_otw_format = "sc16";
      this->rx_device_addr = "";
      this->rx_channel_nums = std::vector<size_t>(1, 0);
      this->n_tx_total = 0;
      this->new_msg_received = false;
      this->meta = pmt::make_dict();

      config_usrp(this->usrp_rx,
                  this->usrp_args_rx,
                  this->rx_rate,
                  this->rx_freq,
                  this->rx_gain,
                  this->rx_subdev,
                  this->verbose);

      n_delay = 0;

      message_port_register_out(PMT_HARMONIA_OUT);
      set_msg_handler(PMT_HARMONIA_IN, [this](pmt::pmt_t msg)
                      { this->handle_message(msg); });
    }

    /*
     * Our virtual destructor.
     */
    usrp_radar_rx_impl::~usrp_radar_rx_impl() {}

    bool usrp_radar_rx_impl::start()
    {
      finished = false;
      main_thread = gr::thread::thread(&usrp_radar_rx_impl::run, this);
      return block::start();
    }

    bool usrp_radar_rx_impl::stop()
    {
      finished = true;
      return block::stop();
    }

    void usrp_radar_rx_impl::handle_message(const pmt::pmt_t &msg)
    {
      if (pmt::is_pdu(msg))
      {
        meta = pmt::dict_update(meta, pmt::car(msg));
        tx_data = pmt::cdr(msg);
        rx_buff_size = pmt::length(tx_data);

        if (pmt::dict_has_key(meta, pmt::intern(prf_key)))
        {
          prf = pmt::to_double(
              pmt::dict_ref(meta, pmt::intern(prf_key), pmt::from_double(0.0)));

          rx_buff_size = round(rx_rate / prf);
        }

        new_msg_received = true;
      }
    }

    void usrp_radar_rx_impl::run()
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

      double start_time = usrp_rx->get_time_now().get_real_secs() + start_delay;

      /***********************************************************************
       * Receive thread
       **********************************************************************/
      // create a receive streamer
      uhd::stream_args_t rx_stream_args(rx_cpu_format, rx_otw_format);
      rx_stream_args.channels = rx_channel_nums;
      rx_stream_args.args = uhd::device_addr_t(rx_device_addr);
      uhd::rx_streamer::sptr rx_stream = usrp_rx->get_rx_stream(rx_stream_args);
      rx_thread =
          gr::thread::thread(&usrp_radar_rx_impl::receive, this, usrp_rx, rx_stream, start_time);
      rx_thread.join();
    }

    void usrp_radar_rx_impl::config_usrp(uhd::usrp::multi_usrp::sptr &usrp_rx,
                                         const std::string &args_rx,
                                         const double rx_rate,
                                         const double rx_freq,
                                         const double rx_gain,
                                         const std::string &rx_subdev,
                                         bool verbose)
    {
      usrp_rx = uhd::usrp::multi_usrp::make(args_rx);
      if (not rx_subdev.empty())
      {
        usrp_rx->set_rx_subdev_spec(rx_subdev);
      }
      usrp_rx->set_rx_rate(rx_rate);
      usrp_rx->set_rx_freq(rx_freq);
      usrp_rx->set_rx_gain(rx_gain);

      usrp_rx->set_time_now(uhd::time_spec_t(0.0));

      if (verbose)
      {
        std::cout << boost::format("Using Device: %s") % usrp_rx->get_pp_string()
                  << std::endl;
        std::cout << boost::format("Actual RX Rate: %f Msps") %
                         (usrp_rx->get_rx_rate() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual RX Freq: %f MHz") % (usrp_rx->get_rx_freq() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual RX Gain: %f dB") % usrp_rx->get_rx_gain()
                  << std::endl;
      }
    }
    void usrp_radar_rx_impl::receive(uhd::usrp::multi_usrp::sptr usrp_rx,
                                     uhd::rx_streamer::sptr rx_stream,
                                     double start_time)
    {
      if (elevate_priority)
      {
        uhd::set_thread_priority(1.0);
      }
      // setup variables
      uhd::rx_metadata_t md;
      uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
      cmd.time_spec = uhd::time_spec_t(start_time);
      cmd.stream_now = usrp_rx->get_time_now().get_real_secs() >= start_time;
      rx_stream->issue_stream_cmd(cmd);

      // Set up and allocate buffers
      pmt::pmt_t rx_data_pmt = pmt::make_c32vector(rx_buff_size, 0);
      gr_complex *rx_data_ptr = pmt::c32vector_writable_elements(rx_data_pmt, rx_buff_size);

      double time_until_start = start_time - usrp_rx->get_time_now().get_real_secs();
      double timeout = 0.1 + time_until_start;

      // TODO: Handle multiple channels (e.g., one out port per channel)
      while (true)
      {
        if (finished)
        {
          rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
        }

        if (n_delay > 0)
        {
          // Throw away n_delay samples at the beginning
          std::vector<gr_complex> dummy_vec(n_delay);
          n_delay -= rx_stream->recv(dummy_vec.data(), n_delay, md, timeout);
        }
        rx_stream->recv(rx_data_ptr, rx_buff_size, md, timeout);
        timeout = 0.1;

        if (pmt::length(this->meta) > 0)
        {
          this->meta = pmt::dict_add(
              this->meta, pmt::intern(rx_freq_key), pmt::from_double(rx_freq));
        }
        message_port_pub(PMT_HARMONIA_OUT, pmt::cons(this->meta, rx_data_pmt));
        this->meta = pmt::make_dict();

        if (finished and md.end_of_burst)
        {
          return;
        }
      }
    }

    void usrp_radar_rx_impl::set_metadata_keys(const std::string &rx_freq_key,
                                              const std::string &sample_start_key,
                                              const std::string &prf_key)
    {
      this->rx_freq_key = rx_freq_key;
      this->sample_start_key = sample_start_key;
      this->prf_key = prf_key;
    }

  } /* namespace harmonia */
} /* namespace gr */
