#ifndef PMT_HARMONIA_CONSTANTS
#define PMT_HARMONIA_CONSTANTS
#include <pmt/pmt.h>

// GNU-radio specific
static const pmt::pmt_t PMT_HARMONIA_IN = pmt::intern("in");
static const pmt::pmt_t PMT_HARMONIA_OUT = pmt::intern("out");
static const pmt::pmt_t PMT_HARMONIA_TX = pmt::intern("tx");
static const pmt::pmt_t PMT_HARMONIA_RX = pmt::intern("rx");
static const pmt::pmt_t PMT_HARMONIA_PDU = pmt::intern("pdu");
static const pmt::pmt_t PMT_HARMONIA_F_OUT = pmt::intern("f_out");

// SigMF core
static const pmt::pmt_t PMT_HARMONIA_GLOBAL = pmt::intern("global");
static const pmt::pmt_t PMT_HARMONIA_CAPTURES = pmt::intern("captures");
static const pmt::pmt_t PMT_HARMONIA_ANNOTATIONS = pmt::intern("annotations");
static const pmt::pmt_t PMT_HARMONIA_DATATYPE = pmt::intern("core:datatype");
static const pmt::pmt_t PMT_HARMONIA_VERSION = pmt::intern("core:version");
static const pmt::pmt_t PMT_HARMONIA_SAMPLE_RATE = pmt::intern("core:sample_rate");
static const pmt::pmt_t PMT_HARMONIA_LABEL = pmt::intern("core:label");
static const pmt::pmt_t PMT_HARMONIA_FREQUENCY = pmt::intern("core:frequency");
static const pmt::pmt_t PMT_HARMONIA_SAMPLE_START = pmt::intern("core:sample_start");
static const pmt::pmt_t PMT_HARMONIA_SAMPLE_COUNT = pmt::intern("core:sample_count");

// Radar extension
static const pmt::pmt_t PMT_HARMONIA_RADAR_DETAIL = pmt::intern("radar:detail");
static const pmt::pmt_t PMT_HARMONIA_BANDWIDTH = pmt::intern("radar:bandwidth");
static const pmt::pmt_t PMT_HARMONIA_RADAR_FREQUENCY = pmt::intern("radar:frequency");
static const pmt::pmt_t PMT_HARMONIA_PHASE = pmt::intern("radar:phase");
static const pmt::pmt_t PMT_HARMONIA_DURATION = pmt::intern("radar:duration");
static const pmt::pmt_t PMT_HARMONIA_PRF = pmt::intern("radar:prf");
static const pmt::pmt_t PMT_HARMONIA_FFT_SIZE = pmt::intern("radar:fft_size");

// Synchronization extension
static const pmt::pmt_t PMT_HARMONIA_F_EST = pmt::intern("sync:f_est");
static const pmt::pmt_t PMT_HARMONIA_T_EST = pmt::intern("sync:t_est");
// Add more if needed
static const pmt::pmt_t PMT_HARMONIA_SDR1 = pmt::intern("sdr1");
static const pmt::pmt_t PMT_HARMONIA_SDR2 = pmt::intern("sdr2");
static const pmt::pmt_t PMT_HARMONIA_SDR3 = pmt::intern("sdr3");
static const pmt::pmt_t PMT_HARMONIA_SDR4 = pmt::intern("sdr4");
static const pmt::pmt_t PMT_HARMONIA_SDR5 = pmt::intern("sdr5");

#endif /* PMT_HARMONIA_CONSTANTS */
