/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_USRP_RADAR_TX_IMPL_H
#define INCLUDED_HARMONIA_USRP_RADAR_TX_IMPL_H

#include <gnuradio/harmonia/pmt_constants.h>
#include <gnuradio/harmonia/usrp_radar_tx.h>
#include <nlohmann/json.hpp>
#include <uhd/convert.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/thread/thread.hpp>
#include <fstream>
#include <queue>

namespace gr
{
  namespace harmonia
  {

    class usrp_radar_tx_impl : public usrp_radar_tx
    {
    private:
      // Block params
      uhd::usrp::multi_usrp::sptr usrp_tx;
      std::string usrp_args_tx;
      double tx_rate;
      double tx_freq;
      double tx_gain;
      double start_delay;
      std::vector<size_t> tx_channel_nums;
      std::string tx_subdev;
      std::string tx_device_addr;
      std::string tx_cpu_format;
      std::string tx_otw_format;
      double prf;

      bool elevate_priority;
      bool verbose;
      size_t n_delay;

      // Implementation params
      gr::thread::thread main_thread;
      gr::thread::thread tx_thread;
      std::vector<std::vector<gr_complex>> tx_buffs;
      std::atomic<bool> finished;
      std::atomic<bool> new_msg_received;
      size_t tx_buff_size;
      size_t n_tx_total;

      pmt::pmt_t tx_data;
      pmt::pmt_t meta;

      // Metadata keys
      std::string tx_freq_key;
      std::string sample_start_key;
      std::string prf_key;

    private:
      void handle_msg(pmt::pmt_t msg);
      void config_usrp(uhd::usrp::multi_usrp::sptr &usrp_tx,
                       const std::string &args_tx,
                       const double tx_rate,
                       const double tx_freq,
                       const double tx_gain,
                       const std::string &tx_subdev,
                       bool verbose);
      void transmit_bursts(uhd::usrp::multi_usrp::sptr usrp_tx,
                           uhd::tx_streamer::sptr tx_stream,
                           double start_time);
      void transmit_continuous(uhd::usrp::multi_usrp::sptr usrp_tx,
                               uhd::tx_streamer::sptr tx_stream,
                               double start_time);
      void set_metadata_keys(const std::string &tx_freq_key,
                             const std::string &sample_start_key,
                             const std::string &prf_key);

    public:
      usrp_radar_tx_impl(const std::string &args_tx,
                         const double tx_rate,
                         const double tx_freq,
                         const double tx_gain,
                         const double start_delay,
                         const bool elevate_priority,
                         const bool verbose);
      ~usrp_radar_tx_impl();

      void run();
      bool start() override;
      bool stop() override;
      void handle_message(const pmt::pmt_t &msg);
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_USRP_RADAR_TX_IMPL_H */
