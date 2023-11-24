#!/usr/bin/python3

import sys
import subprocess

windres = sys.argv[1]
icondir = sys.argv[2]
manifestdir = sys.argv[3]
rcfile = sys.argv[4]
objfile = sys.argv[5]

subprocess.check_call([
    windres,
    "-DICONDIR=\\\"" + icondir + "\\\"",
    "-DMANIFESTDIR=\\\"" + manifestdir + "\\\"",
    "-i", rcfile,
    "-o", objfile,
    ])
