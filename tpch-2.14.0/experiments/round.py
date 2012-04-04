#!/usr/bin/env python
import sys
x = int(sys.argv[1])
n = int(sys.argv[2])
r = x % n
if not r: print x
else: print (x + n - r)
