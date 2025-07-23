/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "usrp_radar_all_impl.h"
#include <gnuradio/io_signature.h>

// Wire Delays
const double wdelay_tx1 = 0;
const double wdelay_rx1 = 0;
const double wdelay_tx2 = 0;
const double wdelay_rx2 = 0;
const double wdelay_tx3 = 0;
const double wdelay_rx3 = 0;

namespace gr
{
  namespace harmonia
  {

    usrp_radar_all::sptr usrp_radar_all::make(const std::string &args_1,
                                              const std::string &args_2,
                                              const std::string &args_3,
                                              const double sdr1_rate,
                                              const double sdr2_rate,
                                              const double sdr3_rate,
                                              const double sdr1_freq,
                                              const double sdr2_freq,
                                              const double sdr3_freq,
                                              const double sdr1_gain,
                                              const double sdr2_gain,
                                              const double sdr3_gain,
                                              const double start_delay,
                                              const double cap_length,
                                              const double cap_length2,
                                              const double wait_time,
                                              const double wait_time2,
                                              const double TDMA_time,
                                              const double TDMA_time2,
                                              const bool verbose)
    {
      return gnuradio::make_block_sptr<usrp_radar_all_impl>(args_1,
                                                            args_2,
                                                            args_3,
                                                            sdr1_rate,
                                                            sdr2_rate,
                                                            sdr3_rate,
                                                            sdr1_freq,
                                                            sdr2_freq,
                                                            sdr3_freq,
                                                            sdr1_gain,
                                                            sdr2_gain,
                                                            sdr3_gain,
                                                            start_delay,
                                                            cap_length,
                                                            cap_length2,
                                                            wait_time,
                                                            wait_time2,
                                                            TDMA_time,
                                                            TDMA_time2,
                                                            verbose);
    }

