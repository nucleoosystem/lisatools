#!/usr/bin/env python

import os
import sys

def run(command):
    commandline = command % globals()
    print "----> %s" % commandline
    
    try:
        assert(os.system(commandline) == 0)
    except:
        print 'Script %s failed at command "%s".' % (sys.argv[0],commandline)
        sys.exit(1)

# only one argument, and it's the seed...
seed = int(sys.argv[1])

# challenge 2.2 BBH sources...

# 1 - A loud (SNR ~ 2000) signal (with in-spiral "in band", that is from f = 0.1 mHz, lasting about 1 week) and coalescing 3 months +/- 1 month after the beginning of the observations
run('bin/makesource-BBH.py --seed=%s --distance=1e10 --requestSN=2000 --coalescTime=90  --coalescRange=30 --sourceName="SMBH-1" Source/BH-1.xml' % (seed+1))

# 2 - A moderate-to-low SNR (~ 20) signal with coalescence between 25 and 26 months from the beginning of the observations
run('bin/makesource-BBH.py --seed=%s --distance=1e10 --requestSN=20   --coalescTime=765 --coalescRange=15 --sourceName="SMBH-2" Source/BH-2.xml' % (seed+2))

# the following BBHs may or may not be included in the challenge, depending on fate...

import random

random.seed(seed)
chosen = random.sample([3,4,5,6],random.randint(2,4))

# 3 - A loud (SNR ~ 1000) signal coalescing between 6 and 24 months from beginning of observations
if 3 in chosen:
    run('bin/makesource-BBH.py --seed=%s --distance=1e10 --requestSN=1000 --coalescTime=450 --coalescRange=270 --sourceName="SMBH-3" Source/BH-3.xml' % (seed+3))

# 4 - A loud (SNR ~ 200) signal coalescing between 6 and 24 months from beginning of observations
if 4 in chosen:
    run('bin/makesource-BBH.py --seed=%s --distance=1e10 --requestSN=200  --coalescTime=450 --coalescRange=270 --sourceName="SMBH-4" Source/BH-4.xml' % (seed+4))

# 5 - A loud (SNR ~ 100) signal with coalescence taking place between 18 and 21 months from the start of the observations
if 5 in chosen:
    run('bin/makesource-BBH.py --seed=%s --distance=1e10 --requestSN=100  --coalescTime=540 --coalescRange=45  --sourceName="SMBH-5" Source/BH-5.xml' % (seed+5))

# 6 - A low SNR (~ 10) signal with coalescence taking place between 27 and 28 months from the beginning of the observations
if 6 in chosen:
    run('bin/makesource-BBH.py --seed=%s --distance=1e10 --requestSN=10   --coalescTime=825 --coalescRange=15  --sourceName="SMBH-6" Source/BH-6.xml' % (seed+6))

# challenge 2.2 EMRI sources...
# 2 with SMBH around 1e6, 2 with SMBH around 5e6, 1 with SMBH around 1e7

requestsn = random.randint(30,100)
run('bin/makesource-EMRI.py --seed=%s --distance=1e6 --requestSN=%s --massSMBH=1e6 --sourceName="EMRI-1" Source/EMRI-1.xml' % (seed+7,requestsn))

requestsn = random.randint(30,100)
run('bin/makesource-EMRI.py --seed=%s --distance=1e6 --requestSN=%s --massSMBH=1e6 --sourceName="EMRI-2" Source/EMRI-2.xml' % (seed+8,requestsn))

requestsn = random.randint(30,100)
run('bin/makesource-EMRI.py --seed=%s --distance=1e6 --requestSN=%s --massSMBH=5e6 --sourceName="EMRI-3" Source/EMRI-3.xml' % (seed+9,requestsn))

requestsn = random.randint(30,100)
run('bin/makesource-EMRI.py --seed=%s --distance=1e6 --requestSN=%s --massSMBH=5e6 --sourceName="EMRI-4" Source/EMRI-4.xml' % (seed+10,requestsn))

requestsn = random.randint(30,100)
run('bin/makesource-EMRI.py --seed=%s --distance=1e6 --requestSN=%s --massSMBH=1e7 --sourceName="EMRI-5" Source/EMRI-5.xml' % (seed+11,requestsn))

# now the Galaxy! (for testing, seed must be 1)
# run('bin/makesource-Galaxy.py -t %s' % seed)

run('bin/makesource-Galaxy.py %s' % seed)
