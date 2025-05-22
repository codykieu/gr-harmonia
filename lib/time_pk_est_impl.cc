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
      d_tp_meta = pmt::make_dict();
      d_tx_port = PMT_HARMONIA_TX;
      d_rx_port = PMT_HARMONIA_RX;
      // d_out_port = PMT_HARMONIA_OUT;
      d_tp_out_port = PMT_HARMONIA_TP_OUT;
      message_port_register_in(d_tx_port);
      message_port_register_in(d_rx_port);
      message_port_register_out(d_tp_out_port);
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
      // Check for incoming receiving data
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
      pmt::pmt_t src_val = pmt::dict_ref(d_meta, pmt::intern("src"), pmt::PMT_NIL);
      std::cout << "[SDR ID]: " << pmt::write_string(src_val) << std::endl;
      pmt::pmt_t TDMA_val = pmt::dict_ref(d_meta, pmt::intern("TDMA_Done"), pmt::PMT_NIL);

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
      af::array mf_resp_abs = af::abs(mf_resp);
      // std::cout << "Length of mf = " << mf_resp.elements() << std::endl;

      // Output max and index of data
      double max_val;
      unsigned max_idx;
      af::max(&max_val, &max_idx, mf_resp_abs);

      // Phase Estimates
      af::array complex_peak = mf_resp(max_idx);   
      af::array phase = af::arg(complex_peak);    
      float p_est = phase.scalar<float>(); 
      // std::cout << "Phase sigma boi: " << p_est << std::endl;

      // Generate time axis
      af::array t =
          (af::seq(0, nconv - 1)) * 1.0 / d_samp_rate + rx_time;

      af::array t_pk = t(max_idx); // Get the value at max_idx as an af::array
      // af_print(t_pk);

      // ----------------- Sinc-NLLS -----------------
      // Create lambda array
      double lambda[] = {max_val, 0.0, d_bandwidth / d_samp_rate};
      af::array af_lambda(3, lambda, afHost);

      // Making Index for NLLS
      af::array nlls_ind = af::seq(0.0, d_NLLS_pts - 1.0);
      nlls_ind = nlls_ind - af::median(nlls_ind);
      nlls_ind = nlls_ind;

      // Output vector of points around max index
      af::array nlls_y = mf_resp_abs(max_idx - nlls_ind);
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
        d_sdr1_time_est.push_back(t_est);
        d_sdr1_phase_est.push_back(p_est);
      }
      else if (pmt::equal(src_val, PMT_HARMONIA_SDR2))
      {
        d_sdr2_time_est.push_back(t_est);
        d_sdr2_phase_est.push_back(p_est);
      }
      else if (pmt::equal(src_val, PMT_HARMONIA_SDR3))
      {
        d_sdr3_time_est.push_back(t_est);
        d_sdr3_phase_est.push_back(p_est);
      }
      else
      {
        GR_LOG_WARN(d_logger, "Unknown SDR source in metadata");
      }

      auto to_pmt_f32 = [&](const std::vector<float> &v)
      {
        return pmt::init_f32vector(v.size(), const_cast<float *>(v.data()));
      };

      // Wait for TDMA to fully finish before outputting all data
      if (pmt::equal(TDMA_val, pmt::PMT_T))
      {
        d_tp_meta = pmt::dict_add(d_tp_meta, PMT_HARMONIA_SDR1, to_pmt_f32(d_sdr1_time_est));
        d_tp_meta = pmt::dict_add(d_tp_meta, PMT_HARMONIA_SDR2, to_pmt_f32(d_sdr2_time_est));
        d_tp_meta = pmt::dict_add(d_tp_meta, PMT_HARMONIA_SDR3, to_pmt_f32(d_sdr3_time_est));
        d_tp_meta = pmt::dict_add(d_tp_meta, PMT_HARMONIA_P_SDR1, to_pmt_f32(d_sdr1_phase_est));
        d_tp_meta = pmt::dict_add(d_tp_meta, PMT_HARMONIA_P_SDR2, to_pmt_f32(d_sdr2_phase_est));
        d_tp_meta = pmt::dict_add(d_tp_meta, PMT_HARMONIA_P_SDR3, to_pmt_f32(d_sdr3_phase_est));
        GR_LOG_WARN(d_logger, "TDMA done");
        // Send the frequency estimates as a message with metadata
        message_port_pub(d_tp_out_port, d_tp_meta);
      }

      // Reset the metadata output
      d_meta = pmt::make_dict();
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
