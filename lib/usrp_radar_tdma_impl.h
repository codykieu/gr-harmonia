/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_USRP_RADAR_TDMA_IMPL_H
#define INCLUDED_HARMONIA_USRP_RADAR_TDMA_IMPL_H

#include <gnuradio/harmonia/pmt_constants.h>
#include <gnuradio/harmonia/usrp_radar_tdma.h>
#include <arrayfire.h>
#include <plasma_dsp/pulsed_waveform.h>
#include <plasma_dsp/fft.h>
#include <nlohmann/json.hpp>
#include <uhd/convert.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/thread/thread.hpp>
#include <fstream>
#include <queue>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace gr
{
  namespace harmonia
  {

    class usrp_radar_tdma_impl : public usrp_radar_tdma
    {
    private:
      // Block params
      uhd::usrp::multi_usrp::sptr usrp;
      std::string usrp_args_sdr;
      double sdr_rate;
      double sdr_freq;
      double sdr_gain;
      int sdr_id;
      double start_delay, cap_length, cap_length2;
      double wait_time;
      double TDMA_time, TDMA_time2;
      std::vector<size_t> sdr_channel_nums;
      std::string sdr_subdev;
      std::string sdr_device_addr;
      std::string sdr_cpu_format;
      std::string sdr_otw_format;

      // Clock Drift/Bias Params
      bool clock_drift_enabled = false;
      bool clock_bias_enabled = false;
      bool cd_run_executed = false;
      bool cb_run_executed = false;
      double cd_est;
      double cb_est;
      double alpha, phi;
      bool cd_ready = false;
      bool cb_ready = false;
      bool waveform_ready = false;
      pmt::pmt_t updated_data = pmt::PMT_NIL;

      // Implementation params
      gr::thread::thread main_thread;
      gr::thread::thread sdr_tx_thread;
      gr::thread::thread sdr_rx_thread;

      gr::thread::thread cd_sdr_tx_thread;
            gr::thread::thread cd_sdr_rx_thread;

      gr::thread::thread cb_sdr_tx_thread;
            gr::thread::thread cb_sdr_rx_thread;

      pmt::pmt_t tx_data_sdr;
      pmt::pmt_t meta_sdr;
      std::vector<std::vector<gr_complex>> tx_buffs;
      size_t tx_buff_size, rx_buff_size;
      size_t n_tx_total;
      pmt::pmt_t tx_data;
      pmt::pmt_t meta;

      void config_usrp(uhd::usrp::multi_usrp::sptr &usrp,
                       const std::string &args,
                       const double sdr_rate,
                       const double sdr_freq,
                       const double sdr_gain,
                       const std::string &sdr_subdev);

      void setup_streamers(
          uhd::tx_streamer::sptr &tx_stream,
          uhd::rx_streamer::sptr &rx_stream);

      void transmit_bursts(uhd::usrp::multi_usrp::sptr usrp_tx,
                           uhd::tx_streamer::sptr tx_stream,
                           double start_time, double tx_time_error);

      void receive(uhd::usrp::multi_usrp::sptr usrp_rx,
                   uhd::rx_streamer::sptr rx_stream,
                   double start_time, double rx_time_error);

      void transmit_all(
          uhd::usrp::multi_usrp::sptr usrp,
          uhd::tx_streamer::sptr tx_stream,
          const std::vector<double> &times,
          const std::vector<double> &tx_time_error);

      void receive_all(
          uhd::usrp::multi_usrp::sptr usrp,
          uhd::rx_streamer::sptr rx_stream,
          const std::vector<double> &times,
          const std::vector<double> &rx_time_error);

    public:
      usrp_radar_tdma_impl(const std::string &args,
                           const double sdr_rate,
                           const double sdr_freq,
                           const double sdr_gain,
                           const int sdr_id,
                           const double start_delay,
                           const double cap_length,
                           const double cap_length2,
                           const double wait_time,
                           const double TDMA_time,
                           const double TDMA_time2);
      ~usrp_radar_tdma_impl();
      void run();
      void check_cd_ready();
      void check_cb_ready();
      void cd_run();
      void cb_run();
      bool start() override;
      bool stop() override;
      void handle_message(const pmt::pmt_t &msg);
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_USRP_RADAR_TDMA_IMPL_H */
