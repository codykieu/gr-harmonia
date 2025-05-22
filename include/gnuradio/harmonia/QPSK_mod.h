/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_QPSK_MOD_H
#define INCLUDED_HARMONIA_QPSK_MOD_H

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
    class HARMONIA_API QPSK_mod : virtual public gr::block
    {
    public:
      typedef std::shared_ptr<QPSK_mod> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of harmonia::QPSK_mod.
       *
       * To avoid accidental use of raw pointers, harmonia::QPSK_mod's
       * constructor is in a private implementation
       * class. harmonia::QPSK_mod::make is the public interface for
       * creating new instances.
       */
      static sptr make(int sps = 1);
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_QPSK_MOD_H */
