#!/usr/bin/sh
export VOLK_GENERIC=1
export GR_DONT_LOAD_PREFS=1
export srcdir=/home/cody/gr-harmonia/lib
export GR_CONF_CONTROLPORT_ON=False
export PATH="/home/cody/gr-harmonia/build/lib":"$PATH"
export LD_LIBRARY_PATH="":$LD_LIBRARY_PATH
export PYTHONPATH=/home/cody/gr-harmonia/build/test_modules:$PYTHONPATH
harmonia_qa_device.cc 
