#!/usr/bin/env python3
import os
import sys
import time
import argparse
import subprocess


# Only import PyQt/GNU Radio in the CHILD process to avoid leaking state in the parent.
def run_once(run_id: int, duration: float):
    from PyQt5.QtWidgets import QApplication
    from time_peak_estimation_test import time_peak_estimation_test


    app = QApplication(sys.argv)
    print(f"[run {run_id}] starting...")
    tb = time_peak_estimation_test()
    tb.start()
    try:
        time.sleep(duration)
    finally:
        tb.stop()
        tb.wait()
        print(f"[run {run_id}] finished cleanly.")
    # If your flowgraph uses Qt widgets, you can optionally quit the app:
    # app.quit()


def parent_driver(num_runs: int, duration: float):
    os.makedirs("outputs_internal", exist_ok=True)


    for run_id in range(1, num_runs + 1):
        log_path = os.path.join("outputs_internal", f"run_{run_id}.log")
        with open(log_path, "wb") as logf:
            # Spawn this same script as a CHILD to do one run, unbuffered (-u)
            proc = subprocess.Popen(
                [sys.executable, "-u", __file__, "--child", str(run_id), "--duration", str(duration)],
                stdout=logf,
                stderr=subprocess.STDOUT,
                close_fds=True,
            )
            code = proc.wait()
        print(f"Finished run {run_id} (exit {code}) -> {log_path}")
        if code != 0:
            print(f"Run {run_id} exited non-zero; stopping further runs.")
            break


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--child", type=int, help="Internal: run a single child job with this run_id")
    p.add_argument("--duration", type=float, default=35.0, help="Seconds to run each flowgraph")
    p.add_argument("--runs", type=int, default=100, help="Number of runs (parent mode)")
    args = p.parse_args()


    if args.child is not None:
        # CHILD mode: do one run and emit all output to stdout/stderr
        run_once(run_id=args.child, duration=args.duration)
    else:
        # PARENT mode: spawn children; each child's output goes to its own file
        parent_driver(num_runs=args.runs, duration=args.duration)


if __name__ == "__main__":
    main()