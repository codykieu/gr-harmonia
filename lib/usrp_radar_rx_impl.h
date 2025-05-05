/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_USRP_RADAR_RX_IMPL_H
#define INCLUDED_HARMONIA_USRP_RADAR_RX_IMPL_H

#include <gnuradio/harmonia/pmt_constants.h>
#include <gnuradio/harmonia/usrp_radar_rx.h>
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

    class usrp_radar_rx_impl : public usrp_radar_rx
    {
    private:
      // Block params
      uhd::usrp::multi_usrp::sptr usrp_rx;
      std::string usrp_args_rx;
      double rx_rate;
      double rx_freq;
      double rx_gain;
      double start_delay;
      std::vector<size_t> rx_channel_nums;
      std::string rx_subdev;
      std::string rx_device_addr;
      std::string rx_cpu_format;
      std::string rx_otw_format;
      double prf;

      bool elevate_priority;
      bool verbose;
      size_t n_delay;

      // Implementation params
      gr::thread::thread main_thread;
      gr::thread::thread rx_thread;
      std::atomic<bool> finished;
      std::atomic<bool> new_msg_received;
      size_t rx_buff_size;
      size_t n_tx_total;

      pmt::pmt_t tx_data;
      pmt::pmt_t meta;

      // Metadata keys
      std::string rx_freq_key;
      std::string sample_start_key;
      std::string prf_key;

    private:
      void handle_msg(pmt::pmt_t msg);
      void config_usrp(uhd::usrp::multi_usrp::sptr &usrp_rx,
                       const std::string &args_rx,
                       const double rx_rate,
                       const double rx_freq,
                       const double rx_gain,
                       const std::string &rx_subdev,
                       bool verbose);
      void receive(uhd::usrp::multi_usrp::sptr usrp_rx,
                   uhd::rx_streamer::sptr rx_stream,
                   double start_time);
      void set_metadata_keys(const std::string &rx_freq_key,
                             const std::string &sample_start_key,
                             const std::string &prf_key);

    public:
      usrp_radar_rx_impl(const std::string &args_rx,
                        const double rx_rate,
                        const double rx_freq,
                        const double rx_gain,
                        const double start_delay,
                        const bool elevate_priority,
                        const bool verbose);
      ~usrp_radar_rx_impl();

      void run();
      bool start() override;
      bool stop() override;
      void handle_message(const pmt::pmt_t &msg);
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_USRP_RADAR_RX_IMPL_H */
