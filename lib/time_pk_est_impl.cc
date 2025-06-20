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

    time_pk_est::sptr time_pk_est::make(double samp_rate,
                                        double bandwidth,
                                        double NLLS_iter,
                                        int sdr_id)
    {
      return gnuradio::make_block_sptr<time_pk_est_impl>(samp_rate,
                                                         bandwidth,
                                                         NLLS_iter,
                                                         sdr_id);
    }

    /*
     * The private constructor
     */
    time_pk_est_impl::time_pk_est_impl(double samp_rate,
                                       double bandwidth,
                                       double NLLS_iter,
                                       int sdr_id)
        : gr::block("time_pk_est",
                    gr::io_signature::make(0, 0, 0),
                    gr::io_signature::make(0, 0, 0)),
          samp_rate(samp_rate),
          bandwidth(bandwidth),
          NLLS_iter(NLLS_iter),
          sdr_id(sdr_id),
          d_rx_count(0)
    {
      d_data = pmt::make_c32vector(0, 0);
      d_meta = pmt::make_dict();
      d_tp_meta = pmt::make_dict();
      // Input Ports
      d_tx_port = PMT_HARMONIA_TX;
      d_rx_port = PMT_HARMONIA_RX;
      // Output Ports
      d_out_port = PMT_HARMONIA_OUT;
      d_tp_out_port = PMT_HARMONIA_TP_OUT;
      message_port_register_in(d_tx_port);
      message_port_register_in(d_rx_port);
      message_port_register_out(d_tp_out_port);
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

      // Set SDR PMT
      switch (sdr_id)
      {
      case 1:
        sdr_pmt = PMT_HARMONIA_SDR1;
        break;
      case 2:
        sdr_pmt = PMT_HARMONIA_SDR2;
        break;
      case 3:
        sdr_pmt = PMT_HARMONIA_SDR3;
        break;
      }

      // Check for 'rx_time'
      pmt::pmt_t rx_time_key = pmt::intern("rx_time");
      if (pmt::dict_has_key(d_meta, rx_time_key))
      {
        pmt::pmt_t rx_time_val = pmt::dict_ref(d_meta, rx_time_key, pmt::PMT_NIL);
        if (pmt::is_real(rx_time_val))
        {
          rx_time = pmt::to_double(rx_time_val);
          // std::cout << "rx_time = " << rx_time << std::endl;
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

      // Apply the matched filter
      af::array mf_resp(af::dim4(n), reinterpret_cast<const af::cfloat *>(in));
      mf_resp = af::convolve1(mf_resp, d_match_filt, AF_CONV_EXPAND, AF_CONV_AUTO);
      af::array mf_resp_abs = af::abs(mf_resp);
      // std::cout << "Length of mf = " << mf_resp_abs.elements() << std::endl;
      // std::cout << "Length of nconv = " << nconv << std::endl;

      d_data = pmt::make_c32vector(nconv, gr_complex{0, 0});
      size_t out_io = 0;
      gr_complex *out = pmt::c32vector_writable_elements(d_data, out_io);
      mf_resp.host(reinterpret_cast<af::cfloat *>(out));
      message_port_pub(d_out_port, pmt::cons(d_meta, d_data));

      // Output max and index of data
      double max_val;
      unsigned max_idx;
      af::max(&max_val, &max_idx, mf_resp_abs);

      // Phase Estimates
      af::array complex_peak = mf_resp(max_idx);
      af::array phase = af::arg(complex_peak);
      float p_est = phase.scalar<float>();
      // std::cout << "Phase: " << p_est << std::endl;

      // Generate time axis
      af::array t = (af::seq(0, nconv - 1)) * 1.0 / samp_rate;

      af::array t_pk = t(max_idx);
      // af_print(t_pk);

      // ----------------- Sinc-NLLS -----------------
      // Create lambda array
      double lambda[] = {max_val, 0.0, bandwidth / samp_rate};
      af::array af_lambda(3, lambda, afHost);

      // Making Index for NLLS
      double NLLS_pts = std::ceil(2.0 * samp_rate / bandwidth) - 1.0;

      // Minimum Number of Points = 5
      if (NLLS_pts < 7.0)
        NLLS_pts = 7.0;
      // GR_LOG_INFO(d_logger, "NLLS Points: " + std::to_string(NLLS_pts));

      // NLLS Infex Vector
      af::array nlls_ind = af::seq(0.0, NLLS_pts - 1.0);
      nlls_ind = nlls_ind - af::median(nlls_ind);
      nlls_ind = nlls_ind.as(f64);
      af::array max_vector = af::constant(static_cast<double>(max_idx), static_cast<dim_t>(NLLS_pts));

      // Output vector of points around max index
      af::array nlls_y = mf_resp_abs(max_vector - nlls_ind);
      nlls_y = nlls_y;

      // Iterates through NLLS x times
      for (int i = 0; i < NLLS_iter; i++)
      {
        af::array x = nlls_ind - af_lambda(1);
        af::array z = x * af_lambda(2);
        af::array sinc_z = sinc(z);
        af::array cos_z = af::cos(af::Pi * z);
        af::array nlls_f = af_lambda(0) * sinc_z;

        // Jacobian Matrix
        af::array GN1 = sinc_z;
        af::array GN2 = af_lambda(0) * (sinc_z - cos_z) / (x);
        af::array GN3 = af_lambda(0) * x * (cos_z - sinc_z) / af_lambda(2);
        af::array J = af::join(1, GN1, GN2, GN3);

        // Replace Infinite and NaNs with zeros
        af::replace(J, !(af::isInf(J)), 0.0);
        af::replace(J, !(af::isNaN(J)), 0.0);

        // Residual
        af::array r = nlls_y - nlls_f;
        // Solve for lambda
        af::array JTJ = af::matmulTN(J, J);
        af::array JTr = af::matmulTN(J, r);
        af::array delta = af::solve(JTJ, JTr);

        af_lambda += delta;
      }
      // Output Lambda
      af_lambda.host(lambda);

      // Compute frequency estimate
      af::array t_est_arr = t_pk + (af_lambda(1) / samp_rate);

      // af_print(t_est_arr);

      // Extract scalar value from ArrayFire array
      t_est_arr.host(&t_est);

      if (d_rx_count < 2)
      {
        switch (sdr_id)
        {
        case 1:
          if (d_rx_count == 0)
          {
            d_sdr2_time_est.push_back(t_est);
            d_sdr2_phase_est.push_back(p_est);
          }
          else
          {
            d_sdr3_time_est.push_back(t_est);
            d_sdr3_phase_est.push_back(p_est);
          }
          break;
        case 2:
          if (d_rx_count == 0)
          {
            d_sdr1_time_est.push_back(t_est);
            d_sdr1_phase_est.push_back(p_est);
          }
          else
          {
            d_sdr3_time_est.push_back(t_est);
            d_sdr3_phase_est.push_back(p_est);
          }
          break;
        case 3:
          if (d_rx_count == 0)
          {
            d_sdr1_time_est.push_back(t_est);
            d_sdr1_phase_est.push_back(p_est);
          }
          else
          {
            d_sdr2_time_est.push_back(t_est);
            d_sdr2_phase_est.push_back(p_est);
          }
          break;
        }
      }
      d_rx_count++;

      auto to_pmt_f32 = [&](const std::vector<float> &v)
      {
        return pmt::init_f32vector(v.size(), const_cast<float *>(v.data()));
      };

      if (d_rx_count == 2)
      {
        d_tp_meta = pmt::dict_add(d_tp_meta, PMT_HARMONIA_SDR1, to_pmt_f32(d_sdr1_time_est));
        d_tp_meta = pmt::dict_add(d_tp_meta, PMT_HARMONIA_P_SDR1, to_pmt_f32(d_sdr1_phase_est));
        d_tp_meta = pmt::dict_add(d_tp_meta, PMT_HARMONIA_SDR2, to_pmt_f32(d_sdr2_time_est));
        d_tp_meta = pmt::dict_add(d_tp_meta, PMT_HARMONIA_P_SDR2, to_pmt_f32(d_sdr2_phase_est));
        d_tp_meta = pmt::dict_add(d_tp_meta, PMT_HARMONIA_SDR3, to_pmt_f32(d_sdr3_time_est));
        d_tp_meta = pmt::dict_add(d_tp_meta, PMT_HARMONIA_P_SDR3, to_pmt_f32(d_sdr3_phase_est));
        d_tp_meta = pmt::dict_add(d_tp_meta, PMT_HARMONIA_P_SDR3, to_pmt_f32(d_sdr3_phase_est));
        d_tp_meta = pmt::dict_add(d_tp_meta, pmt::intern("rx_id"), sdr_pmt);

        // Send the time estimates as a message with metadata
        pmt::pmt_t empty_payload = pmt::make_u8vector(0, 0);
        pmt::pmt_t pdu = pmt::cons(d_tp_meta, empty_payload);

        // Now publish that PDU:
        message_port_pub(d_tp_out_port, pdu);
      }

      // Reset the metadata output
      d_tp_meta = pmt::make_dict();
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
