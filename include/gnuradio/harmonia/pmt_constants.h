#ifndef PMT_HARMONIA_CONSTANTS
#define PMT_HARMONIA_CONSTANTS
#include <pmt/pmt.h>

// GNU-radio specific
static const pmt::pmt_t PMT_HARMONIA_IN = pmt::intern("in");
static const pmt::pmt_t PMT_HARMONIA_IN2 = pmt::intern("in2");
static const pmt::pmt_t PMT_HARMONIA_IN3 = pmt::intern("in3");
static const pmt::pmt_t PMT_HARMONIA_LFM_IN = pmt::intern("LFM_in");
static const pmt::pmt_t PMT_HARMONIA_LFM_IN2 = pmt::intern("LFM_in2");
static const pmt::pmt_t PMT_HARMONIA_LFM_IN3 = pmt::intern("LFM_in3");
static const pmt::pmt_t PMT_HARMONIA_OUT = pmt::intern("out");
static const pmt::pmt_t PMT_HARMONIA_OUT2 = pmt::intern("out2");
static const pmt::pmt_t PMT_HARMONIA_OUT3 = pmt::intern("out3");
static const pmt::pmt_t PMT_HARMONIA_CD_IN = pmt::intern("cd_in");
static const pmt::pmt_t PMT_HARMONIA_CB_IN = pmt::intern("cb_in");
static const pmt::pmt_t PMT_HARMONIA_CP_IN = pmt::intern("cp_in");
static const pmt::pmt_t PMT_HARMONIA_CD_OUT = pmt::intern("cd_out");
static const pmt::pmt_t PMT_HARMONIA_CD_OUT2 = pmt::intern("cd_out2");
static const pmt::pmt_t PMT_HARMONIA_CD_OUT3 = pmt::intern("cd_out3");
static const pmt::pmt_t PMT_HARMONIA_CB_OUT = pmt::intern("cb_out");
static const pmt::pmt_t PMT_HARMONIA_CB_OUT2 = pmt::intern("cb_out2");
static const pmt::pmt_t PMT_HARMONIA_CB_OUT3 = pmt::intern("cb_out3");
static const pmt::pmt_t PMT_HARMONIA_CP_OUT = pmt::intern("cp_out");
static const pmt::pmt_t PMT_HARMONIA_CP_OUT2 = pmt::intern("cp_out2");
static const pmt::pmt_t PMT_HARMONIA_CP_OUT3 = pmt::intern("cp_out3");
static const pmt::pmt_t PMT_HARMONIA_TX = pmt::intern("tx");
static const pmt::pmt_t PMT_HARMONIA_RX = pmt::intern("rx");
static const pmt::pmt_t PMT_HARMONIA_PDU = pmt::intern("pdu");
static const pmt::pmt_t PMT_HARMONIA_F_OUT = pmt::intern("f_out");
static const pmt::pmt_t PMT_HARMONIA_T_OUT = pmt::intern("t_out");
static const pmt::pmt_t PMT_HARMONIA_P_OUT = pmt::intern("p_out");
static const pmt::pmt_t PMT_HARMONIA_TP_OUT = pmt::intern("tp_out");

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
static const pmt::pmt_t PMT_HARMONIA_SDR1 = pmt::intern("sdr1");
static const pmt::pmt_t PMT_HARMONIA_SDR2 = pmt::intern("sdr2");
static const pmt::pmt_t PMT_HARMONIA_SDR3 = pmt::intern("sdr3");
static const pmt::pmt_t PMT_HARMONIA_CB_SDR1 = pmt::intern("cb_sdr1");
static const pmt::pmt_t PMT_HARMONIA_CB_SDR2 = pmt::intern("cb_sdr2");
static const pmt::pmt_t PMT_HARMONIA_CB_SDR3 = pmt::intern("cb_sdr3");
static const pmt::pmt_t PMT_HARMONIA_P_SDR1 = pmt::intern("phase_sdr1");
static const pmt::pmt_t PMT_HARMONIA_P_SDR2 = pmt::intern("phase_sdr2");
static const pmt::pmt_t PMT_HARMONIA_P_SDR3 = pmt::intern("phase_sdr3");
static const pmt::pmt_t PMT_HARMONIA_CP_TX_SDR1 = pmt::intern("cp_tx_sdr1");
static const pmt::pmt_t PMT_HARMONIA_CP_TX_SDR2 = pmt::intern("cp_tx_sdr2");
static const pmt::pmt_t PMT_HARMONIA_CP_TX_SDR3 = pmt::intern("cp_tx_sdr3");
static const pmt::pmt_t PMT_HARMONIA_CP_RX_SDR1 = pmt::intern("cp_rx_sdr1");
static const pmt::pmt_t PMT_HARMONIA_CP_RX_SDR2 = pmt::intern("cp_rx_sdr2");
static const pmt::pmt_t PMT_HARMONIA_CP_RX_SDR3 = pmt::intern("cp_rx_sdr3");
#endif /* PMT_HARMONIA_CONSTANTS */
