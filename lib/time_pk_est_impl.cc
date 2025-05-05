/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "time_pk_est_impl.h"
#include <gnuradio/io_signature.h>
#include <arrayfire.h>
#include <cmath>
#include <iomanip>

namespace gr
{
  namespace harmonia
  {

    time_pk_est::sptr time_pk_est::make(size_t nfft,
                                        double pulse_width,
                                        double cap_length,
                                        double samp_rate,
                                        double NLLS_iter,
                                        double NLLS_pts)
    {
      return gnuradio::make_block_sptr<time_pk_est_impl>(nfft,
                                                         pulse_width,
                                                         cap_length,
                                                         samp_rate,
                                                         NLLS_iter,
                                                         NLLS_pts);
    }

    /*
     * The private constructor
     */
    time_pk_est_impl::time_pk_est_impl(size_t nfft,
                                       double pulse_width,
                                       double cap_length,
                                       double samp_rate,
                                       double NLLS_iter,
                                       double NLLS_pts)
        : gr::block("time_pk_est",
                    gr::io_signature::make(0, 0, 0),
                    gr::io_signature::make(0, 0, 0)),
          d_fftsize(nfft),
          d_pulse_width(pulse_width),
          d_cap_length(cap_length),
          d_samp_rate(samp_rate),
          d_NLLS_iter(NLLS_iter),
          d_NLLS_pts(NLLS_pts)
    {
      d_data = pmt::make_c32vector(1, 0);
      d_meta = pmt::make_dict();

      d_tx_port = PMT_HARMONIA_TX;
      d_rx_port = PMT_HARMONIA_RX;
      d_out_port = PMT_HARMONIA_OUT;
      message_port_register_in(d_tx_port);
      message_port_register_in(d_rx_port);
      message_port_register_out(d_out_port);
      set_msg_handler(d_tx_port, [this](pmt::pmt_t msg)
                      { handle_tx_msg(msg); });
      set_msg_handler(d_rx_port, [this](pmt::pmt_t msg)
                      { handle_rx_msg(msg); });
    }

    /*
     * Our virtual destructor.
     */
    time_pk_est_impl::~time_pk_est_impl() {}

