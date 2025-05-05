/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_BUFFER_CORRECTOR_H
#define INCLUDED_HARMONIA_BUFFER_CORRECTOR_H

#include <gnuradio/block.h>
#include <gnuradio/harmonia/api.h>

namespace gr {
namespace harmonia {

/*!
 * \brief <+description of block+>
 * \ingroup harmonia
 *
 */
class HARMONIA_API buffer_corrector : virtual public gr::block {
public:
  typedef std::shared_ptr<buffer_corrector> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of harmonia::buffer_corrector.
   *
   * To avoid accidental use of raw pointers, harmonia::buffer_corrector's
   * constructor is in a private implementation
   * class. harmonia::buffer_corrector::make is the public interface for
   * creating new instances.
   */
  static sptr make(int max_buffer_size);
};

} // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_BUFFER_CORRECTOR_H */
