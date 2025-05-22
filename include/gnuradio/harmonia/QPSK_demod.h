/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_QPSK_DEMOD_H
#define INCLUDED_HARMONIA_QPSK_DEMOD_H

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
    class HARMONIA_API QPSK_demod : virtual public gr::block
    {
    public:
      typedef std::shared_ptr<QPSK_demod> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of harmonia::QPSK_demod.
       *
       * To avoid accidental use of raw pointers, harmonia::QPSK_demod's
       * constructor is in a private implementation
       * class. harmonia::QPSK_demod::make is the public interface for
       * creating new instances.
       */
      static sptr make(int spb);
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_QPSK_DEMOD_H */