    void time_pk_est_impl::handle_tx_msg(pmt::pmt_t msg)
    {
      pmt::pmt_t samples;
      if (pmt::is_pdu(msg))
      {
        // Get the transmit data
        samples = pmt::cdr(msg);
        d_meta = pmt::dict_update(d_meta, pmt::car(msg));
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
      size_t n = pmt::length(samples);
      size_t io(0);
      const gr_complex *tx_data = pmt::c32vector_elements(samples, io);

      // Resize the matched filter array if necessary
      if (d_match_filt.elements() != (int)n)
      {
        d_match_filt = af::constant(0, n, c32);
      }
      // Create the matched filter
      d_match_filt = af::array(af::dim4(n), reinterpret_cast<const af::cfloat *>(tx_data));
      d_match_filt = af::conjg(d_match_filt);
      d_match_filt = af::flip(d_match_filt, 0);
    }

    void time_pk_est_impl::handle_rx_msg(pmt::pmt_t msg)
    {
      if (this->nmsgs(d_rx_port) > d_msg_queue_depth or d_match_filt.elements() == 0)
      {
        return;
      }
      // Get a copy of the input samples
      pmt::pmt_t samples, meta;
      if (pmt::is_pdu(msg))
      {
        meta = pmt::car(msg);
        samples = pmt::cdr(msg);
      }
      else if (pmt::is_uniform_vector(msg))
      {
        samples = msg;
      }
      else
      {
        GR_LOG_WARN(d_logger, "Invalid message type");
        return;
      }

      // Compute matrix and vector dimensions
      size_t n = pmt::length(samples);
      size_t nconv = n + d_match_filt.elements() - 1;
      if (pmt::length(d_data) != n)
        d_data = pmt::make_c32vector(nconv, 0);

      // Get input and output data
      size_t io(0);
      const gr_complex *in = pmt::c32vector_elements(samples, io);
      gr_complex *out = pmt::c32vector_writable_elements(d_data, io);

      // Apply the matched filter to each column
      af::array mf_resp(af::dim4(n), reinterpret_cast<const af::cfloat *>(in));
      mf_resp = af::convolve1(mf_resp, d_match_filt, AF_CONV_EXPAND, AF_CONV_AUTO);
      mf_resp = af::abs(mf_resp);
      // // Output max and index of data
      // double max_val;
      // unsigned max_idx;
      // af::max(&max_val, &max_idx, mf_resp);

      // // Generate time axis
      // af::array t =
      //     (d_rx_time) + (af::seq(0, (d_cap_length - (1/d_samp_rate))*d_samp_rate) * 1/d_samp_rate);

      // af::array t_pk = t(max_idx); // Get the value at max_idx as an af::array

      // // ----------------- Sinc-NLLS -----------------
      // // Create lambda array
      // double lambda[] = {max_val, 0.0, d_pulse_width / d_cap_length};
      // af::array af_lambda(3, lambda, afHost);

      // // Making Index for NLLS
      // af::array nlls_ind = af::seq(0.0, d_NLLS_pts - 1.0);
      // nlls_ind = nlls_ind - af::median(nlls_ind);
      // nlls_ind = nlls_ind.as(f64);

      // // Output vector of points around max index
      // af::array nlls_y = af_abs_fft(max_idx - nlls_ind);
      // nlls_y = nlls_y.as(f64);

      // // Iterates through NLLS x times
      // for (int i = 0; i < d_NLLS_iter; i++)
      // {

      //   af::array nlls_f = (lambda[0] * sinc((nlls_ind - lambda[1]) * lambda[2]));

      //   // Gaussian-Newton optimization
      //   af::array GN1 = sinc(lambda[2] * (nlls_ind - lambda[1]));
      //   af::array GN2 =
      //       (lambda[0] * (sinc(lambda[2] * (nlls_ind - lambda[1])) -
      //                     af::cos(af::Pi * lambda[2] * (nlls_ind - lambda[1])))) /
      //       (nlls_ind - lambda[1]);
      //   af::array GN3 =
      //       (lambda[0] * (af::cos(af::Pi * lambda[2] * (nlls_ind - lambda[1])) -
      //                     sinc(lambda[2] * (nlls_ind - lambda[1])))) /
      //       (lambda[2]);
      //   af::array nlls_J = af::join(1, GN1, GN2, GN3);
      //   af::replace(nlls_J, !(af::isNaN(nlls_J)), 0.0);

      //   //  Residual errors
      //   af::array nlls_delty = nlls_y - nlls_f;

      //   // Parameter
      //   af::array nlls_deltlambda = af::matmul((af::matmul((af::inverse(af::matmul(nlls_J.T(), nlls_J))), nlls_J.T())), nlls_delty);

      //   // Estimate
      //   af_lambda = af_lambda + nlls_deltlambda;
      //   af_lambda.host(lambda);
      // }

      // // Compute frequency estimate
      // af::array t_est_arr = t_pk + (af_lambda(1) / d_cap_length);

      // // Extract scalar value from ArrayFire array
      // double t_est;
      // t_est_arr.host(&t_est);

      // // Create message for frequency estimate
      // pmt::pmt_t d_t_est = pmt::from_double(t_est);

      mf_resp.host(out);

      message_port_pub(d_out_port, pmt::cons(d_meta, d_data));
      // Reset the metadata output
      d_meta = pmt::make_dict();
    }

    void time_pk_est_impl::set_metadata_keys()
    {
    }

    void time_pk_est_impl::set_msg_queue_depth(size_t depth) { d_msg_queue_depth = depth; }

    void time_pk_est_impl::set_backend(Device::Backend backend)
    {
      switch (backend)
      {
      case Device::CPU:
        d_backend = AF_BACKEND_CPU;
        break;
      case Device::CUDA:
        d_backend = AF_BACKEND_CUDA;
        break;
      case Device::OPENCL:
        d_backend = AF_BACKEND_OPENCL;
        break;
      default:
        d_backend = AF_BACKEND_DEFAULT;
        break;
      }
      af::setBackend(d_backend);
    }

  } /* namespace harmonia */
} /* namespace gr */
