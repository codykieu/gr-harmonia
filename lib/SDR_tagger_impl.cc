/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "SDR_tagger_impl.h"
#include <gnuradio/io_signature.h>

namespace gr
{
  namespace harmonia
  {

    SDR_tagger::sptr SDR_tagger::make(const std::string &sdr_num_key)
    {
      return gnuradio::make_block_sptr<SDR_tagger_impl>(sdr_num_key);
    }

    /*
     * The private constructor
     */
    SDR_tagger_impl::SDR_tagger_impl(const std::string &sdr_num_key)
        : gr::block("SDR_tagger",
                    gr::io_signature::make(0, 0, 0),
                    gr::io_signature::make(0, 0, 0)),
          d_sdr_num_key(pmt::intern(sdr_num_key))
    {
      d_in_port = PMT_HARMONIA_IN;
      d_out_port = PMT_HARMONIA_OUT;
      d_meta = pmt::make_dict();
      d_data = pmt::make_c32vector(0, 0);
      message_port_register_in(d_in_port);
      message_port_register_out(d_out_port);
      set_msg_handler(d_in_port, [this](pmt::pmt_t msg)
                      { handle_msg(msg); });
    }

    /*
     * Our virtual destructor.
     */
    SDR_tagger_impl::~SDR_tagger_impl() {}

    void SDR_tagger_impl::handle_msg(pmt::pmt_t msg)
    {

      // Read the input PDU
      pmt::pmt_t samples;
      if (pmt::is_pdu(msg))
      {
        pmt::pmt_t meta = pmt::car(msg);
        samples = pmt::cdr(msg);
        d_meta = pmt::dict_update(d_meta, meta);
      }
      else if (pmt::is_uniform_vector(msg))
      {
        samples = msg;
      }
      else
      {
        GR_LOG_WARN(d_logger, "Invalid message type")
        return;
      }

      d_meta = pmt::dict_add(d_meta, pmt::intern("src"), d_sdr_num_key);

      message_port_pub(d_out_port, pmt::cons(d_meta, samples));
    }

  } /* namespace harmonia */
} /* namespace gr */
