/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_USRP_RADAR_ALL_IMPL_H
#define INCLUDED_HARMONIA_USRP_RADAR_ALL_IMPL_H

#include <gnuradio/harmonia/pmt_constants.h>
#include <gnuradio/harmonia/usrp_radar_all.h>
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

    class usrp_radar_all_impl : public usrp_radar_all
    {
      // Block params
      uhd::usrp::multi_usrp::sptr usrp_1, usrp_2, usrp_3;
      std::string usrp_args_sdr1, usrp_args_sdr2, usrp_args_sdr3;
      double sdr1_rate, sdr2_rate, sdr3_rate;
      double sdr1_freq, sdr2_freq, sdr3_freq;
      double sdr1_gain, sdr2_gain, sdr3_gain;
      double start_delay, cap_length;
      double wait_time;
      double TDMA_time;
      std::vector<size_t> sdr1_channel_nums, sdr2_channel_nums, sdr3_channel_nums;
      std::string sdr1_subdev, sdr2_subdev, sdr3_subdev;
      std::string sdr1_device_addr, sdr2_device_addr, sdr3_device_addr;
      std::string sdr1_cpu_format, sdr2_cpu_format, sdr3_cpu_format;
      std::string sdr1_otw_format, sdr2_otw_format, sdr3_otw_format;
      double prf;
      bool verbose;
      size_t n_delay;

      
      // Clock Drift/Bias Params
      bool clock_drift_enabled = false;
      bool clock_bias_enabled = false;
      bool cd_run_executed = false;
      bool cb_run_executed = false;
      double cd1_est, cd2_est, cd3_est;
      double cb1_est, cb2_est, cb3_est;
      double alpha, phi;
      bool cd1_ready = false;
      bool cd2_ready = false;
      bool cd3_ready = false;
      bool cb1_ready = false;
      bool cb2_ready = false;
      bool cb3_ready = false;
      bool waveform1_ready = false;
      bool waveform2_ready = false;
      bool waveform3_ready = false;
      bool cbwaveform1_ready = false;
      bool cbwaveform2_ready = false;
      bool cbwaveform3_ready = false;
      pmt::pmt_t updated_data1 = pmt::PMT_NIL;
      pmt::pmt_t updated_data2 = pmt::PMT_NIL;
      pmt::pmt_t updated_data3 = pmt::PMT_NIL;

      // Implementation params
      gr::thread::thread main_thread;
      gr::thread::thread tx1_thread;
      gr::thread::thread tx2_thread;
      gr::thread::thread tx3_thread;
      gr::thread::thread rx1_thread;
      gr::thread::thread rx2_thread;
      gr::thread::thread rx3_thread;

      gr::thread::thread sdr1_tx_thread;
      gr::thread::thread sdr2_tx_thread;
      gr::thread::thread sdr3_tx_thread;
      gr::thread::thread sdr1_rx_thread;
      gr::thread::thread sdr2_rx_thread;
      gr::thread::thread sdr3_rx_thread;

      gr::thread::thread cd_sdr1_tx_thread;
      gr::thread::thread cd_sdr2_tx_thread;
      gr::thread::thread cd_sdr3_tx_thread;
      gr::thread::thread cd_sdr1_rx_thread;
      gr::thread::thread cd_sdr2_rx_thread;
      gr::thread::thread cd_sdr3_rx_thread;

      gr::thread::thread cb_sdr1_tx_thread;
      gr::thread::thread cb_sdr2_tx_thread;
      gr::thread::thread cb_sdr3_tx_thread;
      gr::thread::thread cb_sdr1_rx_thread;
      gr::thread::thread cb_sdr2_rx_thread;
      gr::thread::thread cb_sdr3_rx_thread;

      pmt::pmt_t tx_data_sdr1;
      pmt::pmt_t tx_data_sdr2;
      pmt::pmt_t tx_data_sdr3;

      pmt::pmt_t meta_sdr1;
      pmt::pmt_t meta_sdr2;
      pmt::pmt_t meta_sdr3;

      std::vector<std::vector<gr_complex>> tx_buffs;
      std::atomic<bool> finished;

      size_t tx_buff_size, rx_buff_size;
      size_t n_tx_total;

      pmt::pmt_t tx_data;
      pmt::pmt_t meta;

      // Metadata keys
      std::string sdr1_freq_key;
      std::string sdr2_freq_key;
      std::string sample_start_key;
      std::string prf_key;

    private:
      void config_usrp(uhd::usrp::multi_usrp::sptr &usrp_1,
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
                       bool verbose);
      void receive(uhd::usrp::multi_usrp::sptr usrp_rx,
                   uhd::rx_streamer::sptr rx_stream,
                   double start_time, int sdr_rx);
      void transmit_bursts(uhd::usrp::multi_usrp::sptr usrp_tx,
                           uhd::tx_streamer::sptr tx_stream,
                           double start_time, int sdr_id);
      void set_metadata_keys(const std::string &sdr1_freq_key,
                             const std::string &sdr2_freq_key,
                             const std::string &sample_start_key,
                             const std::string &prf_key);
      void setup_streamers(
          uhd::rx_streamer::sptr &rx1_stream,
          uhd::rx_streamer::sptr &rx2_stream,
          uhd::rx_streamer::sptr &rx3_stream,
          uhd::tx_streamer::sptr &tx1_stream,
          uhd::tx_streamer::sptr &tx2_stream,
          uhd::tx_streamer::sptr &tx3_stream);

      void transmit_all(
          uhd::usrp::multi_usrp::sptr usrp,
          uhd::tx_streamer::sptr tx_stream,
          const std::vector<double> &times,
          int sdr_id);
      void receive_all(
          uhd::usrp::multi_usrp::sptr usrp,
          uhd::rx_streamer::sptr rx_stream,
          const std::vector<double> &times,
          const std::vector<int> &rx_tags);

    public:
      usrp_radar_all_impl(const std::string &args_1,
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
                          const double wait_time,
                          const double TDMA_time,
                          const bool verbose);
      ~usrp_radar_all_impl();
      void run();
      void check_cd_ready();
      void check_cb_ready();
      void cd_run();
      void cb_run();
      bool start() override;
      bool stop() override;
      void handle_message(const pmt::pmt_t &msg, int sdr_id);
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_USRP_RADAR_ALL_IMPL_H */
