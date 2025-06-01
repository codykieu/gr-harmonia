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
      uhd::usrp::multi_usrp::sptr usrp_1;
      uhd::usrp::multi_usrp::sptr usrp_2;
      std::string usrp_args_sdr1;
      std::string usrp_args_sdr2;
      double sdr1_rate, sdr2_rate;
      double sdr1_freq, sdr2_freq;
      double sdr1_gain, sdr2_gain;
      double start_delay, cap_length;
      double wait_time;
      double TDMA_time;
      std::vector<size_t> sdr1_channel_nums, sdr2_channel_nums;
      std::string sdr1_subdev, sdr2_subdev;
      std::string sdr1_device_addr, sdr2_device_addr;
      std::string sdr1_cpu_format, sdr2_cpu_format;
      std::string sdr1_otw_format, sdr2_otw_format;
      double prf;
      bool verbose;
      size_t n_delay;

      // Clock Drift Params
      bool clock_drift_enabled = false;
      bool cd_run_executed = false;
      std::vector<double> sdr1_tx_times_drift;
      std::vector<double> sdr2_rx_times_drift;
      std::vector<int> sdr2_rx_tags_drift;
      std::vector<bool> sdr2_rx_final_drift;
      double cd1_est, cd2_est, cd3_est;
      double alpha;
      bool cd1_ready = false;
      bool cd2_ready = false;
      bool cd3_ready = false;
      bool waveform1_ready = false;
      bool waveform2_ready = false;
      bool waveform3_ready = false;
      pmt::pmt_t updated_data1 = pmt::PMT_NIL;
      pmt::pmt_t updated_data2 = pmt::PMT_NIL;
      pmt::pmt_t updated_data3 = pmt::PMT_NIL;

      // Implementation params
      gr::thread::thread main_thread;
      gr::thread::thread tx1_thread;
      gr::thread::thread tx2_thread;
      gr::thread::thread rx1_thread;
      gr::thread::thread rx2_thread;

      gr::thread::thread sdr1_tx_thread;
      gr::thread::thread sdr2_tx_thread;
      gr::thread::thread sdr1_rx_thread;
      gr::thread::thread sdr2_rx_thread;
      gr::thread::thread cd_sdr1_tx_thread;
      gr::thread::thread cd_sdr2_tx_thread;
      gr::thread::thread cd_sdr1_rx_thread;
      gr::thread::thread cd_sdr2_rx_thread;

      pmt::pmt_t tx_data_sdr1;
      pmt::pmt_t tx_data_sdr2;
      pmt::pmt_t tx_data_sdr3;

      pmt::pmt_t meta_sdr1;
      pmt::pmt_t meta_sdr2;
      pmt::pmt_t meta_sdr3;

      std::vector<std::vector<gr_complex>> tx_buffs;
      std::atomic<bool> finished;
      std::atomic<bool> new_msg_received;
      std::atomic<bool> new_msg_received_sdr1, new_msg_received_sdr2, new_msg_received_sdr3;

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
                       bool verbose);
      void receive(uhd::usrp::multi_usrp::sptr usrp_rx,
                   uhd::rx_streamer::sptr rx_stream,
                   double start_time, int sdr_id, bool TDMA_done);
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
          uhd::tx_streamer::sptr &tx1_stream,
          uhd::tx_streamer::sptr &tx2_stream);

      void transmit_all(
          uhd::usrp::multi_usrp::sptr usrp,
          uhd::tx_streamer::sptr tx_stream,
          const std::vector<double> &times,
          int sdr_id);
      void receive_all(
          uhd::usrp::multi_usrp::sptr usrp,
          uhd::rx_streamer::sptr rx_stream,
          const std::vector<double> &times,
          const std::vector<int> &rx_tags,
          const std::vector<bool> &final_flags);

    public:
      usrp_radar_all_impl(const std::string &args_1,
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
                          const bool verbose);
      ~usrp_radar_all_impl();
      void run();
      void check_cd_ready();
      void cd_run();
      bool start() override;
      bool stop() override;
      void handle_message(const pmt::pmt_t &msg, int sdr_id);
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_USRP_RADAR_ALL_IMPL_H */
