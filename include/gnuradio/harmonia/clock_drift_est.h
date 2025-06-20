/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_CLOCK_DRIFT_EST_H
#define INCLUDED_HARMONIA_CLOCK_DRIFT_EST_H

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
    class HARMONIA_API clock_drift_est : virtual public gr::block
    {
    public:
      typedef std::shared_ptr<clock_drift_est> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of harmonia::clock_drift_est.
       *
       * To avoid accidental use of raw pointers, harmonia::clock_drift_est's
       * constructor is in a private implementation
       * class. harmonia::clock_drift_est::make is the public interface for
       * creating new instances.
       */
      static sptr make(int num_platforms, double baseband_freq,
                       double center_freq, double samp_rate, double pulse_width, 
                       double SNR);
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_CLOCK_DRIFT_EST_H */
