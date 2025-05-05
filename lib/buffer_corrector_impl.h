/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_BUFFER_CORRECTOR_IMPL_H
#define INCLUDED_HARMONIA_BUFFER_CORRECTOR_IMPL_H

#include <gnuradio/harmonia/buffer_corrector.h>
#include <gnuradio/harmonia/pmt_constants.h>
#include <arrayfire.h>

namespace gr
{
  namespace harmonia
  {

    class buffer_corrector_impl : public buffer_corrector
    {
    private:
      const pmt::pmt_t in_port;
      const pmt::pmt_t out_port;

      // Parameters
      int max_buffer_size;

    public:
      buffer_corrector_impl(int max_buffer_size);
      ~buffer_corrector_impl();

      void handle_msg(pmt::pmt_t msg);
    };

  } // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_BUFFER_CORRECTOR_IMPL_H */
