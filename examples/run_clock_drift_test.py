#!/usr/bin/env python3
import os
import sys
import time
from PyQt5.QtWidgets import QApplication
app = QApplication(sys.argv)

from clock_drift_test import clock_drift_test

def main():
    os.makedirs("outputs", exist_ok=True)
    for run_id in range(1, 101):
        tb = clock_drift_test(run_id=run_id)
        tb.start()
        # run for N seconds (tweak to match how long one “run” should be)
        time.sleep(2.0)
        tb.stop()     # tell it to finish
        tb.wait()     # now wait() will return
        print(f"Finished run {run_id}")

if __name__ == "__main__":
    main()
