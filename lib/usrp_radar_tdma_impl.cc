/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "usrp_radar_tdma_impl.h"
#include <gnuradio/io_signature.h>

namespace gr
{
  namespace harmonia
  {

    usrp_radar_tdma::sptr usrp_radar_tdma::make(const std::string &args,
                                                const double sdr_rate,
                                                const double sdr_freq,
                                                const double sdr_gain,
                                                const int sdr_id,
                                                const double start_delay,
                                                const double cap_length,
                                                const double cap_length2,
                                                const double wait_time,
                                                const double TDMA_time,
                                                const double TDMA_time2)
    {
      return gnuradio::make_block_sptr<usrp_radar_tdma_impl>(args,
                                                             sdr_rate,
                                                             sdr_freq,
                                                             sdr_gain,
                                                             sdr_id,
                                                             start_delay,
                                                             cap_length,
                                                             cap_length2,
                                                             wait_time,
                                                             TDMA_time,
                                                             TDMA_time2);
    }

    /*
     * The private constructor
     */
    usrp_radar_tdma_impl::usrp_radar_tdma_impl(const std::string &args,
                                               const double sdr_rate,
                                               const double sdr_freq,
                                               const double sdr_gain,
                                               const int sdr_id,
                                               const double start_delay,
                                               const double cap_length,
                                               const double cap_length2,
                                               const double wait_time,
                                               const double TDMA_time,
                                               const double TDMA_time2)
        : gr::block("usrp_radar_tdma",
                    gr::io_signature::make(0, 0, 0),
                    gr::io_signature::make(0, 0, 0)),
          usrp_args_sdr(args),
          sdr_rate(sdr_rate),
          sdr_freq(sdr_freq),
          sdr_gain(sdr_gain),
          sdr_id(sdr_id),
          start_delay(start_delay),
          cap_length(cap_length),
          cap_length2(cap_length2),
          wait_time(wait_time),
          TDMA_time(TDMA_time),
          TDMA_time2(TDMA_time2)
    {
      this->sdr_subdev = "";
      this->sdr_cpu_format = "fc32";
      this->sdr_otw_format = "sc16";
      this->sdr_device_addr = "";
      this->sdr_channel_nums = std::vector<size_t>(1, 0);
      this->tx_buffs = std::vector<std::vector<gr_complex>>(sdr_channel_nums.size());

      this->n_tx_total = 0;
      this->meta = pmt::make_dict();

      config_usrp(this->usrp,
                  this->usrp_args_sdr,
                  this->sdr_rate,
                  this->sdr_freq,
                  this->sdr_gain,
                  this->sdr_subdev);

      // Input Ports
      message_port_register_in(PMT_HARMONIA_IN);
      message_port_register_in(PMT_HARMONIA_LFM_IN);

      // Output Ports
      message_port_register_out(PMT_HARMONIA_OUT);
      message_port_register_out(PMT_HARMONIA_CD_OUT);
      message_port_register_out(PMT_HARMONIA_CB_OUT);

      message_port_register_in(PMT_HARMONIA_IN);
      set_msg_handler(PMT_HARMONIA_IN, [this](pmt::pmt_t msg)
                      { this->handle_message(msg); });

      message_port_register_in(PMT_HARMONIA_LFM_IN);
      set_msg_handler(PMT_HARMONIA_LFM_IN, [this](pmt::pmt_t msg)
                      { this->handle_message(msg); });
    }

    /*
     * Our virtual destructor.
     */
    usrp_radar_tdma_impl::~usrp_radar_tdma_impl() {}

    bool usrp_radar_tdma_impl::start()
    {
      main_thread = gr::thread::thread(&usrp_radar_tdma_impl::run, this);
      return block::start();
    }

    bool usrp_radar_tdma_impl::stop()
    {
      return block::stop();
    }

