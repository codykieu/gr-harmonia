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
import threading



class delay_test(gr.top_block, Qt.QWidget):

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

        self.settings = Qt.QSettings("gnuradio/flowgraphs", "delay_test")

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
        self.samp_rate = samp_rate = 100e6
        self.center_freq = center_freq = 1e9
        self.bw = bw = 50e6
        self.Tp = Tp = 500e-6

        ##################################################
        # Blocks
        ##################################################

        self.plasma_pdu_file_sink_0 = plasma.pdu_file_sink(gr.sizeof_gr_complex,'delay_test', '')
        self.harmonia_usrp_radar_all_0 = harmonia.usrp_radar_all("addr=192.168.80.2, use_dpkg=1", "addr=192.168.60.2, use_dpkg=1", "addr=192.168.40.2, use_dpkg=1", samp_rate, samp_rate, samp_rate, center_freq, center_freq, center_freq, 0,
         0, 0, 1.5, 0.005, -0.001, 0, False)
        self.harmonia_usrp_radar_all_0.set_metadata_keys('core:tx_freq', 'core:rx_freq', 'core:sample_start', 'radar:prf')
        self.harmonia_single_tone_src_0_0 = harmonia.single_tone_src(1e6, center_freq, 0, Tp, samp_rate, 0, 1996, 1)
        self.harmonia_single_tone_src_0_0.init_meta_dict('radar:frequency', 'radar:phase', 'radar:duration', 'core:sample_rate', 'core:label', 'radar:prf')


        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.harmonia_single_tone_src_0_0, 'out'), (self.harmonia_usrp_radar_all_0, 'in'))
        self.msg_connect((self.harmonia_usrp_radar_all_0, 'out'), (self.plasma_pdu_file_sink_0, 'in'))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("gnuradio/flowgraphs", "delay_test")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate

    def get_center_freq(self):
        return self.center_freq

    def set_center_freq(self, center_freq):
        self.center_freq = center_freq

    def get_bw(self):
        return self.bw

    def set_bw(self, bw):
        self.bw = bw

    def get_Tp(self):
        return self.Tp

    def set_Tp(self, Tp):
        self.Tp = Tp




def main(top_block_cls=delay_test, options=None):

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
