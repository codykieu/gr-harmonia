/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_CLOCKBIAS_PHASE_EST_H
#define INCLUDED_HARMONIA_CLOCKBIAS_PHASE_EST_H

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
    class HARMONIA_API clockbias_phase_est : virtual public gr::block
    {
    public:
      typedef std::shared_ptr<clockbias_phase_est> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of
       * harmonia::clockbias_phase_est.
       *
       * To avoid accidental use of raw pointers, harmonia::clockbias_phase_est's
       * constructor is in a private implementation
       * class. harmonia::clockbias_phase_est::make is the public interface for
       * creating new instances.
       */
      static sptr make(int num_platforms, double center_freq, double samp_rate, double pulse_width,
                       double SNR, bool bias_status, bool phase_status);
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_CLOCKBIAS_PHASE_EST_H */
