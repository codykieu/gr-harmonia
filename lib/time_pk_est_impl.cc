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
                                        double samp_rate,
                                        double bandwidth,
                                        double NLLS_iter,
                                        double NLLS_pts)
    {
      return gnuradio::make_block_sptr<time_pk_est_impl>(nfft,
                                                         samp_rate,
                                                         bandwidth,
                                                         NLLS_iter,
                                                         NLLS_pts);
    }

    /*
     * The private constructor
     */
    time_pk_est_impl::time_pk_est_impl(size_t nfft,
                                       double samp_rate,
                                       double bandwidth,
                                       double NLLS_iter,
                                       double NLLS_pts)
        : gr::block("time_pk_est",
                    gr::io_signature::make(0, 0, 0),
                    gr::io_signature::make(0, 0, 0)),
          d_fftsize(nfft),
          d_samp_rate(samp_rate),
          d_bandwidth(bandwidth),
          d_NLLS_iter(NLLS_iter),
          d_NLLS_pts(NLLS_pts)
    {
      d_data = pmt::make_c32vector(1, 0);
      d_meta = pmt::make_dict();
      d_meta_time = pmt::make_dict();
      sdr1_dict = pmt::make_dict();
      sdr2_dict = pmt::make_dict();
      sdr3_dict = pmt::make_dict();

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

    // Sinc Function
    af::array time_pk_est_impl::sinc(const af::array &x)
    {
      return af::select(x == 0.0, 1.0, af::sin(af::Pi * x) / (af::Pi * x));
    }

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
        d_meta = pmt::dict_update(d_meta, meta);
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

      // Check which SDR TX the signal
      pmt::pmt_t src_key = pmt::intern("src");
      pmt::pmt_t src_val = pmt::dict_ref(d_meta, src_key, pmt::PMT_NIL);
      // Check if 'TDMA_Done' is true
      pmt::pmt_t TDMA_key = pmt::intern("TDMA_Done");
      pmt::pmt_t TDMA_val = pmt::dict_ref(d_meta, TDMA_key, pmt::PMT_NIL);
      // Check for 'rx_time'
      pmt::pmt_t rx_time_key = pmt::intern("rx_time");
      if (pmt::dict_has_key(d_meta, rx_time_key))
      {
        pmt::pmt_t rx_time_val = pmt::dict_ref(d_meta, rx_time_key, pmt::PMT_NIL);
        if (pmt::is_real(rx_time_val))
        {
          rx_time = pmt::to_double(rx_time_val);
          std::cout << "rx_time = " << rx_time << std::endl;
        }
      }
      // Compute matrix and vector dimensions
      size_t n = pmt::length(samples);
      size_t nconv = n + d_match_filt.elements() - 1;
      if (pmt::length(d_data) != n)
        d_data = pmt::make_c32vector(nconv, 0);

      // Get input and output data
      size_t io(0);
      const gr_complex *in = pmt::c32vector_elements(samples, io);

      // Apply the matched filter to each column
      af::array mf_resp(af::dim4(n), reinterpret_cast<const af::cfloat *>(in));
      mf_resp = af::convolve1(mf_resp, d_match_filt, AF_CONV_EXPAND, AF_CONV_AUTO);
      mf_resp = af::abs(mf_resp);
      std::cout << "Length of mf = " << mf_resp.elements() << std::endl;

      // Output max and index of data
      double max_val;
      unsigned max_idx;
      af::max(&max_val, &max_idx, mf_resp);

      // Generate time axis
      af::array t =
          (af::seq(0, nconv - 1)) * 1.0 / d_samp_rate + rx_time;

      af::array t_pk = t(max_idx); // Get the value at max_idx as an af::array
      af_print(t_pk);

      // ----------------- Sinc-NLLS -----------------
      // Create lambda array
      double lambda[] = {max_val, 0.0, d_bandwidth / d_samp_rate};
      af::array af_lambda(3, lambda, afHost);

      // Making Index for NLLS
      af::array nlls_ind = af::seq(0.0, d_NLLS_pts - 1.0);
      nlls_ind = nlls_ind - af::median(nlls_ind);
      nlls_ind = nlls_ind;

      // Output vector of points around max index
      af::array nlls_y = mf_resp(max_idx - nlls_ind);
      nlls_y = nlls_y;

      // Iterates through NLLS x times
      for (int i = 0; i < d_NLLS_iter; i++)
      {

        af::array nlls_f = (lambda[0] * sinc((nlls_ind - lambda[1]) * lambda[2]));

        // Gaussian-Newton optimization
        af::array GN1 = sinc(lambda[2] * (nlls_ind - lambda[1]));
        af::array GN2 =
            (lambda[0] * (sinc(lambda[2] * (nlls_ind - lambda[1])) -
                          af::cos(af::Pi * lambda[2] * (nlls_ind - lambda[1])))) /
            (nlls_ind - lambda[1]);
        af::array GN3 =
            (lambda[0] * (af::cos(af::Pi * lambda[2] * (nlls_ind - lambda[1])) -
                          sinc(lambda[2] * (nlls_ind - lambda[1])))) /
            (lambda[2]);
        af::array nlls_J = af::join(1, GN1, GN2, GN3);
        af::replace(nlls_J, !(af::isNaN(nlls_J)), 0.0);

        //  Residual errors
        af::array nlls_delty = nlls_y - nlls_f;

        // Parameter
        af::array nlls_deltlambda = af::matmul((af::matmul((af::inverse(af::matmul(nlls_J.T(), nlls_J))), nlls_J.T())), nlls_delty);

        // Estimate
        af_lambda = af_lambda + nlls_deltlambda;
        af_lambda.host(lambda);
      }

      // Compute frequency estimate
      af::array t_est_arr = t_pk + (af_lambda(1) / d_samp_rate);
      // Extract scalar value from ArrayFire array
      t_est_arr.host(&t_est);

      if (pmt::equal(src_val, PMT_HARMONIA_SDR1))
      {
        GR_LOG_WARN(d_logger, "SDR1 Found");
        sdr1_dict = pmt::dict_add(sdr1_dict, d_time_est_key, pmt::from_double(t_est));
        d_meta_time = pmt::dict_add(d_meta_time, PMT_HARMONIA_SDR1, sdr1_dict);
      }
      else if (pmt::equal(src_val, PMT_HARMONIA_SDR2))
      {
        GR_LOG_WARN(d_logger, "SDR2 Found");
        sdr2_dict = pmt::dict_add(sdr2_dict, d_time_est_key, pmt::from_double(t_est));
        d_meta_time = pmt::dict_add(d_meta_time, PMT_HARMONIA_SDR2, sdr2_dict);
      }
      else if (pmt::equal(src_val, PMT_HARMONIA_SDR3))
      {
        GR_LOG_WARN(d_logger, "SDR3 Found");
        sdr3_dict = pmt::dict_add(sdr3_dict, d_time_est_key, pmt::from_double(t_est));
        d_meta_time = pmt::dict_add(d_meta_time, PMT_HARMONIA_SDR3, sdr3_dict);
      }
      else
      {
        GR_LOG_WARN(d_logger, "Unknown SDR source in metadata");
      }

      // Wait for TDMA to fully finish before outputting all data
      if (pmt::equal(TDMA_val, pmt::PMT_T))
      {
        GR_LOG_WARN(d_logger, "TDMA done");
        // Send the time estimates as a message with metadata
        message_port_pub(d_out_port, d_meta_time);
      }
      else
      {
        GR_LOG_WARN(d_logger, "TDMA not done, unable to publish metadata");
      }

      // Reset the metadata output
      d_meta = pmt::make_dict();
    }

    void time_pk_est_impl::set_metadata_keys()
    {
      d_time_est_key = pmt::intern("time_est");

      // Create inner dicts with freq_est: PMT_NIL
      sdr1_dict = pmt::dict_add(sdr1_dict, d_time_est_key, pmt::PMT_NIL);
      sdr2_dict = pmt::dict_add(sdr2_dict, d_time_est_key, pmt::PMT_NIL);
      sdr3_dict = pmt::dict_add(sdr3_dict, d_time_est_key, pmt::PMT_NIL);

      // Add to main metadata dictionary
      d_meta_time = pmt::dict_add(d_meta_time, PMT_HARMONIA_SDR1, sdr1_dict);
      d_meta_time = pmt::dict_add(d_meta_time, PMT_HARMONIA_SDR2, sdr2_dict);
      d_meta_time = pmt::dict_add(d_meta_time, PMT_HARMONIA_SDR3, sdr3_dict);
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
