#!/usr/bin/python3

import subprocess

rpms = subprocess.check_output(["rpm", "-qa"])
rpms = rpms.decode("utf-8").split("\n")
rpms.sort()

for rpm in rpms:
    if rpm == "":
        continue
    print(rpm, end="\r\n")
