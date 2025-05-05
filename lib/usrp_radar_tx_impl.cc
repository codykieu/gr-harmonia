/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "usrp_radar_tx_impl.h"
#include <gnuradio/io_signature.h>

float BURST_WARMUP_TIME1 = 3e-6;
float BURST_COOLDOWN_TIME1 = 0.5e-6;

namespace gr
{
  namespace harmonia
  {

    usrp_radar_tx::sptr usrp_radar_tx::make(const std::string &args_tx,
                                            const double tx_rate,
                                            const double tx_freq,
                                            const double tx_gain,
                                            const double start_delay,
                                            const bool elevate_priority,
                                            const bool verbose)
    {
      return gnuradio::make_block_sptr<usrp_radar_tx_impl>(args_tx,
                                                           tx_rate,
                                                           tx_freq,
                                                           tx_gain,
                                                           start_delay,
                                                           elevate_priority,
                                                           verbose);
    }

    /*
     * The private constructor
     */
    usrp_radar_tx_impl::usrp_radar_tx_impl(const std::string &args_tx,
                                           const double tx_rate,
                                           const double tx_freq,
                                           const double tx_gain,
                                           const double start_delay,
                                           const bool elevate_priority,
                                           const bool verbose)
        : gr::block("usrp_radar_tx",
                    gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
          usrp_args_tx(args_tx),
          tx_rate(tx_rate),
          tx_freq(tx_freq),
          tx_gain(tx_gain),
          start_delay(start_delay),
          elevate_priority(elevate_priority),
          verbose(verbose)
    {
      // Additional parameters. I have the hooks in to make them configurable, but we don't
      // need them right now.
      this->tx_subdev = "";
      this->tx_cpu_format = "fc32";
      this->tx_otw_format = "sc16";
      this->tx_device_addr = "";
      this->tx_channel_nums = std::vector<size_t>(1, 0);
      this->tx_buffs = std::vector<std::vector<gr_complex>>(tx_channel_nums.size());
      this->n_tx_total = 0;
      this->new_msg_received = false;
      this->meta = pmt::make_dict();

      config_usrp(this->usrp_tx,
                  this->usrp_args_tx,
                  this->tx_rate,
                  this->tx_freq,
                  this->tx_gain,
                  this->tx_subdev,
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
    usrp_radar_tx_impl::~usrp_radar_tx_impl() {}

    bool usrp_radar_tx_impl::start()
    {
      finished = false;
      main_thread = gr::thread::thread(&usrp_radar_tx_impl::run, this);
      return block::start();
    }

    bool usrp_radar_tx_impl::stop()
    {
      finished = true;
      return block::stop();
    }

    void usrp_radar_tx_impl::handle_message(const pmt::pmt_t &msg)
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
        }

        new_msg_received = true;
      }
    }

    void usrp_radar_tx_impl::run()
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

      double start_time = usrp_tx->get_time_now().get_real_secs() + start_delay;
      /***********************************************************************
       * Transmit thread
       **********************************************************************/
      uhd::stream_args_t tx_stream_args(tx_cpu_format, tx_otw_format);
      tx_stream_args.channels = tx_channel_nums;
      tx_stream_args.args = uhd::device_addr_t(tx_device_addr);
      uhd::tx_streamer::sptr tx_stream = usrp_tx->get_tx_stream(tx_stream_args);
      if (prf > 0)
      {
        tx_thread = gr::thread::thread(
            &usrp_radar_tx_impl::transmit_bursts, this, usrp_tx, tx_stream, start_time);
      }
      else
      {
        tx_thread = gr::thread::thread(
            &usrp_radar_tx_impl::transmit_continuous, this, usrp_tx, tx_stream, start_time);
      }

      tx_thread.join();
    }

