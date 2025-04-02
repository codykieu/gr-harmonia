/* -*- c++ -*- */
/*
 * Copyright 2025 Cody Kieu.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_HARMONIA_DEVICE_H
#define INCLUDED_HARMONIA_DEVICE_H

#include <gnuradio/harmonia/api.h>

namespace gr {
namespace harmonia {

/*!
 * \brief <+description+>
 *
 */
class HARMONIA_API Device
{
public:
    enum Backend { DEFAULT, CPU, CUDA, OPENCL };
};

} // namespace harmonia
} // namespace gr

#endif /* INCLUDED_HARMONIA_DEVICE_H */
