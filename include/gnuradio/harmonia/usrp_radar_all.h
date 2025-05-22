/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_USRP_RADAR_ALL_H
#define INCLUDED_HARMONIA_USRP_RADAR_ALL_H

#include <gnuradio/block.h>
#include <gnuradio/harmonia/api.h>

namespace gr
{
  namespace harmonia
  {

    /*!
     * \brief <+description of block+>
     * \ingroup harmonia
     *
     */
    class HARMONIA_API usrp_radar_all : virtual public gr::block
    {
    public:
      typedef std::shared_ptr<usrp_radar_all> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of harmonia::usrp_radar_all.
       *
       * To avoid accidental use of raw pointers, harmonia::usrp_radar_all's
       * constructor is in a private implementation
       * class. harmonia::usrp_radar_all::make is the public interface for
       * creating new instances.
       */
      static sptr make(const std::string &args_1,
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
      virtual void set_metadata_keys(const std::string &sdr1_freq_key,
                                     const std::string &sdr2_freq_key,
                                     const std::string &sample_start_key,
                                     const std::string &prf_key) = 0;
    };

} // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_USRP_RADAR_ALL_H */
