/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_USRP_RADAR_TDMA_H
#define INCLUDED_HARMONIA_USRP_RADAR_TDMA_H

#include <gnuradio/block.h>
#include <gnuradio/harmonia/api.h>

namespace gr {
namespace harmonia {

/*!
 * \brief <+description of block+>
 * \ingroup harmonia
 *
 */
class HARMONIA_API usrp_radar_tdma : virtual public gr::block {
public:
  typedef std::shared_ptr<usrp_radar_tdma> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of harmonia::usrp_radar_tdma.
   *
   * To avoid accidental use of raw pointers, harmonia::usrp_radar_tdma's
   * constructor is in a private implementation
   * class. harmonia::usrp_radar_tdma::make is the public interface for
   * creating new instances.
   */
  static sptr make(const std::string &args,
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
};

} // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_USRP_RADAR_TDMA_H */
