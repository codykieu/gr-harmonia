/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "frequency_pk_est_impl.h"
#include <gnuradio/io_signature.h>
#include <arrayfire.h>
#include <cmath>
#include <iomanip>

namespace gr
{
    namespace harmonia
    {

        frequency_pk_est::sptr frequency_pk_est::make(size_t fft_ratio,
                                                      double pulse_width,
                                                      double cap_length,
                                                      double samp_rate,
                                                      double NLLS_iter,
                                                      int sdr_id,
                                                      bool enable_out)
        {
            return gnuradio::make_block_sptr<frequency_pk_est_impl>(
                fft_ratio, pulse_width, cap_length, samp_rate, NLLS_iter, sdr_id, enable_out);
        }

        /*
         * The private constructor
         */
        frequency_pk_est_impl::frequency_pk_est_impl(size_t fft_ratio,
                                                     double pulse_width,
                                                     double cap_length,
                                                     double samp_rate,
                                                     double NLLS_iter,
                                                     int sdr_id,
                                                     bool enable_out)
            : gr::block("frequency_pk_est",
                        gr::io_signature::make(0, 0, 0),
                        gr::io_signature::make(0, 0, 0)),
              fft_ratio(fft_ratio),
              pulse_width(pulse_width),
              cap_length(cap_length),
              samp_rate(samp_rate),
              NLLS_iter(NLLS_iter),
              sdr_id(sdr_id),
              enable_out(enable_out),
              d_rx_count(0)
        {
            d_in_port = PMT_HARMONIA_IN;
            d_f_out_port = PMT_HARMONIA_F_OUT;
            d_meta = pmt::make_dict();
            d_data = pmt::make_c32vector(0, 0);
            d_meta_f = pmt::make_dict();
            message_port_register_in(d_in_port);
            if (enable_out)
            {
                d_out_port = PMT_HARMONIA_OUT;
                message_port_register_out(d_out_port);
            }
            message_port_register_out(d_f_out_port);
            set_msg_handler(d_in_port, [this](pmt::pmt_t msg)
                            { handle_msg(msg); });
        }

        /*
         * Our virtual destructor.
         */
        frequency_pk_est_impl::~frequency_pk_est_impl() {}

        // Sinc Function
        af::array frequency_pk_est_impl::sinc(const af::array &x)
        {
            return af::select(x == 0.0, 1.0, af::sin(af::Pi * x) / (af::Pi * x));
        }

        void frequency_pk_est_impl::handle_msg(pmt::pmt_t msg)
        {
            if (this->nmsgs(d_in_port) > d_queue_depth)
            {
                return;
            }

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

            // Retrieves length of samples
            size_t n = pmt::length(samples);
            // GR_LOG_INFO(d_logger, "Received PDU with length: " + std::to_string(n));
            double NFFT = fft_ratio * n;
            // GR_LOG_INFO(d_logger, "NFFT: " + std::to_string(NFFT));

            // Casting data into array
            const std::complex<float> *in_data = pmt::c32vector_elements(samples, n);
            af::array af_input = af::array(n, reinterpret_cast<const af::cfloat *>(in_data));

            // FFT
            af::array af_fft = af::fft(af_input, NFFT);

            // FFTshift to zero
            af_fft = ::plasma::fftshift(af_fft, 0);
            af::array af_abs_fft = af::abs(af_fft);

            // Output absolute fft of input data
            size_t io(0);
            d_data = pmt::make_f32vector(NFFT, 0);
            float *out = pmt::f32vector_writable_elements(d_data, io);
            af_abs_fft.host(out);

            // Send the data as a message
            if (enable_out)
            {
                message_port_pub(d_out_port, pmt::cons(d_meta, d_data));
            }

            // Output max and index of data
            double max_fft;
            unsigned max_idx;
            af::max(&max_fft, &max_idx, af_abs_fft);

            // Generate frequency axis
            af::array f_axis =
                (-samp_rate / 2.0) + ((af::seq(0, NFFT - 1)) * (samp_rate / (NFFT)));

            af::array f_pk = f_axis(max_idx); // Get the value at max_idx as an af::array
            // af_print(f_pk);

            // ----------------- Sinc-NLLS -----------------
            // Create lambda array
            double lambda[] = {max_fft, 0.0, pulse_width / cap_length};
            af::array af_lambda(3, lambda, afHost);

            // Making Index for NLLS
            double NLLS_pts = std::ceil(fft_ratio * 2.0 * pulse_width / cap_length) - 1.0;

            // Minimum Number of Points = 3
            if (NLLS_pts < 5.0)
                NLLS_pts = 5.0;
            // GR_LOG_INFO(d_logger, "NLLS Points: " + std::to_string(NLLS_pts));

            // NLLS Index Vector
            af::array nlls_ind = af::seq(0.0, NLLS_pts - 1.0);
            nlls_ind = nlls_ind - af::median(nlls_ind);
            nlls_ind = nlls_ind.as(f64);
            // af_print(nlls_ind);
            af::array max_vector = af::constant(static_cast<double>(max_idx), static_cast<dim_t>(NLLS_pts));

            // Output vector of points around max index
            af::array nlls_y = af_abs_fft(max_vector - nlls_ind);
            nlls_y = nlls_y.as(f64);
            // af_print(nlls_y);

            // Iterates through NLLS x times
            for (int iter = 0; iter < NLLS_iter; ++iter)
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
            // af_print(af_lambda);

            // Compute frequency estimate
            af::array f_est_arr = f_pk + (af_lambda(1) / (cap_length * fft_ratio));
            // af_print(f_est_arr);

            // Output frequency estimate
            f_est_arr.host(&f_est);

            if (d_rx_count < 2)
            {
                switch (sdr_id)
                {
                case 1:
                    if (d_rx_count == 0)
                        d_sdr2_estimates.push_back(f_est);
                    else
                        d_sdr3_estimates.push_back(f_est);
                    break;
                case 2:
                    if (d_rx_count == 0)
                        d_sdr1_estimates.push_back(f_est);
                    else
                        d_sdr3_estimates.push_back(f_est);
                    break;
                case 3:
                    if (d_rx_count == 0)
                        d_sdr1_estimates.push_back(f_est);
                    else
                        d_sdr2_estimates.push_back(f_est);
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
                d_meta_f = pmt::dict_add(d_meta_f, PMT_HARMONIA_SDR1, to_pmt_f32(d_sdr1_estimates));
                d_meta_f = pmt::dict_add(d_meta_f, PMT_HARMONIA_SDR2, to_pmt_f32(d_sdr2_estimates));
                d_meta_f = pmt::dict_add(d_meta_f, PMT_HARMONIA_SDR3, to_pmt_f32(d_sdr3_estimates));
                d_meta_f = pmt::dict_add(d_meta_f, pmt::intern("rx_id"), sdr_pmt);

                pmt::pmt_t empty_payload = pmt::make_u8vector(0, 0);
                pmt::pmt_t pdu = pmt::cons(d_meta_f, empty_payload);
                message_port_pub(d_f_out_port, pdu);
            }

            // Reset the metadata output
            d_meta = pmt::make_dict();
        }

        void frequency_pk_est_impl::set_msg_queue_depth(size_t depth) { d_queue_depth = depth; }

        void frequency_pk_est_impl::set_backend(Device::Backend backend)
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