    /*
     * The private constructor
     */
    usrp_radar_all_impl::usrp_radar_all_impl(const std::string &args_1,
                                             const std::string &args_2,
                                             const std::string &args_3,
                                             const double sdr1_rate,
                                             const double sdr2_rate,
                                             const double sdr3_rate,
                                             const double sdr1_freq,
                                             const double sdr2_freq,
                                             const double sdr3_freq,
                                             const double sdr1_gain,
                                             const double sdr2_gain,
                                             const double sdr3_gain,
                                             const double start_delay,
                                             const double cap_length,
                                             const double cap_length2,
                                             const double wait_time,
                                             const double wait_time2,
                                             const double TDMA_time,
                                             const double TDMA_time2,
                                             const bool verbose)
        : gr::block("usrp_radar_all",
                    gr::io_signature::make(0, 0, 0),
                    gr::io_signature::make(0, 0, 0)),
          usrp_args_sdr1(args_1),
          usrp_args_sdr2(args_2),
          usrp_args_sdr3(args_3),
          sdr1_rate(sdr1_rate),
          sdr2_rate(sdr2_rate),
          sdr3_rate(sdr3_rate),
          sdr1_freq(sdr1_freq),
          sdr2_freq(sdr2_freq),
          sdr3_freq(sdr3_freq),
          sdr1_gain(sdr1_gain),
          sdr2_gain(sdr2_gain),
          sdr3_gain(sdr3_gain),
          start_delay(start_delay),
          cap_length(cap_length),
          cap_length2(cap_length2),
          wait_time(wait_time),
          wait_time2(wait_time2),
          TDMA_time(TDMA_time),
          TDMA_time2(TDMA_time2),
          verbose(verbose)
    {
      this->sdr1_subdev = "";
      this->sdr2_subdev = "";
      this->sdr3_subdev = "";
      this->sdr1_cpu_format = this->sdr2_cpu_format = this->sdr3_cpu_format = "fc32";
      this->sdr1_otw_format = this->sdr2_otw_format = this->sdr3_otw_format = "sc16";
      this->sdr1_device_addr = "";
      this->sdr2_device_addr = "";
      this->sdr3_device_addr = "";
      this->sdr1_channel_nums = std::vector<size_t>(1, 0);
      this->sdr2_channel_nums = std::vector<size_t>(1, 0);
      this->sdr3_channel_nums = std::vector<size_t>(1, 0);
      this->tx_buffs = std::vector<std::vector<gr_complex>>(sdr1_channel_nums.size());

      this->n_tx_total = 0;
      this->meta = pmt::make_dict();

      config_usrp(this->usrp_1,
                  this->usrp_2,
                  this->usrp_3,
                  this->usrp_args_sdr1,
                  this->usrp_args_sdr2,
                  this->usrp_args_sdr3,
                  this->sdr1_rate,
                  this->sdr2_rate,
                  this->sdr3_rate,
                  this->sdr1_freq,
                  this->sdr2_freq,
                  this->sdr3_freq,
                  this->sdr1_gain,
                  this->sdr2_gain,
                  this->sdr3_gain,
                  this->sdr1_subdev,
                  this->sdr2_subdev,
                  this->sdr3_subdev,
                  this->verbose);

      n_delay = 0;

      // Input Ports
      message_port_register_in(PMT_HARMONIA_IN);
      message_port_register_in(PMT_HARMONIA_IN2);
      message_port_register_in(PMT_HARMONIA_IN3);
      message_port_register_in(PMT_HARMONIA_LFM_IN);
      message_port_register_in(PMT_HARMONIA_LFM_IN2);
      message_port_register_in(PMT_HARMONIA_LFM_IN3);
      // Output Ports
      message_port_register_out(PMT_HARMONIA_OUT);
      message_port_register_out(PMT_HARMONIA_OUT2);
      message_port_register_out(PMT_HARMONIA_OUT3);
      message_port_register_out(PMT_HARMONIA_CD_OUT);
      message_port_register_out(PMT_HARMONIA_CD_OUT2);
      message_port_register_out(PMT_HARMONIA_CD_OUT3);
      message_port_register_out(PMT_HARMONIA_CB_OUT);
      message_port_register_out(PMT_HARMONIA_CB_OUT2);
      message_port_register_out(PMT_HARMONIA_CB_OUT3);
      message_port_register_out(PMT_HARMONIA_CP_OUT);
      message_port_register_out(PMT_HARMONIA_CP_OUT2);
      message_port_register_out(PMT_HARMONIA_CP_OUT3);

      message_port_register_in(PMT_HARMONIA_IN);
      set_msg_handler(PMT_HARMONIA_IN, [this](pmt::pmt_t msg)
                      {
                        this->handle_message(msg, 1); // SDR1
                      });

      message_port_register_in(PMT_HARMONIA_IN2);
      set_msg_handler(PMT_HARMONIA_IN2, [this](pmt::pmt_t msg)
                      {
                        this->handle_message(msg, 2); // SDR2
                      });

      message_port_register_in(PMT_HARMONIA_IN3);
      set_msg_handler(PMT_HARMONIA_IN3, [this](pmt::pmt_t msg)
                      {
                        this->handle_message(msg, 3); // SDR3
                      });

      message_port_register_in(PMT_HARMONIA_LFM_IN);
      set_msg_handler(PMT_HARMONIA_LFM_IN, [this](pmt::pmt_t msg)
                      {
                        this->handle_message(msg, 1); // SDR1
                      });

      message_port_register_in(PMT_HARMONIA_LFM_IN2);
      set_msg_handler(PMT_HARMONIA_LFM_IN2, [this](pmt::pmt_t msg)
                      {
                        this->handle_message(msg, 2); // SDR2
                      });

      message_port_register_in(PMT_HARMONIA_LFM_IN3);
      set_msg_handler(PMT_HARMONIA_LFM_IN3, [this](pmt::pmt_t msg)
                      {
                        this->handle_message(msg, 3); // SDR3
                      });
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

    void usrp_radar_all_impl::handle_message(const pmt::pmt_t &msg, int sdr_id)
    {
      if (!pmt::is_pair(msg))
        return;

      pmt::pmt_t meta = pmt::car(msg);
      pmt::pmt_t data = pmt::cdr(msg);

      // Extract PMT_HARMONIA_LABEL (e.g. "single_tone" or "LFM")
      pmt::pmt_t label = pmt::dict_ref(meta, PMT_HARMONIA_LABEL, pmt::PMT_NIL);

      // Check for clock drift/bias enabled
      clock_drift_enabled = pmt::to_bool(pmt::dict_ref(meta, pmt::intern("clock_drift_enable"), pmt::PMT_F));
      clock_bias_enabled = pmt::to_bool(pmt::dict_ref(meta, pmt::intern("clock_bias_enable"), pmt::PMT_F));
      carrier_phase_enabled = pmt::to_bool(pmt::dict_ref(meta, pmt::intern("carrier_phase_enable"), pmt::PMT_F));
      // GR_LOG_INFO(d_logger,
      //   std::string("clock_drift_enabled=") + (clock_drift_enabled?"true":"false")
      //   + ", clock_bias_enabled=" + (clock_bias_enabled?"true":"false"));

      // Update TX Data with New Waveforms
      switch (sdr_id)
      {
      case 1:
      {
        if (pmt::is_symbol(label))
        {
          if (pmt::equal(label, pmt::intern("single_tone")) && !clock_drift_enabled)
          {
            GR_LOG_INFO(d_logger, "WAVEFORM 1 UPDATED (single_tone, no drift)");
            tx_data_sdr1 = data;
            meta_sdr1 = meta;
            waveform1_ready = true;
          }
          else if (pmt::equal(label, pmt::intern("LFM")) && clock_drift_enabled && !clock_bias_enabled && !carrier_phase_enabled)
          {
            // GR_LOG_INFO(d_logger, "WAVEFORM 1 UPDATED (LFM, drift-enabled)");
            tx_data_sdr1 = data;
            meta_sdr1 = meta;
            waveform1_ready = true;
          }
          else if (pmt::equal(label, pmt::intern("LFM")) && clock_drift_enabled && clock_bias_enabled && !carrier_phase_enabled)
          {
            // GR_LOG_INFO(d_logger, "WAVEFORM 1 UPDATED (LFM, bias-enabled)");
            tx_data_sdr1 = data;
            meta_sdr1 = meta;
            waveform1_ready = true;
          }
          else if (pmt::equal(label, pmt::intern("LFM")) && clock_drift_enabled && clock_bias_enabled && carrier_phase_enabled)
          {
            // GR_LOG_INFO(d_logger, "WAVEFORM 1 UPDATED (LFM, phase-enabled)");
            tx_data_sdr1 = data;
            meta_sdr1 = meta;
            waveform1_ready = true;
          }
        }
        break;
      }

      case 2:
      {
        if (pmt::is_symbol(label))
        {
          if (pmt::equal(label, pmt::intern("single_tone")) && !clock_drift_enabled)
          {
            GR_LOG_INFO(d_logger, "WAVEFORM 2 UPDATED (single_tone, no drift)");
            tx_data_sdr2 = data;
            meta_sdr2 = meta;
            waveform2_ready = true;
          }
          else if (pmt::equal(label, pmt::intern("LFM")) && clock_drift_enabled && !clock_bias_enabled && !carrier_phase_enabled)
          {
            // GR_LOG_INFO(d_logger, "WAVEFORM 2 UPDATED (LFM, drift-enabled)");
            tx_data_sdr2 = data;
            meta_sdr2 = meta;
            waveform2_ready = true;
          }
          else if (pmt::equal(label, pmt::intern("LFM")) && clock_drift_enabled && clock_bias_enabled && !carrier_phase_enabled)
          {
            // GR_LOG_INFO(d_logger, "WAVEFORM 2 UPDATED (LFM, bias-enabled)");
            tx_data_sdr2 = data;
            meta_sdr2 = meta;
            waveform2_ready = true;
          }
          else if (pmt::equal(label, pmt::intern("LFM")) && clock_drift_enabled && clock_bias_enabled && carrier_phase_enabled)
          {
            // GR_LOG_INFO(d_logger, "WAVEFORM 2 UPDATED (LFM, phase-enabled)");
            tx_data_sdr2 = data;
            meta_sdr2 = meta;
            waveform2_ready = true;
          }
        }
        break;
      }

      case 3:
      {
        if (pmt::is_symbol(label))
        {
          if (pmt::equal(label, pmt::intern("single_tone")) && !clock_drift_enabled)
          {
            GR_LOG_INFO(d_logger, "WAVEFORM 3 UPDATED (single_tone, no drift)");
            tx_data_sdr3 = data;
            meta_sdr3 = meta;
            waveform3_ready = true;
          }
          else if (pmt::equal(label, pmt::intern("LFM")) && clock_drift_enabled && !clock_bias_enabled && !carrier_phase_enabled)
          {
            // GR_LOG_INFO(d_logger, "WAVEFORM 3 UPDATED (LFM, drift-enabled)");
            tx_data_sdr3 = data;
            meta_sdr3 = meta;
            waveform3_ready = true;
          }
          else if (pmt::equal(label, pmt::intern("LFM")) && clock_drift_enabled && clock_bias_enabled && !carrier_phase_enabled)
          {
            // GR_LOG_INFO(d_logger, "WAVEFORM 3 UPDATED (LFM, bias-enabled)");
            tx_data_sdr3 = data;
            meta_sdr3 = meta;
            waveform3_ready = true;
          }
          else if (pmt::equal(label, pmt::intern("LFM")) && clock_drift_enabled && clock_bias_enabled && carrier_phase_enabled)
          {
            // GR_LOG_INFO(d_logger, "WAVEFORM 3 UPDATED (LFM, phase-enabled)");
            tx_data_sdr3 = data;
            meta_sdr3 = meta;
            waveform3_ready = true;
          }
        }
        break;
      }
      }

      // Clock Drift Logic
      if (clock_drift_enabled)
      {
        pmt::pmt_t pmt_cd1 = pmt::dict_ref(meta, PMT_HARMONIA_SDR1, pmt::PMT_NIL);
        pmt::pmt_t pmt_cd2 = pmt::dict_ref(meta, PMT_HARMONIA_SDR2, pmt::PMT_NIL);
        pmt::pmt_t pmt_cd3 = pmt::dict_ref(meta, PMT_HARMONIA_SDR3, pmt::PMT_NIL);

        if (pmt::is_number(pmt_cd1))
        {
          cd1_est = pmt::to_double(pmt_cd1);
          cd1_ready = true;
        }
        if (pmt::is_number(pmt_cd2))
        {
          cd2_est = pmt::to_double(pmt_cd2);
          cd2_ready = true;
        }
        if (pmt::is_number(pmt_cd3))
        {
          cd3_est = pmt::to_double(pmt_cd3);
          cd3_ready = true;
        }
      }

      // Clock Bias Logic
      if (clock_bias_enabled)
      {
        pmt::pmt_t pmt_cb1 = pmt::dict_ref(meta, PMT_HARMONIA_CB_SDR1, pmt::PMT_NIL);
        pmt::pmt_t pmt_cb2 = pmt::dict_ref(meta, PMT_HARMONIA_CB_SDR2, pmt::PMT_NIL);
        pmt::pmt_t pmt_cb3 = pmt::dict_ref(meta, PMT_HARMONIA_CB_SDR3, pmt::PMT_NIL);

        if (pmt::is_number(pmt_cb1))
        {
          cb1_est = pmt::to_double(pmt_cb1);
          cb1_ready = true;
        }
        if (pmt::is_number(pmt_cb2))
        {
          cb2_est = pmt::to_double(pmt_cb2);
          cb2_ready = true;
        }
        if (pmt::is_number(pmt_cb3))
        {
          cb3_est = pmt::to_double(pmt_cb3);
          cb3_ready = true;
        }
      }

      // Checks what run to start
      check_cd_ready();
      // check_cb_ready();

      // if (carrier_phase_enabled && !cp_run_executed)
      // {
      //   GR_LOG_INFO(d_logger, "Carrier Phase enabled. Launching cd_run.");
      //   cp_run_executed = true;
      //   cp_run();
      // }
    }

    void usrp_radar_all_impl::check_cd_ready()
    {
      if (cd1_ready && cd2_ready && cd3_ready &&
          waveform1_ready && waveform2_ready && waveform3_ready &&
          clock_drift_enabled && !cd_run_executed)
      {
        cd_run_executed = true;
        GR_LOG_INFO(d_logger, "Clock drift enabled. Launching cd_run.");
        cd_run();
      }
    }

    void usrp_radar_all_impl::check_cb_ready()
    {
      if (cb1_ready && cb2_ready && cb3_ready &&
          waveform1_ready && waveform2_ready && waveform3_ready &&
          clock_bias_enabled && cd_run_executed && !cb_run_executed)
      {
        cb_run_executed = true;
        GR_LOG_INFO(d_logger, "Clock bias enabled. Launching cb_run.");
        cb_run();
      }
    }

    void usrp_radar_all_impl::config_usrp(uhd::usrp::multi_usrp::sptr &usrp_1,
                                          uhd::usrp::multi_usrp::sptr &usrp_2,
                                          uhd::usrp::multi_usrp::sptr &usrp_3,
                                          const std::string &args_1,
                                          const std::string &args_2,
                                          const std::string &args_3,
                                          const double sdr1_rate,
                                          const double sdr2_rate,
                                          const double sdr3_rate,
                                          const double sdr1_freq,
                                          const double sdr2_freq,
                                          const double sdr3_freq,
                                          const double sdr1_gain,
                                          const double sdr2_gain,
                                          const double sdr3_gain,
                                          const std::string &sdr1_subdev,
                                          const std::string &sdr2_subdev,
                                          const std::string &sdr3_subdev,
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
      if (not sdr3_subdev.empty())
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
      if (not sdr3_subdev.empty())
      {
        usrp_2->set_rx_subdev_spec(sdr2_subdev);
      }
      usrp_2->set_tx_rate(sdr2_rate);
      usrp_2->set_rx_rate(sdr2_rate);
      usrp_2->set_tx_freq(sdr2_freq);
      usrp_2->set_rx_freq(sdr2_freq);
      usrp_2->set_tx_gain(sdr2_gain);
      usrp_2->set_rx_gain(sdr2_gain);

      usrp_3 = uhd::usrp::multi_usrp::make(args_3);
      if (not sdr1_subdev.empty())
      {
        usrp_3->set_tx_subdev_spec(sdr3_subdev);
      }
      if (not sdr2_subdev.empty())
      {
        usrp_3->set_rx_subdev_spec(sdr3_subdev);
      }
      if (not sdr3_subdev.empty())
      {
        usrp_3->set_rx_subdev_spec(sdr3_subdev);
      }
      usrp_3->set_tx_rate(sdr3_rate);
      usrp_3->set_rx_rate(sdr3_rate);
      usrp_3->set_tx_freq(sdr3_freq);
      usrp_3->set_rx_freq(sdr3_freq);
      usrp_3->set_tx_gain(sdr3_gain);
      usrp_3->set_rx_gain(sdr3_gain);

      // Sets USRPs Clock Source
      usrp_1->set_clock_source("external");
      usrp_2->set_clock_source("external");
      usrp_3->set_clock_source("external");

      // Sets USRPs Time to 0.0 ***ONLY FOR COARSE SYNCHRONIZATION
      usrp_1->set_time_now(uhd::time_spec_t(0.0));
      usrp_3->set_time_now(uhd::time_spec_t(0.0));
      usrp_2->set_time_now(uhd::time_spec_t(0.0));

      if (verbose)
      {
        std::cout << boost::format("Using Device 1: %s") % usrp_1->get_pp_string()
                  << std::endl;
        std::cout << boost::format("Using Device 2: %s") % usrp_2->get_pp_string()
                  << std::endl;
        std::cout << boost::format("Using Device 3: %s") % usrp_3->get_pp_string()
                  << std::endl;
        std::cout << boost::format("Actual TX Rate: %f Msps") %
                         (usrp_1->get_tx_rate() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual RX Rate: %f Msps") %
                         (usrp_1->get_rx_rate() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual TX Freq: %f MHz") % (usrp_1->get_tx_freq() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual RX Freq: %f MHz") % (usrp_1->get_rx_freq() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual TX Gain: %f dB") % usrp_1->get_tx_gain()
                  << std::endl;
        std::cout << boost::format("Actual RX Gain: %f dB") % usrp_1->get_rx_gain()
                  << std::endl;
      }
    }

    void usrp_radar_all_impl::setup_streamers(
        uhd::rx_streamer::sptr &rx1_stream,
        uhd::rx_streamer::sptr &rx2_stream,
        uhd::rx_streamer::sptr &rx3_stream,
        uhd::tx_streamer::sptr &tx1_stream,
        uhd::tx_streamer::sptr &tx2_stream,
        uhd::tx_streamer::sptr &tx3_stream)
    {
      /***********************************************************************
       * Receive thread
       **********************************************************************/
      uhd::stream_args_t rx1_args(sdr1_cpu_format, sdr1_otw_format);
      rx1_args.channels = sdr1_channel_nums;
      rx1_args.args = uhd::device_addr_t(sdr1_device_addr);
      rx1_stream = usrp_1->get_rx_stream(rx1_args);

      uhd::stream_args_t rx2_args(sdr2_cpu_format, sdr2_otw_format);
      rx2_args.channels = sdr2_channel_nums;
      rx2_args.args = uhd::device_addr_t(sdr2_device_addr);
      rx2_stream = usrp_2->get_rx_stream(rx2_args);

      uhd::stream_args_t rx3_args(sdr3_cpu_format, sdr3_otw_format);
      rx3_args.channels = sdr3_channel_nums;
      rx3_args.args = uhd::device_addr_t(sdr3_device_addr);
      rx3_stream = usrp_3->get_rx_stream(rx3_args);

      /***********************************************************************
       * Transmit thread
       **********************************************************************/
      uhd::stream_args_t tx1_args(sdr1_cpu_format, sdr1_otw_format);
      tx1_args.channels = sdr1_channel_nums;
      tx1_args.args = uhd::device_addr_t(sdr1_device_addr);
      tx1_stream = usrp_1->get_tx_stream(tx1_args);

      uhd::stream_args_t tx2_args(sdr2_cpu_format, sdr2_otw_format);
      tx2_args.channels = sdr2_channel_nums;
      tx2_args.args = uhd::device_addr_t(sdr2_device_addr);
      tx2_stream = usrp_2->get_tx_stream(tx2_args);

      uhd::stream_args_t tx3_args(sdr3_cpu_format, sdr3_otw_format);
      tx3_args.channels = sdr3_channel_nums;
      tx3_args.args = uhd::device_addr_t(sdr3_device_addr);
      tx3_stream = usrp_3->get_tx_stream(tx3_args);
    }

    void usrp_radar_all_impl::transmit_all(
        uhd::usrp::multi_usrp::sptr usrp,
        uhd::tx_streamer::sptr tx_stream,
        const std::vector<double> &times,
        const std::vector<double> &tx_time_error,
        int sdr_id)
    {
      for (size_t i = 0; i < times.size(); ++i)
      {
        this->transmit_bursts(usrp, tx_stream, times[i], tx_time_error[i], sdr_id);
      }
    }

    void usrp_radar_all_impl::receive_all(
        uhd::usrp::multi_usrp::sptr usrp,
        uhd::rx_streamer::sptr rx_stream,
        const std::vector<double> &times,
        const std::vector<double> &rx_time_error,
        int sdr_id)
    {
      for (size_t i = 0; i < times.size(); ++i)
      {
        this->receive(usrp, rx_stream, times[i], rx_time_error[i], sdr_id);
      }
    }

    void usrp_radar_all_impl::run()
    {
      uhd::rx_streamer::sptr rx1_stream, rx2_stream, rx3_stream;
      uhd::tx_streamer::sptr tx1_stream, tx2_stream, tx3_stream;
      setup_streamers(rx1_stream, rx2_stream, rx3_stream, tx1_stream, tx2_stream, tx3_stream);

      /***********************************************************************
       * Thread Implementations
       **********************************************************************/
      double sdr1_begin = usrp_1->get_time_now().get_real_secs();
      double sdr2_begin = usrp_2->get_time_now().get_real_secs();
      double sdr3_begin = usrp_3->get_time_now().get_real_secs();

      std::cout << std::fixed << std::setprecision(12)
                << "[SDR1] Start Time: " << sdr1_begin << "\n"
                << "[SDR2] Start Time: " << sdr2_begin << "\n"
                << "[SDR3] Start Time: " << sdr3_begin << std::endl;

      // Transmit and Receive Times
      double sdr1_tx1 = start_delay + TDMA_time;
      double sdr2_rx1 = start_delay + TDMA_time - wait_time;
      double sdr3_rx1 = start_delay + TDMA_time - wait_time;

      double sdr2_tx1 = start_delay + TDMA_time * 2;
      double sdr1_rx1 = start_delay + TDMA_time * 2 - wait_time;
      double sdr3_rx2 = start_delay + TDMA_time * 2 - wait_time;

      double sdr3_tx1 = start_delay + TDMA_time * 3;
      double sdr1_rx2 = start_delay + TDMA_time * 3 - wait_time;
      double sdr2_rx2 = start_delay + TDMA_time * 3 - wait_time;

      // Rounded Down Version
      double resolution = 1 / sdr1_rate;
      double r1_tx1_err = std::fmod(sdr1_tx1, resolution);
      double r2_rx1_err = std::fmod(sdr2_rx1, resolution);
      double r3_rx1_err = std::fmod(sdr3_rx1, resolution);

      double r2_tx1_err = std::fmod(sdr2_tx1, resolution);
      double r1_rx1_err = std::fmod(sdr1_rx1, resolution);
      double r3_rx2_err = std::fmod(sdr3_rx2, resolution);

      double r3_tx1_err = std::fmod(sdr3_tx1, resolution);
      double r1_rx2_err = std::fmod(sdr1_rx2, resolution);
      double r2_rx2_err = std::fmod(sdr2_rx2, resolution);

      std::vector<double> sdr1_tx_times = {sdr1_tx1};
      std::vector<double> sdr2_tx_times = {sdr2_tx1};
      std::vector<double> sdr3_tx_times = {sdr3_tx1};

      std::vector<double> sdr1_rx_times = {sdr1_rx1, sdr1_rx2};
      std::vector<double> sdr2_rx_times = {sdr2_rx1, sdr2_rx2};
      std::vector<double> sdr3_rx_times = {sdr3_rx1, sdr3_rx2};

      // TX and RX Time Errors due to time resolution
      std::vector<double> sdr1_tx_times_err = {-r1_tx1_err};
      std::vector<double> sdr2_tx_times_err = {-r2_tx1_err};
      std::vector<double> sdr3_tx_times_err = {-r3_tx1_err};

      std::vector<double> sdr1_rx_times_err = {-r1_rx1_err, -r1_rx2_err};
      std::vector<double> sdr2_rx_times_err = {-r2_rx1_err, -r2_rx2_err};
      std::vector<double> sdr3_rx_times_err = {-r3_rx1_err, -r3_rx2_err};

      // Threads
      sdr1_tx_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_all, this, usrp_1, tx1_stream, sdr1_tx_times, sdr1_tx_times_err, 1);

      sdr1_rx_thread = gr::thread::thread(
          &usrp_radar_all_impl::receive_all,
          this, usrp_1, rx1_stream, sdr1_rx_times, sdr1_rx_times_err, 1);

      sdr2_tx_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_all, this, usrp_2, tx2_stream, sdr2_tx_times, sdr2_tx_times_err, 2);

      sdr2_rx_thread = gr::thread::thread(
          &usrp_radar_all_impl::receive_all,
          this, usrp_2, rx2_stream, sdr2_rx_times, sdr2_rx_times_err, 2);

      sdr3_tx_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_all, this, usrp_3, tx3_stream, sdr3_tx_times, sdr3_tx_times_err, 3);

      sdr3_rx_thread = gr::thread::thread(
          &usrp_radar_all_impl::receive_all,
          this, usrp_3, rx3_stream, sdr3_rx_times, sdr3_rx_times_err, 3);

      sdr1_tx_thread.join();
      sdr1_rx_thread.join();
      sdr2_tx_thread.join();
      sdr2_rx_thread.join();
      sdr3_tx_thread.join();
      sdr3_rx_thread.join();
    }

