/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_USRP_RADAR_2_H
#define INCLUDED_HARMONIA_USRP_RADAR_2_H

#include <gnuradio/block.h>
#include <gnuradio/harmonia/api.h>

namespace gr
{
  namespace harmonia
  {

    /*!
     * \brief A block that simultaneously transmits and receives data from a USRP device
     * \ingroup harmonia
     *
     */
    class HARMONIA_API usrp_radar_2 : virtual public gr::block
    {
    public:
      typedef std::shared_ptr<usrp_radar_2> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of harmonia::usrp_radar_2.
       *
       * To avoid accidental use of raw pointers, harmonia::usrp_radar_2's
       * constructor is in a private implementation
       * class. harmonia::usrp_radar_2::make is the public interface for
       * creating new instances.
       */
      static sptr make(const std::string &args_tx,
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
                      double cap_length);
      virtual void set_metadata_keys(const std::string &tx_freq_key,
                                     const std::string &rx_freq_key,
                                     const std::string &sample_start_key,
                                     const std::string &prf_key) = 0;
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_USRP_RADAR_2_H */
