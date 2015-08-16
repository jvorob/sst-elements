#!/usr/bin/env python
'''
Date        : 08/04/2015  
Created by  : Fulya Kaplan
Description : This global script runs scheduler and ember simulations successively until completion.
'''

import os, sys
from xml.dom.minidom import parse
import xml.dom.minidom

from optparse import OptionParser
import math
import numpy as np

# Function to run linux commands
def run(cmd):
    #print(cmd)
    os.system(cmd)

def clear_files():

    erFile = open('emberRunning.txt', 'w')
    ecFile = open('emberCompleted.txt', 'w')

    erFile.close()
    ecFile.close()

def run_sim (options):

    # Run scheduler for the first time and create the first snapshot
    init_cmd  = "sst ./%s" %(options.schedPythonFile)
    run(init_cmd)

    #ember_cmd = "./%s --xml %s > %s" %(options.sched_parser, options.xmlFile, options.emberOutFile)
    #ember_cmd = "./%s --xml %s " %(options.sched_parser, options.xmlFile )
    #run(ember_cmd)

    # Do the following in a loop until the simulation is completed
    # Parse scheduler snapshot->run ember->Parse ember output->run scheduler->...
    while( is_not_empty(options.xmlFile) ):
        ember_cmd = "./%s --xml %s > %s" %(options.sched_parser, options.xmlFile, options.emberOutFile)
        run(ember_cmd)

        sched_cmd = "./%s --xml %s --emberOut %s --schedPy %s" %(options.ember_parser, options.xmlFile, options.emberOutFile, options.schedPythonFile)
        run(sched_cmd)

def is_not_empty(fileName):

    try:
        if os.stat(fileName).st_size > 0:
            return True
        else:
            return False 
    except OSError:
        print "No file"

def main():

    parser = OptionParser(usage="usage: %prog [options]")
    parser.add_option("--xml",  action='store', dest="xmlFile", help="Name of the xml file that holds the current scheduler snapshot.") 
    parser.add_option("--emberOut",  action='store', dest="emberOutFile", help="Name of the ember output file.")
    parser.add_option("--schedPy",  action='store', dest="schedPythonFile", help="Name of the python file that holds the scheduler parameters.")
    parser.add_option("--sched_parser",  action='store', dest="sched_parser", help="Name of the file that parses the current scheduler snapshot and runs ember.") 
    parser.add_option("--ember_parser",  action='store', dest="ember_parser", help="Name of the file that parses the ember output file and runs scheculer.") 
    (options, args) = parser.parse_args()

    #print is_not_empty(options.xmlFile)
    clear_files()
    run_sim(options)
    

if __name__ == '__main__':
    main()