    void usrp_radar_tx_impl::config_usrp(uhd::usrp::multi_usrp::sptr &usrp_tx,
                                         const std::string &args_tx,
                                         const double tx_rate,
                                         const double tx_freq,
                                         const double tx_gain,
                                         const std::string &tx_subdev,
                                         bool verbose)
    {
      usrp_tx = uhd::usrp::multi_usrp::make(args_tx);
      if (not tx_subdev.empty())
      {
        usrp_tx->set_tx_subdev_spec(tx_subdev);
      }
      usrp_tx->set_tx_rate(tx_rate);
      usrp_tx->set_tx_freq(tx_freq);
      usrp_tx->set_tx_gain(tx_gain);

      usrp_tx->set_time_now(uhd::time_spec_t(0.0));

      if (verbose)
      {
        std::cout << boost::format("Using Device: %s") % usrp_tx->get_pp_string()
                  << std::endl;
        std::cout << boost::format("Actual TX Rate: %f Msps") %
                         (usrp_tx->get_tx_rate() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual TX Freq: %f MHz") % (usrp_tx->get_tx_freq() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual TX Gain: %f dB") % usrp_tx->get_tx_gain()
                  << std::endl;
      }
    }

    void usrp_radar_tx_impl::transmit_bursts(uhd::usrp::multi_usrp::sptr usrp_tx,
                                             uhd::tx_streamer::sptr tx_stream,
                                             double start_time)
    {
      if (elevate_priority)
      {
        uhd::set_thread_priority(1.0);
      }
      // Create the metadata, and populate the time spec at the latest possible moment
      uhd::tx_metadata_t md;
      std::vector<gr_complex> cooldown_zeros(round(BURST_COOLDOWN_TIME1 * tx_rate),
                                             gr_complex(0, 0));
      std::vector<gr_complex> warmup_zeros(round(BURST_WARMUP_TIME1 * tx_rate),
                                           gr_complex(0, 0));
      while (not finished)
      {
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
        md.start_of_burst = true;
        md.end_of_burst = false;
        md.has_time_spec = true;
        md.time_spec = uhd::time_spec_t(start_time) - BURST_WARMUP_TIME1;

        double timeout = 0.1 + std::max(1 / prf, start_time);

        tx_stream->send(warmup_zeros.data(), warmup_zeros.size(), md);
        md.start_of_burst = false;
        md.has_time_spec = false;
        n_tx_total +=
            tx_stream->send(tx_buffs[0].data(), tx_buffs[0].size(), md, timeout);

        md.end_of_burst = true;
        tx_stream->send(cooldown_zeros.data(), cooldown_zeros.size(), md);

        start_time += 1 / prf;
      }
    }

    void usrp_radar_tx_impl::transmit_continuous(uhd::usrp::multi_usrp::sptr usrp_tx,
                                                 uhd::tx_streamer::sptr tx_stream,
                                                 double start_time)
    {
      if (elevate_priority)
      {
        uhd::set_thread_priority(1.0);
      }
      // Create the metadata, and populate the time spec at the latest possible moment
      uhd::tx_metadata_t md;
      md.has_time_spec = true;
      md.time_spec = uhd::time_spec_t(start_time);

      double timeout = 0.1 + start_time;
      bool first = true;
      while (not finished)
      {
        if (new_msg_received)
        {
          tx_buffs[0] = pmt::c32vector_elements(tx_data);
          meta =
              pmt::dict_add(meta, pmt::intern(tx_freq_key), pmt::from_double(tx_freq));
          meta = pmt::dict_add(
              meta, pmt::intern(sample_start_key), pmt::from_long(n_tx_total));
          new_msg_received = false;
        }
        n_tx_total += tx_stream->send(tx_buffs[0].data(), tx_buff_size, md, timeout) *
                      tx_stream->get_num_channels();

        if (first)
        {
          first = false;
          md.has_time_spec = false;
          timeout = 0.5;
        }
      }

      // send a mini EOB packet
      md.end_of_burst = true;
      tx_stream->send("", 0, md);
    }

    void usrp_radar_tx_impl::set_metadata_keys(const std::string &tx_freq_key,
                                               const std::string &sample_start_key,
                                               const std::string &prf_key)
    {
      this->tx_freq_key = tx_freq_key;
      this->sample_start_key = sample_start_key;
      this->prf_key = prf_key;
    }

  } /* namespace harmonia */
} /* namespace gr */
