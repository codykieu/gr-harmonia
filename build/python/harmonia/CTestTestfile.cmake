# CMake generated Testfile for 
# Source directory: /home/cody/gr-harmonia/python/harmonia
# Build directory: /home/cody/gr-harmonia/build/python/harmonia
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(qa_pdu_complex_to_mag "/usr/bin/sh" "qa_pdu_complex_to_mag_test.sh")
set_tests_properties(qa_pdu_complex_to_mag PROPERTIES  _BACKTRACE_TRIPLES "/usr/local/lib/cmake/gnuradio/GrTest.cmake;119;add_test;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;37;GR_ADD_TEST;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;0;")
add_test(qa_pdu_fft "/usr/bin/sh" "qa_pdu_fft_test.sh")
set_tests_properties(qa_pdu_fft PROPERTIES  _BACKTRACE_TRIPLES "/usr/local/lib/cmake/gnuradio/GrTest.cmake;119;add_test;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;38;GR_ADD_TEST;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;0;")
add_test(qa_frequency_pk_est "/usr/bin/sh" "qa_frequency_pk_est_test.sh")
set_tests_properties(qa_frequency_pk_est PROPERTIES  _BACKTRACE_TRIPLES "/usr/local/lib/cmake/gnuradio/GrTest.cmake;119;add_test;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;39;GR_ADD_TEST;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;0;")
add_test(qa_single_tone_src "/usr/bin/sh" "qa_single_tone_src_test.sh")
set_tests_properties(qa_single_tone_src PROPERTIES  _BACKTRACE_TRIPLES "/usr/local/lib/cmake/gnuradio/GrTest.cmake;119;add_test;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;40;GR_ADD_TEST;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;0;")
add_test(qa_usrp_radar_2 "/usr/bin/sh" "qa_usrp_radar_2_test.sh")
set_tests_properties(qa_usrp_radar_2 PROPERTIES  _BACKTRACE_TRIPLES "/usr/local/lib/cmake/gnuradio/GrTest.cmake;119;add_test;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;41;GR_ADD_TEST;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;0;")
add_test(qa_SDR_tagger "/usr/bin/sh" "qa_SDR_tagger_test.sh")
set_tests_properties(qa_SDR_tagger PROPERTIES  _BACKTRACE_TRIPLES "/usr/local/lib/cmake/gnuradio/GrTest.cmake;119;add_test;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;42;GR_ADD_TEST;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;0;")
add_test(qa_usrp_radar_all "/usr/bin/sh" "qa_usrp_radar_all_test.sh")
set_tests_properties(qa_usrp_radar_all PROPERTIES  _BACKTRACE_TRIPLES "/usr/local/lib/cmake/gnuradio/GrTest.cmake;119;add_test;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;43;GR_ADD_TEST;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;0;")
add_test(qa_usrp_radar_tx "/usr/bin/sh" "qa_usrp_radar_tx_test.sh")
set_tests_properties(qa_usrp_radar_tx PROPERTIES  _BACKTRACE_TRIPLES "/usr/local/lib/cmake/gnuradio/GrTest.cmake;119;add_test;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;44;GR_ADD_TEST;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;0;")
add_test(qa_usrp_radar_rx "/usr/bin/sh" "qa_usrp_radar_rx_test.sh")
set_tests_properties(qa_usrp_radar_rx PROPERTIES  _BACKTRACE_TRIPLES "/usr/local/lib/cmake/gnuradio/GrTest.cmake;119;add_test;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;45;GR_ADD_TEST;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;0;")
add_test(qa_clock_drift_est "/usr/bin/sh" "qa_clock_drift_est_test.sh")
set_tests_properties(qa_clock_drift_est PROPERTIES  _BACKTRACE_TRIPLES "/usr/local/lib/cmake/gnuradio/GrTest.cmake;119;add_test;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;46;GR_ADD_TEST;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;0;")
add_test(qa_time_pk_est "/usr/bin/sh" "qa_time_pk_est_test.sh")
set_tests_properties(qa_time_pk_est PROPERTIES  _BACKTRACE_TRIPLES "/usr/local/lib/cmake/gnuradio/GrTest.cmake;119;add_test;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;47;GR_ADD_TEST;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;0;")
add_test(qa_buffer_corrector "/usr/bin/sh" "qa_buffer_corrector_test.sh")
set_tests_properties(qa_buffer_corrector PROPERTIES  _BACKTRACE_TRIPLES "/usr/local/lib/cmake/gnuradio/GrTest.cmake;119;add_test;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;48;GR_ADD_TEST;/home/cody/gr-harmonia/python/harmonia/CMakeLists.txt;0;")
subdirs("bindings")
