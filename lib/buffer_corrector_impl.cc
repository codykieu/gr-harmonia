/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "buffer_corrector_impl.h"
#include <gnuradio/io_signature.h>

namespace gr
{
  namespace harmonia
  {

    buffer_corrector::sptr buffer_corrector::make(int max_buffer_size)
    {
      return gnuradio::make_block_sptr<buffer_corrector_impl>(max_buffer_size);
    }

    /*
     * The private constructor
     */
    buffer_corrector_impl::buffer_corrector_impl(int max_buffer_size)
        : gr::block("buffer_corrector",
                    gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
          in_port(PMT_HARMONIA_IN),
          out_port(PMT_HARMONIA_OUT),
          max_buffer_size(max_buffer_size)
    {
      message_port_register_in(in_port);
      message_port_register_out(out_port);

      set_msg_handler(in_port, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
    }

    /*
     * Our virtual destructor.
     */
    buffer_corrector_impl::~buffer_corrector_impl() {}

    void buffer_corrector_impl::handle_msg(pmt::pmt_t msg)
    {
      if (!pmt::is_pair(msg))
      {
        GR_LOG_WARN(d_logger, "Received non-PDU message, ignoring.");
        return;
      }

      // Extract metadata and data vector
      pmt::pmt_t meta = pmt::car(msg);
      pmt::pmt_t data = pmt::cdr(msg);

      if (!pmt::is_c32vector(data))
      {
        GR_LOG_WARN(d_logger, "Received non-complex vector, ignoring.");
        return;
      }

      // Make a mutable copy of the data vector
      std::vector<gr_complex> data_vector = pmt::c32vector_elements(data);
      size_t tx_data_len = data_vector.size();
      size_t remainder = tx_data_len % max_buffer_size;
      // GR_LOG_INFO(d_logger, "Remainder: " + std::to_string(remainder));
      if (remainder != 0)
      {
        size_t padding_needed = max_buffer_size - remainder;
        data_vector.insert(data_vector.end(), padding_needed, gr_complex(0, 0));
      }

      pmt::pmt_t padded_vector_pmt = pmt::init_c32vector(data_vector.size(), data_vector);
      message_port_pub(out_port, pmt::cons(meta, padded_vector_pmt));
    }

  } /* namespace harmonia */
} /* namespace gr */