    void usrp_radar_tdma_impl::config_usrp(uhd::usrp::multi_usrp::sptr &usrp,
                                           const std::string &args,
                                           const double sdr_rate,
                                           const double sdr_freq,
                                           const double sdr_gain,
                                           const std::string &sdr_subdev)
    {
      usrp = uhd::usrp::multi_usrp::make(args);
      if (not sdr_subdev.empty())
      {
        usrp->set_tx_subdev_spec(sdr_subdev);
      }
      usrp->set_tx_rate(sdr_rate);
      usrp->set_tx_freq(sdr_freq);
      usrp->set_tx_gain(sdr_gain);
      usrp->set_rx_rate(sdr_rate);
      usrp->set_rx_freq(sdr_freq);
      usrp->set_rx_gain(sdr_gain);

      // Sets USRPs Clock Source
      usrp->set_clock_source("internal");

      // Sets USRP Time to 0.0 ***ONLY FOR COARSE SYNCHRONIZATION
      usrp->set_time_now(uhd::time_spec_t(0.0));
    }

    void usrp_radar_tdma_impl::setup_streamers(
        uhd::tx_streamer::sptr &tx_stream,
        uhd::rx_streamer::sptr &rx_stream)
    {
      /***********************************************************************
       * Transmit thread
       **********************************************************************/
      uhd::stream_args_t tx_args(sdr_cpu_format, sdr_otw_format);
      tx_args.channels = sdr_channel_nums;
      tx_args.args = uhd::device_addr_t(sdr_device_addr);
      tx_stream = usrp->get_tx_stream(tx_args);

      /***********************************************************************
       * Receive thread
       **********************************************************************/
      uhd::stream_args_t rx_args(sdr_cpu_format, sdr_otw_format);
      rx_args.channels = sdr_channel_nums;
      rx_args.args = uhd::device_addr_t(sdr_device_addr);
      rx_stream = usrp->get_rx_stream(rx_args);
    }

    void usrp_radar_tdma_impl::handle_message(const pmt::pmt_t &msg)
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
      // GR_LOG_INFO(d_logger,
      //   std::string("clock_drift_enabled=") + (clock_drift_enabled?"true":"false")
      //   + ", clock_bias_enabled=" + (clock_bias_enabled?"true":"false"));

      // Update TX Data with New Waveform
      if (pmt::is_symbol(label))
      {
        if (pmt::equal(label, pmt::intern("single_tone")) && !clock_drift_enabled)
        {
          GR_LOG_INFO(d_logger, "WAVEFORM UPDATED (single_tone, no drift)");
          tx_data_sdr = data;
          meta_sdr = meta;
          waveform_ready = true;
        }
        else if (pmt::equal(label, pmt::intern("LFM")) && clock_drift_enabled && !clock_bias_enabled)
        {
          GR_LOG_INFO(d_logger, "WAVEFORM UPDATED (LFM, drift-enabled)");
          tx_data_sdr = data;
          meta_sdr = meta;
          waveform_ready = true;
        }
        else if (pmt::equal(label, pmt::intern("LFM")) && clock_bias_enabled)
        {
          GR_LOG_INFO(d_logger, "WAVEFORM UPDATED (LFM, bias-enabled)");
          tx_data_sdr = data;
          meta_sdr = meta;
          waveform_ready = true;
        }
      }

      // Clock Drift Logic
      if (clock_drift_enabled)
      {
        pmt::pmt_t pmt_cd1 = pmt::dict_ref(meta, PMT_HARMONIA_SDR1, pmt::PMT_NIL);
        pmt::pmt_t pmt_cd2 = pmt::dict_ref(meta, PMT_HARMONIA_SDR2, pmt::PMT_NIL);
        pmt::pmt_t pmt_cd3 = pmt::dict_ref(meta, PMT_HARMONIA_SDR3, pmt::PMT_NIL);

        if (pmt::is_number(pmt_cd1) && sdr_id == 1)
        {
          cd_est = pmt::to_double(pmt_cd1);
          cd_ready = true;
        }
        if (pmt::is_number(pmt_cd2) && sdr_id == 2)
        {
          cd_est = pmt::to_double(pmt_cd2);
          cd_ready = true;
        }
        if (pmt::is_number(pmt_cd3) && sdr_id == 3)
        {
          cd_est = pmt::to_double(pmt_cd3);
          cd_ready = true;
        }
      }