    void usrp_radar_all_impl::cd_run()
    {
      uhd::rx_streamer::sptr rx1_stream, rx2_stream, rx3_stream;
      uhd::tx_streamer::sptr tx1_stream, tx2_stream, tx3_stream;
      setup_streamers(rx1_stream, rx2_stream, rx3_stream, tx1_stream, tx2_stream, tx3_stream);

      // TX And RX Times
      double sdr1_tx1 = (start_delay * 2 + TDMA_time - wdelay_tx1) / cd1_est;
      double sdr2_rx1 = (start_delay * 2 + TDMA_time - wait_time - wdelay_rx2) / cd2_est;
      double sdr3_rx1 = (start_delay * 2 + TDMA_time - wait_time - wdelay_rx3) / cd3_est;

      double sdr2_tx1 = (start_delay * 2 + TDMA_time * 2 - wdelay_tx2) / cd2_est;
      double sdr1_rx1 = (start_delay * 2 + TDMA_time * 2 - wait_time - wdelay_rx1) / cd1_est;
      double sdr3_rx2 = (start_delay * 2 + TDMA_time * 2 - wait_time - wdelay_rx3) / cd3_est;

      double sdr3_tx1 = (start_delay * 2 + TDMA_time * 3 - wdelay_tx3) / cd3_est;
      double sdr1_rx2 = (start_delay * 2 + TDMA_time * 3 - wait_time - wdelay_rx1) / cd1_est;
      double sdr2_rx2 = (start_delay * 2 + TDMA_time * 3 - wait_time - wdelay_rx2) / cd2_est;

      // Rounded Down to nearest time resolution
      double resolution = 1 / sdr1_rate;
      double r1_tx1_err = std::fmod(sdr1_tx1, resolution);
      double r2_rx1_err = std::fmod(sdr2_rx1, resolution);
      double r3_rx1_err = std::fmod(sdr3_rx1, resolution);

      double r2_tx1_err = std::fmod(sdr2_tx1, resolution);
      double r1_rx1_err = std::fmod(sdr1_rx1, resolution);
      double r3_rx2_err = std::fmod(sdr3_rx2, resolution);

      double r3_tx1_err = std::fmod(sdr3_tx1, resolution);
      double r1_rx2_err = std::fmod(sdr1_rx2, resolution);
      double r2_rx2_err = std::fmod(sdr2_rx2, resolution);

      // // — print them out —
      // std::cout << std::fixed << std::setprecision(12)
      //           << "resdr1_tx1 = " << r1_tx1_err << "\n"
      //           << "resdr2_rx1 = " << r2_rx1_err << "\n"
      //           << "resdr3_rx1 = " << r3_rx1_err << "\n\n"

      //           << "resdr2_tx1 = " << r2_tx1_err << "\n"
      //           << "resdr1_rx1 = " << r1_rx1_err << "\n"
      //           << "resdr3_rx2 = " << r3_rx2_err << "\n\n"

      //           << "resdr3_tx1 = " << r3_tx1_err << "\n"
      //           << "resdr1_rx2 = " << r1_rx2_err << "\n"
      //           << "resdr2_rx2 = " << r2_rx2_err << "\n"
      //           << std::endl;

      // — print them out —
      std::cout << std::fixed << std::setprecision(12)
                << "sdr1_tx1 = " << sdr1_tx1 << "\n"
                << "sdr2_rx1 = " << sdr2_rx1 << "\n"
                << "sdr3_rx1 = " << sdr3_rx1 << "\n\n"

                << "sdr2_tx1 = " << sdr2_tx1 << "\n"
                << "sdr1_rx1 = " << sdr1_rx1 << "\n"
                << "sdr3_rx2 = " << sdr3_rx2 << "\n\n"

                << "sdr3_tx1 = " << sdr3_tx1 << "\n"
                << "sdr1_rx2 = " << sdr1_rx2 << "\n"
                << "sdr2_rx2 = " << sdr2_rx2 << "\n"
                << std::endl;

      // TX and RX Time Vectors
      std::vector<double> sdr1_tx_times = {sdr1_tx1};
      std::vector<double> sdr2_tx_times = {sdr2_tx1};
      std::vector<double> sdr3_tx_times = {sdr3_tx1};

      std::vector<double> sdr1_rx_times = {sdr1_rx1, sdr1_rx2};
      std::vector<double> sdr2_rx_times = {sdr2_rx1, sdr2_rx2};
      std::vector<double> sdr3_rx_times = {sdr3_rx1, sdr3_rx2};

      // TX and RX Errors due to time resolution for FFT-based Fractionally Delaying the Signals
      std::vector<double> sdr1_tx_times_err = {-r1_tx1_err};
      std::vector<double> sdr2_tx_times_err = {-r2_tx1_err};
      std::vector<double> sdr3_tx_times_err = {-r3_tx1_err};

      std::vector<double> sdr1_rx_times_err = {-r1_rx1_err, -r1_rx2_err};
      std::vector<double> sdr2_rx_times_err = {-r2_rx1_err, -r2_rx2_err};
      std::vector<double> sdr3_rx_times_err = {-r3_rx1_err, -r3_rx2_err};

      /////////////////////////////////////////////////////////////////////
      // // Print with std::cout
      // auto print_vec = [&](const std::string &name, const std::vector<double> &v)
      // {
      //   std::cout << name << ":";
      //   for (double t : v)
      //     std::cout << " " << std::fixed << std::setprecision(12) << t;
      //   std::cout << std::endl;
      // };
      // print_vec("sdr1_tx_times", sdr1_tx_times);
      // print_vec("sdr2_tx_times", sdr2_tx_times);
      // print_vec("sdr3_tx_times", sdr3_tx_times);

      // print_vec("sdr1_rx_times", sdr1_rx_times);
      // print_vec("sdr2_rx_times", sdr2_rx_times);
      // print_vec("sdr3_rx_times", sdr3_rx_times);
      ///////////////////////////////////////////////////////////////////

      // Threads
      cd_sdr1_tx_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_all, this, usrp_1, tx1_stream, sdr1_tx_times, sdr1_tx_times_err, 1);

