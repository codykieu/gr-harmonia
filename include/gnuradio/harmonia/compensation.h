/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_COMPENSATION_H
#define INCLUDED_HARMONIA_COMPENSATION_H

#include <gnuradio/block.h>
#include <gnuradio/harmonia/api.h>

namespace gr {
namespace harmonia {

/*!
 * \brief <+description of block+>
 * \ingroup harmonia
 *
 */
class HARMONIA_API compensation : virtual public gr::block {
public:
  typedef std::shared_ptr<compensation> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of harmonia::compensation.
   *
   * To avoid accidental use of raw pointers, harmonia::compensation's
   * constructor is in a private implementation
   * class. harmonia::compensation::make is the public interface for
   * creating new instances.
   */
  static sptr make(double center_freq, double samp_rate, int sdr_id);
};

} // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_COMPENSATION_H */