      // Clock Bias Logic
      if (clock_bias_enabled)
      {
        pmt::pmt_t pmt_cb1 = pmt::dict_ref(meta, PMT_HARMONIA_CB_SDR1, pmt::PMT_NIL);
        pmt::pmt_t pmt_cb2 = pmt::dict_ref(meta, PMT_HARMONIA_CB_SDR2, pmt::PMT_NIL);
        pmt::pmt_t pmt_cb3 = pmt::dict_ref(meta, PMT_HARMONIA_CB_SDR3, pmt::PMT_NIL);

        if (pmt::is_number(pmt_cb1) && sdr_id == 1)
        {
          cb_est = pmt::to_double(pmt_cb1);
          cb_ready = true;
        }
        if (pmt::is_number(pmt_cb2) && sdr_id == 2)
        {
          cb_est = pmt::to_double(pmt_cb2);
          cb_ready = true;
        }
        if (pmt::is_number(pmt_cb3) && sdr_id == 3)
        {
          cb_est = pmt::to_double(pmt_cb3);
          cb_ready = true;
        }
      }

      check_cd_ready();
      check_cb_ready();
    }

    void usrp_radar_tdma_impl::check_cd_ready()
    {
      if (cd_ready && waveform_ready && clock_drift_enabled && !cd_run_executed)
      {
        cd_run_executed = true;
        GR_LOG_INFO(d_logger, "Clock drift enabled. Launching cd_run.");
        cd_run();
      }
    }

    void usrp_radar_tdma_impl::check_cb_ready()
    {
      if (cb_ready && waveform_ready && clock_bias_enabled && cd_run_executed && !cb_run_executed)
      {
        cb_run_executed = true;
        GR_LOG_INFO(d_logger, "Clock bias enabled. Launching cb_run.");
        cb_run();
      }
    }

    void usrp_radar_tdma_impl::transmit_all(
        uhd::usrp::multi_usrp::sptr usrp,
        uhd::tx_streamer::sptr tx_stream,
        const std::vector<double> &times,
        const std::vector<double> &tx_time_error)
    {
      for (size_t i = 0; i < times.size(); ++i)
      {
        this->transmit_bursts(usrp, tx_stream, times[i], tx_time_error[i]);
      }
    }

    void usrp_radar_tdma_impl::receive_all(
        uhd::usrp::multi_usrp::sptr usrp,
        uhd::rx_streamer::sptr rx_stream,
        const std::vector<double> &times,
        const std::vector<double> &rx_time_error)
    {
      for (size_t i = 0; i < times.size(); ++i)
      {
        this->receive(usrp, rx_stream, times[i], rx_time_error[i]);
      }
    }

    void usrp_radar_tdma_impl::run()
    {
      uhd::tx_streamer::sptr tx_stream;
      uhd::rx_streamer::sptr rx_stream;

      setup_streamers(tx_stream, rx_stream);

      double sdr_begin = usrp->get_time_now().get_real_secs();

      std::cout << std::fixed << std::setprecision(12)
                << "[SDR] Start Time: " << sdr_begin << "SDR ID: " << sdr_id << std::endl;

      // Transmit Time
      double sdr_tx1 = start_delay + TDMA_time * double(sdr_id);

      // Receive Time
      double sdr_rx1;
      double sdr_rx2;

      if (sdr_id == 1)
      {
        sdr_rx1 = start_delay + TDMA_time * 2 - wait_time;
        sdr_rx2 = start_delay + TDMA_time * 3 - wait_time;
      }
      else if (sdr_id == 2)
      {
        sdr_rx1 = start_delay + TDMA_time - wait_time;
        sdr_rx2 = start_delay + TDMA_time * 3 - wait_time;
      }
      else if (sdr_id == 3)
      {
        sdr_rx1 = start_delay + TDMA_time - wait_time;
        sdr_rx2 = start_delay + TDMA_time * 2 - wait_time;
      }

      // Rounded Down Version
      double resolution = 1 / sdr_rate; // 10 ns
      double tx1_err = std::fmod(sdr_tx1, resolution);
      double rx1_err = std::fmod(sdr_rx1, resolution);
      double rx2_err = std::fmod(sdr_rx2, resolution);

      // Threads
      std::vector<double> sdr_tx_times = {sdr_tx1 - tx1_err};
      std::vector<double> sdr_tx_times_err = {tx1_err};
      std::vector<double> sdr_rx_times = {sdr_rx1 - rx1_err, sdr_rx2 - rx2_err};
      std::vector<double> sdr_rx_times_err = {rx1_err, rx2_err};

      sdr_tx_thread = gr::thread::thread(
          &usrp_radar_tdma_impl::transmit_all, this, usrp, tx_stream, sdr_tx_times, sdr_tx_times_err);
      sdr_tx_thread.join();

      sdr_rx_thread = gr::thread::thread(
          &usrp_radar_tdma_impl::receive_all,
          this, usrp, rx_stream, sdr_rx_times, sdr_rx_times_err);
      sdr_rx_thread.join();
    }