      cd_sdr1_rx_thread = gr::thread::thread(
          &usrp_radar_all_impl::receive_all,
          this, usrp_1, rx1_stream, sdr1_rx_times, sdr1_rx_times_err, 1);

      cd_sdr2_tx_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_all, this, usrp_2, tx2_stream, sdr2_tx_times, sdr2_tx_times_err, 2);

      cd_sdr2_rx_thread = gr::thread::thread(
          &usrp_radar_all_impl::receive_all,
          this, usrp_2, rx2_stream, sdr2_rx_times, sdr2_rx_times_err, 2);

      cd_sdr3_tx_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_all, this, usrp_3, tx3_stream, sdr3_tx_times, sdr3_tx_times_err, 3);

      cd_sdr3_rx_thread = gr::thread::thread(
          &usrp_radar_all_impl::receive_all,
          this, usrp_3, rx3_stream, sdr3_rx_times, sdr3_rx_times_err, 3);

      cd_sdr1_tx_thread.join();
      cd_sdr2_rx_thread.join();
      cd_sdr2_tx_thread.join();
      cd_sdr1_rx_thread.join();
      cd_sdr3_tx_thread.join();
      cd_sdr3_rx_thread.join();

      GR_LOG_INFO(d_logger, "Drift TX/RX sequence completed.");
    }

    void usrp_radar_all_impl::cb_run()
    {
      uhd::rx_streamer::sptr rx1_stream, rx2_stream, rx3_stream;
      uhd::tx_streamer::sptr tx1_stream, tx2_stream, tx3_stream;
      setup_streamers(rx1_stream, rx2_stream, rx3_stream, tx1_stream, tx2_stream, tx3_stream);

      // TX and RX Times  **REMOVE WAIT_TIME or MAKE SMALLER??????
      double sdr1_tx1 = (start_delay * 3 + TDMA_time2 - wdelay_tx1 + cb1_est) / cd1_est;
      double sdr2_rx1 = (start_delay * 3 + TDMA_time2 - wdelay_rx2 + cb2_est) / cd2_est;
      double sdr3_rx1 = (start_delay * 3 + TDMA_time2 - wdelay_rx3 + cb3_est) / cd3_est;

      double sdr2_tx1 = (start_delay * 3 + TDMA_time2 * 2 - wdelay_tx2 + cb2_est) / cd2_est;
      double sdr1_rx1 = (start_delay * 3 + TDMA_time2 * 2 - wdelay_rx1 + cb1_est) / cd1_est;
      double sdr3_rx2 = (start_delay * 3 + TDMA_time2 * 2 - wdelay_rx3 + cb3_est) / cd3_est;

      double sdr3_tx1 = (start_delay * 3 + TDMA_time2 * 3 - wdelay_tx3 + cb3_est) / cd3_est;
      double sdr1_rx2 = (start_delay * 3 + TDMA_time2 * 3 - wdelay_rx1 + cb1_est) / cd1_est;
      double sdr2_rx2 = (start_delay * 3 + TDMA_time2 * 3 - wdelay_rx2 + cb2_est) / cd2_est;

      std::cout << std::fixed << std::setprecision(12)
                << "sdr1_tx1 = " << sdr1_tx1 << "\n"
                << "sdr2_rx1 = " << sdr2_rx1 << "\n"
                << "sdr3_rx1 = " << sdr3_rx1 << "\n\n"

                << "sdr2_tx1 = " << sdr2_tx1 << "\n"
                << "sdr1_rx1 = " << sdr1_rx1 << "\n"
                << "sdr3_rx2 = " << sdr3_rx2 << "\n\n"

                << "sdr3_tx1 = " << sdr3_tx1 << "\n"
                << "sdr1_rx2 = " << sdr1_rx2 << "\n"
                << "sdr2_rx2 = " << sdr2_rx2 << "\n"
                << std::endl;

      // Rounded Down nearest time resolution
      double resolution = 1 / sdr1_rate;
      double r1_tx1_err = std::fmod(sdr1_tx1, resolution);
      double r2_rx1_err = std::fmod(sdr2_rx1, resolution);
      double r3_rx1_err = std::fmod(sdr3_rx1, resolution);

      double r2_tx1_err = std::fmod(sdr2_tx1, resolution);
      double r1_rx1_err = std::fmod(sdr1_rx1, resolution);
      double r3_rx2_err = std::fmod(sdr3_rx2, resolution);

      double r3_tx1_err = std::fmod(sdr3_tx1, resolution);
      double r1_rx2_err = std::fmod(sdr1_rx2, resolution);
      double r2_rx2_err = std::fmod(sdr2_rx2, resolution);

      // TX and RX Time Vectors
      std::vector<double> sdr1_tx_times = {sdr1_tx1};
      std::vector<double> sdr2_tx_times = {sdr2_tx1};
      std::vector<double> sdr3_tx_times = {sdr3_tx1};

      std::vector<double> sdr1_rx_times = {sdr1_rx1, sdr1_rx2};
      std::vector<double> sdr2_rx_times = {sdr2_rx1, sdr2_rx2};
      std::vector<double> sdr3_rx_times = {sdr3_rx1, sdr3_rx2};

      // TX and RX Errors due to time resolution for FFT-based Fractionally Delaying the Signals
      std::vector<double> sdr1_tx_times_err = {-r1_tx1_err};
      std::vector<double> sdr2_tx_times_err = {-r2_tx1_err};
      std::vector<double> sdr3_tx_times_err = {-r3_tx1_err};

      std::vector<double> sdr1_rx_times_err = {-r1_rx1_err, -r1_rx2_err};
      std::vector<double> sdr2_rx_times_err = {-r2_rx1_err, -r2_rx2_err};
      std::vector<double> sdr3_rx_times_err = {-r3_rx1_err, -r3_rx2_err};

      // Threads
      cb_sdr1_tx_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_all, this, usrp_1, tx1_stream, sdr1_tx_times, sdr1_tx_times_err, 1);

      cb_sdr1_rx_thread = gr::thread::thread(
          &usrp_radar_all_impl::receive_all,
          this, usrp_1, rx1_stream, sdr1_rx_times, sdr1_rx_times_err, 1);

      cb_sdr2_tx_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_all, this, usrp_2, tx2_stream, sdr2_tx_times, sdr2_tx_times_err, 2);

      cb_sdr2_rx_thread = gr::thread::thread(
          &usrp_radar_all_impl::receive_all,
          this, usrp_2, rx2_stream, sdr2_rx_times, sdr2_rx_times_err, 2);

      cb_sdr3_tx_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_all, this, usrp_3, tx3_stream, sdr3_tx_times, sdr3_tx_times_err, 3);

      cb_sdr3_rx_thread = gr::thread::thread(
          &usrp_radar_all_impl::receive_all,
          this, usrp_3, rx3_stream, sdr3_rx_times, sdr3_rx_times_err, 3);

      cb_sdr1_tx_thread.join();
      cb_sdr2_rx_thread.join();
      cb_sdr2_tx_thread.join();
      cb_sdr1_rx_thread.join();
      cb_sdr3_tx_thread.join();
      cb_sdr3_rx_thread.join();

      GR_LOG_INFO(d_logger, "Bias TX/RX sequence completed.");
    }

    void usrp_radar_all_impl::cp_run()
    {
      uhd::rx_streamer::sptr rx1_stream, rx2_stream, rx3_stream;
      uhd::tx_streamer::sptr tx1_stream, tx2_stream, tx3_stream;
      setup_streamers(rx1_stream, rx2_stream, rx3_stream, tx1_stream, tx2_stream, tx3_stream);

      // TX and RX Times  **REMOVE WAIT_TIME or MAKE SMALLER??????
      double sdr1_tx1 = (start_delay * 4 + TDMA_time2 - wdelay_tx1 + cb1_est) / cd1_est;
      double sdr2_rx1 = (start_delay * 4 + TDMA_time2 - wait_time2 - wdelay_rx2 + cb2_est) / cd2_est;
      double sdr3_rx1 = (start_delay * 4 + TDMA_time2 - wait_time2 - wdelay_rx3 + cb3_est) / cd3_est;

      double sdr2_tx1 = (start_delay * 4 + TDMA_time2 * 2 - wdelay_tx2 + cb2_est) / cd2_est;
      double sdr1_rx1 = (start_delay * 4 + TDMA_time2 * 2 - wait_time2 - wdelay_rx1 + cb1_est) / cd1_est;
      double sdr3_rx2 = (start_delay * 4 + TDMA_time2 * 2 - wait_time2 - wdelay_rx3 + cb3_est) / cd3_est;

      double sdr3_tx1 = (start_delay * 4 + TDMA_time2 * 3 - wdelay_tx3 + cb3_est) / cd3_est;
      double sdr1_rx2 = (start_delay * 4 + TDMA_time2 * 3 - wait_time2 - wdelay_rx1 + cb1_est) / cd1_est;
      double sdr2_rx2 = (start_delay * 4 + TDMA_time2 * 3 - wait_time2 - wdelay_rx2 + cb2_est) / cd2_est;

      std::cout << std::fixed << std::setprecision(12)
                << "sdr1_tx1 = " << sdr1_tx1 << "\n"
                << "sdr2_rx1 = " << sdr2_rx1 << "\n"
                << "sdr3_rx1 = " << sdr3_rx1 << "\n\n"

                << "sdr2_tx1 = " << sdr2_tx1 << "\n"
                << "sdr1_rx1 = " << sdr1_rx1 << "\n"
                << "sdr3_rx2 = " << sdr3_rx2 << "\n\n"

                << "sdr3_tx1 = " << sdr3_tx1 << "\n"
                << "sdr1_rx2 = " << sdr1_rx2 << "\n"
                << "sdr2_rx2 = " << sdr2_rx2 << "\n"
                << std::endl;

      // Rounded Down nearest time resolution
      double resolution = 1 / sdr1_rate;
      double r1_tx1_err = std::fmod(sdr1_tx1, resolution);
      double r2_rx1_err = std::fmod(sdr2_rx1, resolution);
      double r3_rx1_err = std::fmod(sdr3_rx1, resolution);

      double r2_tx1_err = std::fmod(sdr2_tx1, resolution);
      double r1_rx1_err = std::fmod(sdr1_rx1, resolution);
      double r3_rx2_err = std::fmod(sdr3_rx2, resolution);

      double r3_tx1_err = std::fmod(sdr3_tx1, resolution);
      double r1_rx2_err = std::fmod(sdr1_rx2, resolution);
      double r2_rx2_err = std::fmod(sdr2_rx2, resolution);

      // TX and RX Time Vectors
      std::vector<double> sdr1_tx_times = {sdr1_tx1};
      std::vector<double> sdr2_tx_times = {sdr2_tx1};
      std::vector<double> sdr3_tx_times = {sdr3_tx1};

      std::vector<double> sdr1_rx_times = {sdr1_rx1, sdr1_rx2};
      std::vector<double> sdr2_rx_times = {sdr2_rx1, sdr2_rx2};
      std::vector<double> sdr3_rx_times = {sdr3_rx1, sdr3_rx2};

      // TX and RX Errors due to time resolution for FFT-based Fractionally Delaying the Signals
      std::vector<double> sdr1_tx_times_err = {-r1_tx1_err};
      std::vector<double> sdr2_tx_times_err = {-r2_tx1_err};
      std::vector<double> sdr3_tx_times_err = {-r3_tx1_err};

      std::vector<double> sdr1_rx_times_err = {-r1_rx1_err, -r1_rx2_err};
      std::vector<double> sdr2_rx_times_err = {-r2_rx1_err, -r2_rx2_err};
      std::vector<double> sdr3_rx_times_err = {-r3_rx1_err, -r3_rx2_err};

      // Threads
      cp_sdr1_tx_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_all, this, usrp_1, tx1_stream, sdr1_tx_times, sdr1_tx_times_err, 1);

      cp_sdr1_rx_thread = gr::thread::thread(
          &usrp_radar_all_impl::receive_all,
          this, usrp_1, rx1_stream, sdr1_rx_times, sdr1_rx_times_err, 1);

      cp_sdr2_tx_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_all, this, usrp_2, tx2_stream, sdr2_tx_times, sdr2_tx_times_err, 2);

      cp_sdr2_rx_thread = gr::thread::thread(
          &usrp_radar_all_impl::receive_all,
          this, usrp_2, rx2_stream, sdr2_rx_times, sdr2_rx_times_err, 2);

      cp_sdr3_tx_thread = gr::thread::thread(
          &usrp_radar_all_impl::transmit_all, this, usrp_3, tx3_stream, sdr3_tx_times, sdr3_tx_times_err, 3);

      cp_sdr3_rx_thread = gr::thread::thread(
          &usrp_radar_all_impl::receive_all,
          this, usrp_3, rx3_stream, sdr3_rx_times, sdr3_rx_times_err, 3);

      cp_sdr1_tx_thread.join();
      cp_sdr2_rx_thread.join();
      cp_sdr2_tx_thread.join();
      cp_sdr1_rx_thread.join();
      cp_sdr3_tx_thread.join();
      cp_sdr3_rx_thread.join();

      GR_LOG_INFO(d_logger, "Final TX/RX sequence completed.");
    }

    void usrp_radar_all_impl::transmit_bursts(uhd::usrp::multi_usrp::sptr usrp_tx,
                                              uhd::tx_streamer::sptr tx_stream,
                                              double start_time, double tx_time_err, int sdr_id)
    {
      uhd::tx_metadata_t md;
      pmt::pmt_t selected_data = pmt::PMT_NIL;
      pmt::pmt_t selected_meta = pmt::PMT_NIL;

      // Update intermediate data
      if (waveform1_ready)
      {
        updated_data1 = tx_data_sdr1;
        waveform1_ready = false;
      }
      if (waveform2_ready)
      {
        updated_data2 = tx_data_sdr2;
        waveform2_ready = false;
      }
      if (waveform3_ready)
      {
        updated_data3 = tx_data_sdr3;
        waveform3_ready = false;
      }

      // Update transmit vector for SDR
      switch (sdr_id)
      {
      case 1:
      {
        selected_data = updated_data1;
        selected_meta = meta_sdr1;
        break;
      }
      case 2:
      {
        selected_data = updated_data2;
        selected_meta = meta_sdr2;
        break;
      }
      case 3:
      {
        selected_data = updated_data3;
        selected_meta = meta_sdr3;
        break;
      }
      }

      // Validate
      if (!pmt::is_c32vector(selected_data))
      {
        GR_LOG_ERROR(d_logger, "TX data is not a c32vector for SDR " + std::to_string(sdr_id));
        return;
      }

      // Extract vector
      size_t len = 0;
      const gr_complex *raw = pmt::c32vector_elements(selected_data, len);
      if (!raw || len == 0)
      {
        GR_LOG_ERROR(d_logger, "Invalid TX data for SDR " + std::to_string(sdr_id));
        return;
      }

      double rx_time_secs = usrp_tx->get_time_now().get_real_secs();
      pmt::pmt_t rx_time = pmt::from_double(rx_time_secs);
      std::cout << "USRP TX Time: " << rx_time_secs << std::endl;

      // Transmit Data Vector
      std::vector<gr_complex> tx_data_vector(raw, raw + len);

      // Fractional Delay via FFT Domian
      af::array x = af::array(len, reinterpret_cast<const af::cfloat *>(raw), afHost);
      // FFT
      af::array X = af::fft(x);
      X = ::plasma::fftshift(X, 0);
      // Build frequency vector
      af::array f = (-sdr1_rate / 2.0) + ((af::seq(0, len - 1)) * (sdr1_rate / len));
      // Apply Delay and IFFT
      af::array X_delay = X * af::exp(-1.0 * af::Im * 2.0 * M_PI * f * (tx_time_err));
      X_delay = ::plasma::ifftshift(X_delay, 0);
      af::array x_delay = af::ifft(X_delay);

      // Copy back into tx_data_vector
      x_delay.host(reinterpret_cast<af::cfloat *>(tx_data_vector.data()));
      if (tx_buffs.size() < 1)
        tx_buffs.resize(1);
      tx_buffs[0] = tx_data_vector;

      // Populate metadata
      md.start_of_burst = true;
      md.end_of_burst = true;
      md.has_time_spec = true;

      long long ticks_req = (long long)std::floor(start_time * sdr1_rate);
      // double t_sched = double(ticks_req) / sdr1_rate;

      // std::cout << std::fixed << std::setprecision(12)
      //           << "tx time = " << t_sched << "\n";
      auto tspec = uhd::time_spec_t::from_ticks(ticks_req, sdr1_rate);
      md.time_spec = tspec;

      // md.time_spec = uhd::time_spec_t(start_time);

      double timeout = 0.0;
      tx_stream->send(tx_buffs[0].data(), tx_buffs[0].size(), md, timeout);
    }

    void usrp_radar_all_impl::receive(uhd::usrp::multi_usrp::sptr usrp_rx,
                                      uhd::rx_streamer::sptr rx_stream,
                                      double start_time, double rx_time_error, int sdr_rx)
    {
      // Setup variables
      uhd::rx_metadata_t md;

      // Total samples to receive based on capture time
      if (carrier_phase_enabled)
        total_samps_to_rx = cap_length2 * sdr1_rate; // sdr1_rate shoulld be the same as sdr2,3,.....N
      else
        total_samps_to_rx = cap_length * sdr1_rate; // sdr1_rate shoulld be the same as sdr2,3,.....N

      size_t samps_received = 0;

      // Set up UHD receive mode
      uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
      cmd.num_samps = total_samps_to_rx;
      cmd.stream_now = usrp_rx->get_time_now().get_real_secs() >= start_time;
      // cmd.stream_now = false;


      ////////////////////////////////////////////////////////////////////
      double rx_time_secs = usrp_rx->get_time_now().get_real_secs();
      pmt::pmt_t rx_time = pmt::from_double(rx_time_secs);
      std::cout << "USRP RX Time: " << rx_time_secs << std::endl;
      ////////////////////////////////////////////////////////////////////

      // cmd.time_spec = uhd::time_spec_t(start_time);
      long long ticks_req = (long long)std::floor(start_time * sdr1_rate);
      // double t_sched = double(ticks_req) / sdr1_rate;
      // std::cout << std::fixed << std::setprecision(12)
      //           << "rx time = " << t_sched << "\n";
      auto tspec = uhd::time_spec_t::from_ticks(ticks_req, sdr1_rate);
      // double fractional_sec = start_time - t_sched;
      cmd.time_spec = tspec;

      rx_stream->issue_stream_cmd(cmd);

      // Time Error
      pmt::pmt_t rx_error_pmt = pmt::from_double(rx_time_error);

      // Allocate buffer
      pmt::pmt_t rx_data_pmt = pmt::make_c32vector(total_samps_to_rx, 0);
      gr_complex *rx_data_ptr = pmt::c32vector_writable_elements(rx_data_pmt, total_samps_to_rx);

      // Tells USRP to wait for incoming samples before giving up
      double timeout = 0.0;

      // Counts number of samples received until end of capture time
      while (samps_received < total_samps_to_rx)
      {
        size_t samps_to_recv = total_samps_to_rx - samps_received;
        size_t num_rx = rx_stream->recv(rx_data_ptr + samps_received, samps_to_recv, md, timeout);

        samps_received += num_rx;
      }

      // Assign RX Time metadata
      this->meta = pmt::dict_add(
          this->meta, pmt::intern("rx_error"), rx_error_pmt);

      // Choose the correct output port
      pmt::pmt_t output_msg = pmt::cons(this->meta, rx_data_pmt);
      if (clock_drift_enabled && !clock_bias_enabled)
      {
        // GR_LOG_INFO(d_logger, "CLOCK DRIFT OUT" );
        switch (sdr_rx)
        {
        case 1:
          message_port_pub(PMT_HARMONIA_CD_OUT, output_msg);
          break;
        case 2:
          message_port_pub(PMT_HARMONIA_CD_OUT2, output_msg);
          break;
        case 3:
          message_port_pub(PMT_HARMONIA_CD_OUT3, output_msg);
          break;
        }
      }
      else if (clock_drift_enabled && clock_bias_enabled)
      {
        // GR_LOG_INFO(d_logger, "CLOCK BIAS OUT" );
        switch (sdr_rx)
        {
        case 1:
          message_port_pub(PMT_HARMONIA_CB_OUT, output_msg);
          break;
        case 2:
          message_port_pub(PMT_HARMONIA_CB_OUT2, output_msg);
          break;
        case 3:
          message_port_pub(PMT_HARMONIA_CB_OUT3, output_msg);
          break;
        }
      }
      else if (clock_drift_enabled && clock_bias_enabled && carrier_phase_enabled)
      {
        // GR_LOG_INFO(d_logger, "CLOCK BIAS OUT" );
        switch (sdr_rx)
        {
        case 1:
          message_port_pub(PMT_HARMONIA_CP_OUT, output_msg);
          break;
        case 2:
          message_port_pub(PMT_HARMONIA_CP_OUT2, output_msg);
          break;
        case 3:
          message_port_pub(PMT_HARMONIA_CP_OUT3, output_msg);
          break;
        }
      }
      else
      {
        // GR_LOG_INFO(d_logger, "ST OUT" );
        switch (sdr_rx)
        {
        case 1:
          message_port_pub(PMT_HARMONIA_OUT, output_msg);
          break;
        case 2:
          message_port_pub(PMT_HARMONIA_OUT2, output_msg);
          break;
        case 3:
          message_port_pub(PMT_HARMONIA_OUT3, output_msg);
          break;
        }
      }

      // Reset metadata
      this->meta = pmt::make_dict();
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
