#!/usr/bin/sh
export VOLK_GENERIC=1
export GR_DONT_LOAD_PREFS=1
export srcdir=/home/cody/gr-harmonia/python/harmonia
export GR_CONF_CONTROLPORT_ON=False
export PATH="/home/cody/gr-harmonia/build/python/harmonia":"$PATH"
export LD_LIBRARY_PATH="":$LD_LIBRARY_PATH
export PYTHONPATH=/home/cody/gr-harmonia/build/test_modules:$PYTHONPATH
/usr/bin/python3 /home/cody/gr-harmonia/python/harmonia/qa_clockbias_phase_est.py 