    void usrp_radar_tdma_impl::cd_run()
    {
      uhd::tx_streamer::sptr tx_stream;
      uhd::rx_streamer::sptr rx_stream;
      setup_streamers(tx_stream, rx_stream);

      // Transmit Time
      double sdr_tx1 = (start_delay * 2 + TDMA_time * double(sdr_id)) / cd_est;

      // Receive Time
      double wdelay_rx = 0;
      double sdr_rx1;
      double sdr_rx2;
      if (sdr_id == 1)
      {
        sdr_rx1 = (start_delay * 2 + TDMA_time * 2 - wait_time) / cd_est;
        sdr_rx2 = (start_delay * 2 + TDMA_time * 3 - wait_time) / cd_est;
      }
      else if (sdr_id == 2)
      {
        sdr_rx1 = (start_delay * 2 + TDMA_time - wait_time) / cd_est;
        sdr_rx2 = (start_delay * 2 + TDMA_time * 3 - wait_time) / cd_est;
      }
      else if (sdr_id == 3)
      {
        sdr_rx1 = (start_delay * 2 + TDMA_time - wait_time) / cd_est;
        sdr_rx2 = (start_delay * 2 + TDMA_time * 2 - wait_time) / cd_est;
      }

      // Rounded Down Version
      double resolution = 1 / sdr_rate; // 10 ns
      double tx1_err = std::fmod(sdr_tx1, resolution);
      double rx1_err = std::fmod(sdr_rx1, resolution);
      double rx2_err = std::fmod(sdr_rx2, resolution);

      // Threads
      std::vector<double> sdr_tx_times = {sdr_tx1};
      std::vector<double> sdr_tx_times_err = {tx1_err};
      std::vector<double> sdr_rx_times = {sdr_rx1, sdr_rx2};
      std::vector<double> sdr_rx_times_err = {wdelay_rx, wdelay_rx};

      cd_sdr_tx_thread = gr::thread::thread(
          &usrp_radar_tdma_impl::transmit_all, this, usrp, tx_stream, sdr_tx_times, sdr_tx_times_err);
      cd_sdr_tx_thread.join();

      cd_sdr_rx_thread = gr::thread::thread(
          &usrp_radar_tdma_impl::receive_all,
          this, usrp, rx_stream, sdr_rx_times, sdr_rx_times_err);
      cd_sdr_rx_thread.join();

      GR_LOG_INFO(d_logger, "Drift TX/RX sequence completed.");
    }

