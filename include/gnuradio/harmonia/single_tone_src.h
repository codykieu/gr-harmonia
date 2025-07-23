/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_SINGLE_TONE_SRC_H
#define INCLUDED_HARMONIA_SINGLE_TONE_SRC_H

#include <gnuradio/block.h>
#include <gnuradio/harmonia/api.h>

namespace gr {
namespace harmonia {

/*!
 * \brief <+description of block+>
 * \ingroup harmonia
 *
 */
class HARMONIA_API single_tone_src : virtual public gr::block {
public:
  typedef std::shared_ptr<single_tone_src> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of harmonia::single_tone_src.
   *
   * To avoid accidental use of raw pointers, harmonia::single_tone_src's
   * constructor is in a private implementation
   * class. harmonia::single_tone_src::make is the public interface for
   * creating new instances.
   */
  static sptr
  make(double frequency, double center_freq, double phase, double pulse_width, double samp_rate, double prf, int sdr_id);

  /**
   * @brief Set the metadata keys in the PDU output
   *
   * @param label
   * @param sample_rate
   * @param frequency
   * @param duration
   */
  virtual void init_meta_dict(const std::string& frequency_key,
                              const std::string& phase_key,
                              const std::string& duration_key,
                              const std::string& sample_rate_key,
                              const std::string& label_key,
                              const std::string& prf_key) = 0;
};

} // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_SINGLE_TONE_SRC_H */
