#!/usr/bin/env python

import subprocess
import sys

exe = sys.argv[1]

P = subprocess.run(["avr-size",exe],capture_output=True)
if P.returncode:
    print(P.stdout)
    print(P.stderr)
    sys.exit(P.returncode)

tmp = P.stdout.decode().strip().split("\n")[-1].strip()
tsize, dsize, bsize = [int(q) for q in tmp.split()[:3]]
print("Code (flash):",tsize,"bytes")
print("Data+ROdata+bss:",dsize+bsize,"bytes")
