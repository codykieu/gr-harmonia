#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Not titled yet
# Author: cody
# GNU Radio version: 3.10.12.0

from PyQt5 import Qt
from gnuradio import qtgui
from gnuradio import gr
from gnuradio.filter import firdes
from gnuradio.fft import window
import sys
import signal
from PyQt5 import Qt
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
from gnuradio import harmonia
from gnuradio import plasma
import sip
import threading



class time_peak_estimation_test(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Not titled yet", catch_exceptions=True)
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Not titled yet")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except BaseException as exc:
            print(f"Qt GUI: Could not set Icon: {str(exc)}", file=sys.stderr)
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("gnuradio/flowgraphs", "time_peak_estimation_test")

        try:
            geometry = self.settings.value("geometry")
            if geometry:
                self.restoreGeometry(geometry)
        except BaseException as exc:
            print(f"Qt GUI: Could not restore geometry: {str(exc)}", file=sys.stderr)
        self.flowgraph_started = threading.Event()

        ##################################################
        # Variables
        ##################################################
        self.tdma_time2 = tdma_time2 = 100e-6
        self.tdma_time = tdma_time = 15e-3
        self.samp_rate = samp_rate = 100e6
        self.run_id = run_id = 0
        self.prf = prf = 0
        self.center_freq = center_freq = 3e9
        self.bw = bw = 25e6
        self.baseband_freq = baseband_freq = 10e6
        self.Tw2 = Tw2 = 10e-6
        self.Tw = Tw = 4e-3
        self.Tp2 = Tp2 = 1e-3
        self.Tp = Tp = 1e-3
        self.Tc2 = Tc2 = 100e-6
        self.Tc = Tc = 9e-3

        ##################################################
        # Blocks
        ##################################################

        self.qtgui_time_sink_x_0 = qtgui.time_sink_f(
            1024, #size
            samp_rate, #samp_rate
            "", #name
            0, #number of inputs
            None # parent
        )
        self.qtgui_time_sink_x_0.set_update_time(0.10)
        self.qtgui_time_sink_x_0.set_y_axis(-1, 1)

        self.qtgui_time_sink_x_0.set_y_label('Amplitude', "")

        self.qtgui_time_sink_x_0.enable_tags(True)
        self.qtgui_time_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, qtgui.TRIG_SLOPE_POS, 0.0, 0, 0, "")
        self.qtgui_time_sink_x_0.enable_autoscale(False)
        self.qtgui_time_sink_x_0.enable_grid(False)
        self.qtgui_time_sink_x_0.enable_axis_labels(True)
        self.qtgui_time_sink_x_0.enable_control_panel(False)
        self.qtgui_time_sink_x_0.enable_stem_plot(False)


        labels = ['Signal 1', 'Signal 2', 'Signal 3', 'Signal 4', 'Signal 5',
            'Signal 6', 'Signal 7', 'Signal 8', 'Signal 9', 'Signal 10']
        widths = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        colors = ['blue', 'red', 'green', 'black', 'cyan',
            'magenta', 'yellow', 'dark red', 'dark green', 'dark blue']
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
            1.0, 1.0, 1.0, 1.0, 1.0]
        styles = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        markers = [-1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1]


        for i in range(1):
            if len(labels[i]) == 0:
                self.qtgui_time_sink_x_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_time_sink_x_0.set_line_label(i, labels[i])
            self.qtgui_time_sink_x_0.set_line_width(i, widths[i])
            self.qtgui_time_sink_x_0.set_line_color(i, colors[i])
            self.qtgui_time_sink_x_0.set_line_style(i, styles[i])
            self.qtgui_time_sink_x_0.set_line_marker(i, markers[i])
            self.qtgui_time_sink_x_0.set_line_alpha(i, alphas[i])

        self._qtgui_time_sink_x_0_win = sip.wrapinstance(self.qtgui_time_sink_x_0.qwidget(), Qt.QWidget)
        self.top_layout.addWidget(self._qtgui_time_sink_x_0_win)
        self.plasma_pdu_file_sink_0_0_3_0_0_0_2 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/pulse2_comp1', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_3_0_0_0_1_1_1 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/final_rx_comp3', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_3_0_0_0_1_1_0 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/final_rx_comp2', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_3_0_0_0_1_1 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/final_rx_comp1', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_3_0_0_0_1_0 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/pulse2_comp3', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_3_0_0_0_1 = plasma.pdu_file_sink(gr.sizeof_float,'/home/cody/gr-MATLAB/pulse_comp3', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_3_0_0_0_0_0 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/pulse2_comp2', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_3_0_0_0_0 = plasma.pdu_file_sink(gr.sizeof_float,'/home/cody/gr-MATLAB/pulse_comp2', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_3_0_0_0 = plasma.pdu_file_sink(gr.sizeof_float,'/home/cody/gr-MATLAB/pulse_comp1', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_3_0_0 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/lfm_post3', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_3_0 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/lfm3', '/home/cody/gr-MATLAB/meta_test3')
        self.plasma_pdu_file_sink_0_0_3 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/st3', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_2_0_1_1 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/post_fd3', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_2_0_1_0 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/post_fd2', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_2_0_1 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/post_fd1', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_2_0_0 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/lfm_post2', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_2_0 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/lfm2', '/home/cody/gr-MATLAB/meta_test2')
        self.plasma_pdu_file_sink_0_0_2 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/st2', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_1_0 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/lfm_post1', '/home/cody/gr-MATLAB/TDMA_meta')
        self.plasma_pdu_file_sink_0_0_1 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/lfm1', '/home/cody/gr-MATLAB/meta_test')
        self.plasma_pdu_file_sink_0_0_0_2 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/waverform3', '/home/cody/gr-MATLAB/freq_pk_est_meta')
        self.plasma_pdu_file_sink_0_0_0_1 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/waverform2', '/home/cody/gr-MATLAB/freq_pk_est_meta')
        self.plasma_pdu_file_sink_0_0_0_0_0_0 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'', f'/home/cody/gr-MATLAB/range_test/range_est{run_id}')
        self.plasma_pdu_file_sink_0_0_0 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/waverform1', '/home/cody/gr-MATLAB/freq_pk_est_meta')
        self.plasma_pdu_file_sink_0_0 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'/home/cody/gr-MATLAB/st1', '/home/cody/gr-MATLAB/TDMA_meta')
        self.harmonia_usrp_radar_all_0 = harmonia.usrp_radar_all("addr=192.168.40.2,ignore-cal-file=0", "addr=192.168.60.2,ignore-cal-file=0", "addr=192.168.80.2,ignore-cal-file=0", samp_rate, samp_rate, samp_rate, center_freq, center_freq, center_freq, 20,
         20, 20, 1.5, Tc, Tc2, Tw, Tw2, tdma_time, tdma_time2, False)
        self.harmonia_usrp_radar_all_0.set_metadata_keys('core:tx_freq', 'core:rx_freq', 'core:sample_start', 'radar:prf')
        self.harmonia_time_pk_est_0_2 = harmonia.time_pk_est(samp_rate, bw, Tp2, Tw2, 81.6539, 10, 1)
        self.harmonia_time_pk_est_0_2.set_msg_queue_depth(1)
        self.harmonia_time_pk_est_0_2.set_backend(harmonia.Device.DEFAULT)
        self.harmonia_time_pk_est_0_1_0 = harmonia.time_pk_est(samp_rate, bw, Tp2, Tw2, 81.6490, 10, 3)
        self.harmonia_time_pk_est_0_1_0.set_msg_queue_depth(1)
        self.harmonia_time_pk_est_0_1_0.set_backend(harmonia.Device.DEFAULT)
        self.harmonia_time_pk_est_0_1 = harmonia.time_pk_est(samp_rate, bw, Tp, Tw, 81.6490, 10, 3)
        self.harmonia_time_pk_est_0_1.set_msg_queue_depth(1)
        self.harmonia_time_pk_est_0_1.set_backend(harmonia.Device.DEFAULT)
        self.harmonia_time_pk_est_0_0_0 = harmonia.time_pk_est(samp_rate, bw, Tp2, Tw2, 81.6506, 10, 2)
        self.harmonia_time_pk_est_0_0_0.set_msg_queue_depth(1)
        self.harmonia_time_pk_est_0_0_0.set_backend(harmonia.Device.DEFAULT)
        self.harmonia_time_pk_est_0_0 = harmonia.time_pk_est(samp_rate, bw, Tp, Tw, 81.6506, 10, 2)
        self.harmonia_time_pk_est_0_0.set_msg_queue_depth(1)
        self.harmonia_time_pk_est_0_0.set_backend(harmonia.Device.DEFAULT)
        self.harmonia_time_pk_est_0 = harmonia.time_pk_est(samp_rate, bw, Tp, Tw, 81.6539, 10, 1)
        self.harmonia_time_pk_est_0.set_msg_queue_depth(1)
        self.harmonia_time_pk_est_0.set_backend(harmonia.Device.DEFAULT)
        self.harmonia_single_tone_src_0_1 = harmonia.single_tone_src(baseband_freq, center_freq, 0, Tp, samp_rate, prf, 2)
        self.harmonia_single_tone_src_0_1.init_meta_dict('radar:frequency', 'radar:phase', 'radar:duration', 'core:sample_rate', 'core:label', 'radar:prf')
        self.harmonia_single_tone_src_0_0 = harmonia.single_tone_src(baseband_freq, center_freq, 0, Tp, samp_rate, prf, 1)
        self.harmonia_single_tone_src_0_0.init_meta_dict('radar:frequency', 'radar:phase', 'radar:duration', 'core:sample_rate', 'core:label', 'radar:prf')
        self.harmonia_single_tone_src_0 = harmonia.single_tone_src(baseband_freq, center_freq, 0, Tp, samp_rate, prf, 3)
        self.harmonia_single_tone_src_0.init_meta_dict('radar:frequency', 'radar:phase', 'radar:duration', 'core:sample_rate', 'core:label', 'radar:prf')
        self.harmonia_frequency_pk_est_0_1 = harmonia.frequency_pk_est(12, Tp, Tc, samp_rate, 10, 2, False)
        self.harmonia_frequency_pk_est_0_1.set_msg_queue_depth(1)
        self.harmonia_frequency_pk_est_0_1.set_backend(harmonia.Device.CPU)
        self.harmonia_frequency_pk_est_0_0 = harmonia.frequency_pk_est(12, Tp, Tc, samp_rate, 10, 3, False)
        self.harmonia_frequency_pk_est_0_0.set_msg_queue_depth(1)
        self.harmonia_frequency_pk_est_0_0.set_backend(harmonia.Device.CPU)
        self.harmonia_frequency_pk_est_0 = harmonia.frequency_pk_est(12, Tp, Tc, samp_rate, 10, 1, False)
        self.harmonia_frequency_pk_est_0.set_msg_queue_depth(1)
        self.harmonia_frequency_pk_est_0.set_backend(harmonia.Device.CPU)
        self.harmonia_compensation_0_2_0 = harmonia.compensation(center_freq, samp_rate, 1)
        self.harmonia_compensation_0_2 = harmonia.compensation(center_freq, samp_rate, 1)
        self.harmonia_compensation_0_1_0_0 = harmonia.compensation(center_freq, samp_rate, 3)
        self.harmonia_compensation_0_1_0 = harmonia.compensation(center_freq, samp_rate, 3)
        self.harmonia_compensation_0_1 = harmonia.compensation(center_freq, samp_rate, 3)
        self.harmonia_compensation_0_0_0_0 = harmonia.compensation(center_freq, samp_rate, 2)
        self.harmonia_compensation_0_0_0 = harmonia.compensation(center_freq, samp_rate, 2)
        self.harmonia_compensation_0_0 = harmonia.compensation(center_freq, samp_rate, 2)
        self.harmonia_compensation_0 = harmonia.compensation(center_freq, samp_rate, 1)
        self.harmonia_clockbias_phase_est_0_0 = harmonia.clockbias_phase_est(3, center_freq, samp_rate, Tp2, 0, False, True)
        self.harmonia_clockbias_phase_est_0 = harmonia.clockbias_phase_est(3, center_freq, samp_rate, Tp, 0, True, False)
        self.harmonia_clock_drift_est_0 = harmonia.clock_drift_est(3, baseband_freq, center_freq, samp_rate, Tp, 0)
        self.harmonia_LFM_src_0_1 = harmonia.LFM_src(bw, -bw/2, center_freq, Tp, Tp2, samp_rate, 0, 3)
        self.harmonia_LFM_src_0_1.init_meta_dict('radar:bandwidth', 'radar:duration', 'core:sample_rate', 'core:label', 'radar:prf')
        self.harmonia_LFM_src_0_0 = harmonia.LFM_src(bw, -bw/2, center_freq, Tp, Tp2, samp_rate, 0, 2)
        self.harmonia_LFM_src_0_0.init_meta_dict('radar:bandwidth', 'radar:duration', 'core:sample_rate', 'core:label', 'radar:prf')
        self.harmonia_LFM_src_0 = harmonia.LFM_src(bw, -bw/2, center_freq, Tp, Tp2, samp_rate, 0, 1)
        self.harmonia_LFM_src_0.init_meta_dict('radar:bandwidth', 'radar:duration', 'core:sample_rate', 'core:label', 'radar:prf')


        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.harmonia_LFM_src_0, 'out'), (self.harmonia_time_pk_est_0, 'tx'))
        self.msg_connect((self.harmonia_LFM_src_0, 'out'), (self.harmonia_time_pk_est_0_2, 'tx'))
        self.msg_connect((self.harmonia_LFM_src_0, 'out'), (self.harmonia_usrp_radar_all_0, 'LFM_in'))
        self.msg_connect((self.harmonia_LFM_src_0, 'out'), (self.plasma_pdu_file_sink_0_0_0, 'in'))
        self.msg_connect((self.harmonia_LFM_src_0_0, 'out'), (self.harmonia_time_pk_est_0_0, 'tx'))
        self.msg_connect((self.harmonia_LFM_src_0_0, 'out'), (self.harmonia_time_pk_est_0_0_0, 'tx'))
        self.msg_connect((self.harmonia_LFM_src_0_0, 'out'), (self.harmonia_usrp_radar_all_0, 'LFM_in2'))
        self.msg_connect((self.harmonia_LFM_src_0_0, 'out'), (self.plasma_pdu_file_sink_0_0_0_1, 'in'))
        self.msg_connect((self.harmonia_LFM_src_0_1, 'out'), (self.harmonia_time_pk_est_0_1, 'tx'))
        self.msg_connect((self.harmonia_LFM_src_0_1, 'out'), (self.harmonia_time_pk_est_0_1_0, 'tx'))
        self.msg_connect((self.harmonia_LFM_src_0_1, 'out'), (self.harmonia_usrp_radar_all_0, 'LFM_in3'))
        self.msg_connect((self.harmonia_LFM_src_0_1, 'out'), (self.plasma_pdu_file_sink_0_0_0_2, 'in'))
        self.msg_connect((self.harmonia_clock_drift_est_0, 'out'), (self.harmonia_LFM_src_0, 'cd_in'))
        self.msg_connect((self.harmonia_clock_drift_est_0, 'out'), (self.harmonia_LFM_src_0_0, 'cd_in'))
        self.msg_connect((self.harmonia_clock_drift_est_0, 'out'), (self.harmonia_LFM_src_0_1, 'cd_in'))
        self.msg_connect((self.harmonia_clock_drift_est_0, 'out'), (self.harmonia_clockbias_phase_est_0, 'cd_in'))
        self.msg_connect((self.harmonia_clock_drift_est_0, 'out'), (self.harmonia_clockbias_phase_est_0_0, 'cd_in'))
        self.msg_connect((self.harmonia_clock_drift_est_0, 'out'), (self.harmonia_compensation_0, 'cd_in'))
        self.msg_connect((self.harmonia_clock_drift_est_0, 'out'), (self.harmonia_compensation_0_0, 'cd_in'))
        self.msg_connect((self.harmonia_clock_drift_est_0, 'out'), (self.harmonia_compensation_0_0_0, 'cd_in'))
        self.msg_connect((self.harmonia_clock_drift_est_0, 'out'), (self.harmonia_compensation_0_0_0_0, 'cd_in'))
        self.msg_connect((self.harmonia_clock_drift_est_0, 'out'), (self.harmonia_compensation_0_1, 'cd_in'))
        self.msg_connect((self.harmonia_clock_drift_est_0, 'out'), (self.harmonia_compensation_0_1_0, 'cd_in'))
        self.msg_connect((self.harmonia_clock_drift_est_0, 'out'), (self.harmonia_compensation_0_1_0_0, 'cd_in'))
        self.msg_connect((self.harmonia_clock_drift_est_0, 'out'), (self.harmonia_compensation_0_2, 'cd_in'))
        self.msg_connect((self.harmonia_clock_drift_est_0, 'out'), (self.harmonia_compensation_0_2_0, 'cd_in'))
        self.msg_connect((self.harmonia_clockbias_phase_est_0, 'out'), (self.harmonia_compensation_0_0_0, 'cb_in'))
        self.msg_connect((self.harmonia_clockbias_phase_est_0, 'out'), (self.harmonia_compensation_0_0_0_0, 'cb_in'))
        self.msg_connect((self.harmonia_clockbias_phase_est_0, 'out'), (self.harmonia_compensation_0_1_0, 'cb_in'))
        self.msg_connect((self.harmonia_clockbias_phase_est_0, 'out'), (self.harmonia_compensation_0_1_0_0, 'cb_in'))
        self.msg_connect((self.harmonia_clockbias_phase_est_0, 'out'), (self.harmonia_compensation_0_2, 'cb_in'))
        self.msg_connect((self.harmonia_clockbias_phase_est_0, 'out'), (self.harmonia_compensation_0_2_0, 'cb_in'))
        self.msg_connect((self.harmonia_clockbias_phase_est_0, 'out'), (self.plasma_pdu_file_sink_0_0_0_0_0_0, 'in'))
        self.msg_connect((self.harmonia_clockbias_phase_est_0_0, 'out'), (self.harmonia_LFM_src_0, 'cp_in'))
        self.msg_connect((self.harmonia_clockbias_phase_est_0_0, 'out'), (self.harmonia_LFM_src_0_0, 'cp_in'))
        self.msg_connect((self.harmonia_clockbias_phase_est_0_0, 'out'), (self.harmonia_LFM_src_0_1, 'cp_in'))
        self.msg_connect((self.harmonia_clockbias_phase_est_0_0, 'out'), (self.harmonia_compensation_0_0_0_0, 'cp_in'))
        self.msg_connect((self.harmonia_clockbias_phase_est_0_0, 'out'), (self.harmonia_compensation_0_1_0_0, 'cp_in'))
        self.msg_connect((self.harmonia_clockbias_phase_est_0_0, 'out'), (self.harmonia_compensation_0_2_0, 'cp_in'))
        self.msg_connect((self.harmonia_compensation_0, 'out'), (self.harmonia_time_pk_est_0, 'rx'))
        self.msg_connect((self.harmonia_compensation_0, 'out'), (self.plasma_pdu_file_sink_0_0_2_0_1, 'in'))
        self.msg_connect((self.harmonia_compensation_0_0, 'out'), (self.harmonia_time_pk_est_0_0, 'rx'))
        self.msg_connect((self.harmonia_compensation_0_0, 'out'), (self.plasma_pdu_file_sink_0_0_2_0_1_0, 'in'))
        self.msg_connect((self.harmonia_compensation_0_0_0, 'out'), (self.harmonia_time_pk_est_0_0_0, 'rx'))
        self.msg_connect((self.harmonia_compensation_0_0_0_0, 'out'), (self.plasma_pdu_file_sink_0_0_3_0_0_0_1_1_0, 'in'))
        self.msg_connect((self.harmonia_compensation_0_1, 'out'), (self.harmonia_time_pk_est_0_1, 'rx'))
        self.msg_connect((self.harmonia_compensation_0_1, 'out'), (self.plasma_pdu_file_sink_0_0_2_0_1_1, 'in'))
        self.msg_connect((self.harmonia_compensation_0_1_0, 'out'), (self.harmonia_time_pk_est_0_1_0, 'rx'))
        self.msg_connect((self.harmonia_compensation_0_1_0_0, 'out'), (self.plasma_pdu_file_sink_0_0_3_0_0_0_1_1_1, 'in'))
        self.msg_connect((self.harmonia_compensation_0_2, 'out'), (self.harmonia_time_pk_est_0_2, 'rx'))
        self.msg_connect((self.harmonia_compensation_0_2_0, 'out'), (self.plasma_pdu_file_sink_0_0_3_0_0_0_1_1, 'in'))
        self.msg_connect((self.harmonia_frequency_pk_est_0, 'f_out'), (self.harmonia_clock_drift_est_0, 'in'))
        self.msg_connect((self.harmonia_frequency_pk_est_0_0, 'f_out'), (self.harmonia_clock_drift_est_0, 'in3'))
        self.msg_connect((self.harmonia_frequency_pk_est_0_1, 'f_out'), (self.harmonia_clock_drift_est_0, 'in2'))
        self.msg_connect((self.harmonia_single_tone_src_0, 'out'), (self.harmonia_usrp_radar_all_0, 'in3'))
        self.msg_connect((self.harmonia_single_tone_src_0_0, 'out'), (self.harmonia_usrp_radar_all_0, 'in'))
        self.msg_connect((self.harmonia_single_tone_src_0_1, 'out'), (self.harmonia_usrp_radar_all_0, 'in2'))
        self.msg_connect((self.harmonia_time_pk_est_0, 'tp_out'), (self.harmonia_clockbias_phase_est_0, 'in'))
        self.msg_connect((self.harmonia_time_pk_est_0, 'out'), (self.plasma_pdu_file_sink_0_0_3_0_0_0, 'in'))
        self.msg_connect((self.harmonia_time_pk_est_0_0, 'tp_out'), (self.harmonia_clockbias_phase_est_0, 'in2'))
        self.msg_connect((self.harmonia_time_pk_est_0_0, 'out'), (self.plasma_pdu_file_sink_0_0_3_0_0_0_0, 'in'))
        self.msg_connect((self.harmonia_time_pk_est_0_0_0, 'tp_out'), (self.harmonia_clockbias_phase_est_0_0, 'in2'))
        self.msg_connect((self.harmonia_time_pk_est_0_0_0, 'out'), (self.plasma_pdu_file_sink_0_0_3_0_0_0_0_0, 'in'))
        self.msg_connect((self.harmonia_time_pk_est_0_1, 'tp_out'), (self.harmonia_clockbias_phase_est_0, 'in3'))
        self.msg_connect((self.harmonia_time_pk_est_0_1, 'out'), (self.plasma_pdu_file_sink_0_0_3_0_0_0_1, 'in'))
        self.msg_connect((self.harmonia_time_pk_est_0_1, 'out'), (self.qtgui_time_sink_x_0, 'in'))
        self.msg_connect((self.harmonia_time_pk_est_0_1_0, 'tp_out'), (self.harmonia_clockbias_phase_est_0_0, 'in3'))
        self.msg_connect((self.harmonia_time_pk_est_0_1_0, 'out'), (self.plasma_pdu_file_sink_0_0_3_0_0_0_1_0, 'in'))
        self.msg_connect((self.harmonia_time_pk_est_0_2, 'tp_out'), (self.harmonia_clockbias_phase_est_0_0, 'in'))
        self.msg_connect((self.harmonia_time_pk_est_0_2, 'out'), (self.plasma_pdu_file_sink_0_0_3_0_0_0_2, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cd_out'), (self.harmonia_compensation_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cd_out2'), (self.harmonia_compensation_0_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cb_out2'), (self.harmonia_compensation_0_0_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cp_out2'), (self.harmonia_compensation_0_0_0_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cd_out3'), (self.harmonia_compensation_0_1, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cb_out3'), (self.harmonia_compensation_0_1_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cp_out3'), (self.harmonia_compensation_0_1_0_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cb_out'), (self.harmonia_compensation_0_2, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cp_out'), (self.harmonia_compensation_0_2_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'out'), (self.harmonia_frequency_pk_est_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'out3'), (self.harmonia_frequency_pk_est_0_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'out2'), (self.harmonia_frequency_pk_est_0_1, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'out'), (self.plasma_pdu_file_sink_0_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cd_out'), (self.plasma_pdu_file_sink_0_0_1, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cb_out'), (self.plasma_pdu_file_sink_0_0_1_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'out2'), (self.plasma_pdu_file_sink_0_0_2, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cd_out2'), (self.plasma_pdu_file_sink_0_0_2_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cb_out2'), (self.plasma_pdu_file_sink_0_0_2_0_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'out3'), (self.plasma_pdu_file_sink_0_0_3, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cd_out3'), (self.plasma_pdu_file_sink_0_0_3_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'cb_out3'), (self.plasma_pdu_file_sink_0_0_3_0_0, 'in'))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("gnuradio/flowgraphs", "time_peak_estimation_test")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

    def get_tdma_time2(self):
        return self.tdma_time2

    def set_tdma_time2(self, tdma_time2):
        self.tdma_time2 = tdma_time2

    def get_tdma_time(self):
        return self.tdma_time

    def set_tdma_time(self, tdma_time):
        self.tdma_time = tdma_time

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.qtgui_time_sink_x_0.set_samp_rate(self.samp_rate)

    def get_run_id(self):
        return self.run_id

    def set_run_id(self, run_id):
        self.run_id = run_id

    def get_prf(self):
        return self.prf

    def set_prf(self, prf):
        self.prf = prf

    def get_center_freq(self):
        return self.center_freq

    def set_center_freq(self, center_freq):
        self.center_freq = center_freq

    def get_bw(self):
        return self.bw

    def set_bw(self, bw):
        self.bw = bw

    def get_baseband_freq(self):
        return self.baseband_freq

    def set_baseband_freq(self, baseband_freq):
        self.baseband_freq = baseband_freq

    def get_Tw2(self):
        return self.Tw2

    def set_Tw2(self, Tw2):
        self.Tw2 = Tw2

    def get_Tw(self):
        return self.Tw

    def set_Tw(self, Tw):
        self.Tw = Tw

    def get_Tp2(self):
        return self.Tp2

    def set_Tp2(self, Tp2):
        self.Tp2 = Tp2

    def get_Tp(self):
        return self.Tp

    def set_Tp(self, Tp):
        self.Tp = Tp

    def get_Tc2(self):
        return self.Tc2

    def set_Tc2(self, Tc2):
        self.Tc2 = Tc2

    def get_Tc(self):
        return self.Tc

    def set_Tc(self, Tc):
        self.Tc = Tc




def main(top_block_cls=time_peak_estimation_test, options=None):

    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()

    tb.start()
    tb.flowgraph_started.set()

    tb.show()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        Qt.QApplication.quit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    timer = Qt.QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    qapp.exec_()

if __name__ == '__main__':
    main()
