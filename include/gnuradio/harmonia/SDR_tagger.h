/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_SDR_TAGGER_H
#define INCLUDED_HARMONIA_SDR_TAGGER_H

#include <gnuradio/harmonia/device.h>
#include <gnuradio/block.h>
#include <gnuradio/harmonia/api.h>

namespace gr {
namespace harmonia {

/*!
 * \brief <+description of block+>
 * \ingroup harmonia
 *
 */
class HARMONIA_API SDR_tagger : virtual public gr::block {
public:
  typedef std::shared_ptr<SDR_tagger> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of harmonia::SDR_tagger.
   *
   * To avoid accidental use of raw pointers, harmonia::SDR_tagger's
   * constructor is in a private implementation
   * class. harmonia::SDR_tagger::make is the public interface for
   * creating new instances.
   */
  static sptr make(const std::string& sdr_num_key);
};

} // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_SDR_TAGGER_H */
