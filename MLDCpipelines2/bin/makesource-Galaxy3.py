#!/usr/bin/env python

__version__='$Id: $'

import sys
import os
import tempfile

def run(command,quiet = False):
    commandline = command % globals()
    
    try:
        if not quiet:
            print "----> %s" % commandline
            assert(os.system(commandline) == 0)
        else:
            assert(os.system(commandline + ' > /dev/null') == 0)
    except:
        print 'Script %s failed at command "%s".' % (sys.argv[0],commandline)
        sys.exit(1)

# set ourselves up to parse command-line options

from optparse import OptionParser

# note that correct management of the Id string requires issuing the command
# svn propset svn:keywords Id FILENAME

parser = OptionParser(usage="usage: %prog --seed=SEED OUTPUT.xml",
                      version="$Id: $")

parser.add_option("-s", "--seed",
                  type="int", dest="seed", default=None,
                  help="Galaxy seed (int) [required]")

parser.add_option("-v", "--verification",
                  action="store_true", dest="verification", default=False,
                  help="include verification binaries [defaults to False]")

parser.add_option("-c", "--confusion",
                  action="store_true", dest="confusion", default=False,
                  help="run Galaxy3 code to produce partially regressed confusion foreground [defaults to False]")

parser.add_option("-f", "--bulgefix",
                  action="store_true", dest="bulgefix", default=False,
                  help="use the fixed Challenge-4 catalogs with the correct cusp distribution [defaults to False]")

parser.add_option("-g", "--general",
                  action="store_true", dest="general", default=False,
                  help="use Galaxy_General instead of Galaxy3 [off by default]")

(options, args) = parser.parse_args()

if options.seed == None:
    parser.error("You must give me a seed!")

seed         = options.seed
verification = options.verification
confusion    = options.confusion
bulgefix     = options.bulgefix

if verification and confusion:
    parser.error("The --verification and --confusion options are incompatible.")

if bulgefix and confusion:
    parser.error("The --bulgefix and --confusion options are incompatible.")
    
if len(args) != 1:
    parser.error("I need a location for the output Galaxy file!")

outputfile = args[0]

# try to find the Galaxy code

if not options.general:
    galaxydir = os.path.abspath(os.path.dirname(os.path.abspath(sys.argv[0])) + '/../../MLDCwaveforms/Galaxy3')
else:
    galaxydir = os.path.abspath(os.path.dirname(os.path.abspath(sys.argv[0])) + '/../../MLDCwaveforms/Galaxy_General')

# see if the fast galaxy code is there

if not os.path.isfile(galaxydir + '/Fast_Response') and not os.path.isfile(galaxydir + '/Fast_Response3'):
    print "Cannot find the fast Galaxy code! Try re-running master-install.py."
    sys.exit(1)

# --- make a virtual Galaxy environment

here = os.getcwd()

workdir = os.path.abspath(tempfile.mkdtemp(dir='.'))
os.chdir(workdir)

if not options.general:
    execfiles = ['Fast_Response3','Fast_XML_LS3','Fast_XML_SL3','Galaxy_key3','Galaxy_Maker3','Galaxy_Maker4','Confusion_Maker3']

    for f in execfiles:
        run('ln -s %s/%s ./%s' % (galaxydir,f,f),quiet=True)
else:
    execfiles = ['Fast_Response','Fast_XML_LS','Fast_XML_SL','Galaxy_key','Galaxy_Maker','Confusion_Maker']
    
    for f in execfiles:
        run('ln -s %s/%s ./%s3' % (galaxydir,f,f),quiet=True)

run('mkdir Binary',quiet=True)
run('mkdir Data',quiet=True)
run('mkdir XML',quiet=True)

run('ln -s %s/Data/AMCVn_GWR_MLDC.dat ./Data/AMCVn_GWR_MLDC.dat' % galaxydir,quiet=True)
run('ln -s %s/Data/dwd_GWR_MLDC.dat ./Data/dwd_GWR_MLDC.dat' % galaxydir,quiet=True)
run('ln -s %s/Data/AMCVn_GWR_MLDC_bulgefix_opt.dat ./Data/AMCVn_GWR_MLDC_bulgefix_opt.dat' % galaxydir,quiet=True)
run('ln -s %s/Data/dwd_GWR_MLDC_bulgefix.dat ./Data/dwd_GWR_MLDC_bulgefix.dat' % galaxydir,quiet=True)
run('ln -s %s/Data/Verification.dat ./Data/Verification.dat' % galaxydir,quiet=True)

# make Galaxy!

os.chdir(here)

galaxyfile = os.path.abspath(outputfile)

os.chdir(workdir)

if bulgefix:
    if verification:
        run('./Galaxy_Maker4 %s 1' % seed)
    else:
        run('./Galaxy_Maker4 %s 0' % seed)
else:
    if confusion:
        run('./Confusion_Maker3 %s' % seed)
    else:
        if verification:
            run('./Galaxy_Maker3 %s 1' % seed)
        else:
            run('./Galaxy_Maker3 %s 0' % seed)

run('./Galaxy_key3 TheGalaxy %s' % seed)

# move and rename the Galaxy XML file

run("sed 's/Data\///g' < XML/TheGalaxy_key.xml > %s" % galaxyfile)

destdir = os.path.dirname(galaxyfile) + '/.'
run("mv Data/Galaxy_%s.dat %s" % (seed,destdir))
# if we do confusion, this file may not be here
if os.path.isfile('Data/Galaxy_Bright_%s.dat' % seed):
    run("mv Data/Galaxy_Bright_%s.dat %s" % (seed,destdir))

run('rm -f XML/noise-only.xml',quiet=True)
run('rm -f XML/noise-only.bin',quiet=True)
run('rm -f Data/*.txt',quiet=True)

os.chdir(here)

# ...remove the tmp dir

run('rm -rf %s' % workdir,quiet=True)

sys.exit(0)
