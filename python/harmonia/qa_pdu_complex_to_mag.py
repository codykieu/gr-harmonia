#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2025 Cody Kieu.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from gnuradio import gr, gr_unittest
# from gnuradio import blocks
try:
    from gnuradio.harmonia import pdu_complex_to_mag
except ImportError:
    import os
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from gnuradio.harmonia import pdu_complex_to_mag

class qa_pdu_complex_to_mag(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def test_simple (self):
        self.emitter = harmonia.message_emitter()
        self.ctm2 = harmonia.pdu_complex_to_mag()
        self.debug = blocks.message_debug()
        self.tb.msg_connect((self.emitter, 'msg'), (self.ctm2, 'cpdus'))
        self.tb.msg_connect((self.ctm2, 'fpdus'), (self.debug, 'store'))

        # gnuradio uses single-precision floats by default
        i_vec = pmt.init_c32vector(3, [3+4j, 1+0j, 0+1j])
        e_vec = pmt.init_f32vector(3, [25, 1, 1])

        in_pdu = pmt.cons(pmt.make_dict(), i_vec)
        e_pdu = pmt.cons(pmt.make_dict(), e_vec)

        self.tb.start()
        time.sleep(.001)
        self.emitter.emit(pmt.intern("MALFORMED PDU"))
        time.sleep(.001)
        self.emitter.emit(e_pdu)
        time.sleep(.001)
        self.emitter.emit(in_pdu)
        time.sleep(.01)
        self.tb.stop()
        self.tb.wait()

        print("test ctm2:")
        print("pdu expected: " + repr(pmt.car(e_pdu)))
        print("pdu got:      " + repr(pmt.car(self.debug.get_message(0))))
        print("data expected: " + repr(pmt.to_python(pmt.cdr(e_pdu))))
        print("data got:      " + repr(pmt.to_python(pmt.cdr(self.debug.get_message(0)))))
        print

        self.assertTrue(pmt.equal(self.debug.get_message(0), e_pdu))


if __name__ == '__main__':
    gr_unittest.run(qa_pdu_complex_to_mag)