    void usrp_radar_tdma_impl::cb_run()
    {
      uhd::tx_streamer::sptr tx_stream;
      uhd::rx_streamer::sptr rx_stream;
      setup_streamers(tx_stream, rx_stream);

      // Transmit Time
      double sdr_tx1 = (start_delay * 3 + (TDMA_time * double(sdr_id)) + cb_est) / cd_est;

      // Receive Time
      double wdelay_rx = 0;
      double sdr_rx1;
      double sdr_rx2;
      if (sdr_id == 1)
      {
        sdr_rx1 = (start_delay * 2 + TDMA_time * 2 - wait_time + cb_est) / cd_est;
        sdr_rx2 = (start_delay * 2 + TDMA_time * 3 - wait_time + cb_est) / cd_est;
      }
      else if (sdr_id == 2)
      {
        sdr_rx1 = (start_delay * 2 + TDMA_time - wait_time + cb_est) / cd_est;
        sdr_rx2 = (start_delay * 2 + TDMA_time * 3 - wait_time + cb_est) / cd_est;
      }
      else if (sdr_id == 3)
      {
        sdr_rx1 = (start_delay * 2 + TDMA_time - wait_time + cb_est) / cd_est;
        sdr_rx2 = (start_delay * 2 + TDMA_time * 2 - wait_time + cb_est) / cd_est;
      }

      // Rounded Down Version
      double resolution = 1 / sdr_rate; // 10 ns
      double tx1_err = std::fmod(sdr_tx1, resolution);
      double rx1_err = std::fmod(sdr_rx1, resolution);
      double rx2_err = std::fmod(sdr_rx2, resolution);

      // Threads
      std::vector<double> sdr_tx_times = {sdr_tx1};
      std::vector<double> sdr_tx_times_err = {tx1_err};
      std::vector<double> sdr_rx_times = {sdr_rx1, sdr_rx2};
      std::vector<double> sdr_rx_times_err = {wdelay_rx, wdelay_rx};

      cb_sdr_tx_thread = gr::thread::thread(
          &usrp_radar_tdma_impl::transmit_all, this, usrp, tx_stream, sdr_tx_times, sdr_tx_times_err);
      cb_sdr_tx_thread.join();

      cb_sdr_rx_thread = gr::thread::thread(
          &usrp_radar_tdma_impl::receive_all,
          this, usrp, rx_stream, sdr_rx_times, sdr_rx_times_err);
      cb_sdr_rx_thread.join();

      GR_LOG_INFO(d_logger, "Bias TX/RX sequence completed.");
    }

    void usrp_radar_tdma_impl::transmit_bursts(uhd::usrp::multi_usrp::sptr usrp_tx,
                                               uhd::tx_streamer::sptr tx_stream,
                                               double start_time, double tx_time_err)
    {
      uhd::tx_metadata_t md;
      pmt::pmt_t selected_data = pmt::PMT_NIL;
      pmt::pmt_t selected_meta = pmt::PMT_NIL;

      // Update intermediate data
      if (waveform_ready)
        updated_data = tx_data_sdr;

      selected_data = updated_data;
      selected_meta = meta_sdr;

      // Validate
      if (!pmt::is_c32vector(selected_data))
      {
        GR_LOG_ERROR(d_logger, "TX data is not a c32vector for SDR ");
        return;
      }

      // Extract vector
      size_t len = 0;
      const gr_complex *raw = pmt::c32vector_elements(selected_data, len);
      if (!raw || len == 0)
      {
        GR_LOG_ERROR(d_logger, "Invalid TX data for SDR ");
        return;
      }

      std::vector<gr_complex> tx_data_vector(raw, raw + len);

      // Convert into AF
      af::array x = af::array(len, reinterpret_cast<const af::cfloat *>(raw), afHost);
      // FFT
      af::array X = af::fft(x);
      X = ::plasma::fftshift(X, 0);
      // Build frequency vector
      af::array f = (-sdr_rate / 2.0) + ((af::seq(0, len - 1)) * (sdr_rate / len));
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
      md.time_spec = uhd::time_spec_t(start_time);

      double timeout = 0.0;
      tx_stream->send(tx_buffs[0].data(), tx_buffs[0].size(), md, timeout);
    }

    void usrp_radar_tdma_impl::receive(uhd::usrp::multi_usrp::sptr usrp_rx,
                                       uhd::rx_streamer::sptr rx_stream,
                                       double start_time, double rx_time_error)
    {
      // Setup variables
      uhd::rx_metadata_t md;

      // Total samples to receive based on capture time
      size_t total_samps_to_rx = cap_length * sdr_rate;
      size_t samps_received = 0;
      pmt::pmt_t rx_error_pmt = pmt::from_double(rx_time_error);

      // Set up UHD receive mode
      uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
      cmd.num_samps = total_samps_to_rx;
      cmd.stream_now = false;
      cmd.time_spec = uhd::time_spec_t(start_time);
      rx_stream->issue_stream_cmd(cmd);

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
        message_port_pub(PMT_HARMONIA_CD_OUT, output_msg);
      }
      else if (clock_drift_enabled && clock_bias_enabled)
      {
        message_port_pub(PMT_HARMONIA_CB_OUT, output_msg);
      }
      else
      {
        message_port_pub(PMT_HARMONIA_OUT, output_msg);
      }

      // Reset metadata
      this->meta = pmt::make_dict();
    }

  } /* namespace harmonia */
} /* namespace gr */
