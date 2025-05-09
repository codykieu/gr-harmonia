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

        frequency_pk_est::sptr frequency_pk_est::make(size_t nfft,
                                                      double pulse_width,
                                                      double cap_length,
                                                      double samp_rate,
                                                      double NLLS_iter,
                                                      double NLLS_pts,
                                                      bool enable_out)
        {
            return gnuradio::make_block_sptr<frequency_pk_est_impl>(
                nfft, pulse_width, cap_length, samp_rate, NLLS_iter, NLLS_pts, enable_out);
        }

        /*
         * The private constructor
         */
        frequency_pk_est_impl::frequency_pk_est_impl(size_t nfft,
                                                     double pulse_width,
                                                     double cap_length,
                                                     double samp_rate,
                                                     double NLLS_iter,
                                                     double NLLS_pts,
                                                     bool enable_out)
            : gr::block("frequency_pk_est",
                        gr::io_signature::make(0, 0, 0),
                        gr::io_signature::make(0, 0, 0)),
              d_fftsize(nfft),
              d_pulse_width(pulse_width),
              d_cap_length(cap_length),
              d_samp_rate(samp_rate),
              d_NLLS_iter(NLLS_iter),
              d_NLLS_pts(NLLS_pts),
              d_enable_out(enable_out)
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
        af::array sinc(const af::array &x)
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

            // Check which SDR TX the signal
            pmt::pmt_t src_val = pmt::dict_ref(d_meta, pmt::intern("src"), pmt::PMT_NIL);
            std::cout << "src_val in freq block: " << pmt::write_string(src_val) << std::endl;
            pmt::pmt_t TDMA_val = pmt::dict_ref(d_meta, pmt::intern("TDMA_Done"), pmt::PMT_NIL);

            // Retrieves length of samples
            size_t n = pmt::length(samples);
            // GR_LOG_INFO(d_logger, "Received PDU with length: " + std::to_string(n));
            double NFFT = d_fftsize * n;
            const std::complex<float> *in_data = pmt::c32vector_elements(samples, n);

            // Calculating absolute FFT of input data
            af::array af_input(n, reinterpret_cast<const af::cfloat *>(in_data));
            af::array af_fft = af::fftNorm(af_input, 1.0, NFFT);
            // FFTshift to zero
            af_fft = ::plasma::fftshift(af_fft, 0);
            af::array af_abs_fft = af::abs(af_fft);
            // Log FFT data length
            // size_t fft_length = af_abs_fft.elements();
            // GR_LOG_INFO(d_logger, "Length of af_abs_fft: " + std::to_string(fft_length));

            // Output absolute fft of input data
            size_t io(0);
            d_data = pmt::make_f32vector(NFFT, 0);
            float *out = pmt::f32vector_writable_elements(d_data, io);
            af_abs_fft.host(out);

            if (d_enable_out)
            {
                // Send the data as a message
                message_port_pub(d_out_port, pmt::cons(d_meta, d_data));
            }

            // Output max and index of data
            double max_fft;
            unsigned max_idx;
            af::max(&max_fft, &max_idx, af_fft);

            // Generate frequency axis
            af::array f_axis =
                (-d_samp_rate / 2.0) + ((af::seq(0, NFFT - 1)) * (d_samp_rate / (NFFT)));

            af::array f_pk = f_axis(max_idx); // Get the value at max_idx as an af::array

            // ----------------- Sinc-NLLS -----------------
            // Create lambda array
            double lambda[] = {max_fft, 0.0, d_pulse_width / d_cap_length};
            af::array af_lambda(3, lambda, afHost);

            // Making Index for NLLS
            af::array nlls_ind = af::seq(0.0, d_NLLS_pts - 1.0);
            nlls_ind = nlls_ind - af::median(nlls_ind);
            nlls_ind = nlls_ind;

            // Output vector of points around max index
            af::array nlls_y = af_abs_fft(max_idx - nlls_ind);
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
                af::replace(nlls_J, !(af::isInf(nlls_J)), 0.0);

                //  Residual errors
                af::array nlls_delty = nlls_y - nlls_f;

                // Parameter
                af::array nlls_deltlambda = af::matmul((af::matmul((af::inverse(af::matmul(nlls_J.T(), nlls_J))), nlls_J.T())), nlls_delty);

                // Estimate
                af_lambda = af_lambda + nlls_deltlambda;
                af_lambda.host(lambda);
            }

            // Compute frequency estimate
            af::array f_est_arr = f_pk + (af_lambda(1) / d_cap_length);

            // Extract scalar value from ArrayFire array
            f_est_arr.host(&f_est);

            if (pmt::equal(src_val, PMT_HARMONIA_SDR1))
            {
                GR_LOG_WARN(d_logger, "SDR1 Found");
                d_sdr1_estimates.push_back(f_est);
            }
            else if (pmt::equal(src_val, PMT_HARMONIA_SDR2))
            {
                GR_LOG_WARN(d_logger, "SDR2 Found");
                d_sdr2_estimates.push_back(f_est);
            }
            else if (pmt::equal(src_val, PMT_HARMONIA_SDR3))
            {
                GR_LOG_WARN(d_logger, "SDR3 Found");
                d_sdr3_estimates.push_back(f_est);
            }
            else
            {
                GR_LOG_WARN(d_logger, "Unknown SDR source in metadata");
            }

            auto to_pmt_f32 = [&](const std::vector<float>& v){
                return pmt::init_f32vector(v.size(), const_cast<float*>(v.data()));
            };

            // Wait for TDMA to fully finish before outputting all data
            if (pmt::equal(TDMA_val, pmt::PMT_T))
            {
                d_meta_f = pmt::dict_add(d_meta_f, PMT_HARMONIA_SDR1, to_pmt_f32(d_sdr1_estimates));
                d_meta_f = pmt::dict_add(d_meta_f, PMT_HARMONIA_SDR2, to_pmt_f32(d_sdr2_estimates));
                d_meta_f = pmt::dict_add(d_meta_f, PMT_HARMONIA_SDR3, to_pmt_f32(d_sdr3_estimates));

                GR_LOG_WARN(d_logger, "TDMA done");
                // Send the frequency estimates as a message with metadata
                message_port_pub(d_f_out_port, d_meta_f);
            }
            else
            {
                GR_LOG_WARN(d_logger, "TDMA not done, unable to publish metadata");
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
