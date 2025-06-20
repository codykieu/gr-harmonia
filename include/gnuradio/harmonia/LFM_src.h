/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_LFM_SRC_H
#define INCLUDED_HARMONIA_LFM_SRC_H

#include <gnuradio/block.h>
#include <gnuradio/harmonia/api.h>

namespace gr {
namespace harmonia {

/*!
 * \brief <+description of block+>
 * \ingroup harmonia
 *
 */
class HARMONIA_API LFM_src : virtual public gr::block {
public:
  typedef std::shared_ptr<LFM_src> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of harmonia::LFM_src.
   *
   * To avoid accidental use of raw pointers, harmonia::LFM_src's
   * constructor is in a private implementation
   * class. harmonia::LFM_src::make is the public interface for
   * creating new instances.
   */
  static sptr
  make(double bandwidth, double start_freq, double center_freq, double pulse_width, double samp_rate, double prf, int sdr_id);

  /**
   * @brief Set the metadata keys in the PDU output
   *
   * @param label
   * @param sample_rate
   * @param bandwidth
   * @param duration
   */
  virtual void init_meta_dict(const std::string& bandwidth_key,
                              const std::string& duration_key,
                              const std::string& sample_rate_key,
                              const std::string& label_key,
                              const std::string& prf_key) = 0;
};

} // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_LFM_SRC_H */
