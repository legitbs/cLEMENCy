#!/usr/bin/env python
import sys, os
import struct
import sqlite3
from dilas import *

def DisassembleData(FileData):

	return Assembly

def PrintHelp():
	print ""
	print "Lightning ROP Search for cLEMECy"
	print "The output is an assembly dump of the raw binary data provided"
	print "Usage: %s [-b] [-o output] [-l x] [-c x] input" % (sys.argv[0])
	print "\t-f\tDisassemble a raw firmware file instead of ELF"
	print "\t-o\tWrite data to an output file instead of stdout"
	print "\t-l\tStart disassembling at location x, value in hex"
	print "\t-c\tNumber of instructions per line"
	print ""
	return

def main():
	inputfile = ""
	outputfile = "/dev/stdout"
	binformat = 0
	Offset = 0
	EntryCount = 5

	i = 1
	while i < len(sys.argv):
		if sys.argv[i] == "-h":
			PrintHelp()
			return 0
		elif sys.argv[i] == "-f":
			binformat = 1
		elif len(inputfile) == 0:
			inputfile = sys.argv[i]
		else:
			print "Unknown parameter:", sys.argv[i]
			return 1
		i += 1

	#make sure we can locate the file
	if os.path.isfile(inputfile) == False:
		print "Unable to open %s for input" % (inputfile)
		return 1

	if binformat:
		indata = DataHandler_RAW(inputfile)
	else:
		indata = DataHandler_ELF(inputfile)

	indata.seek(Offset, indata.SEEK_SET)
	Output = DisassembleData(indata)

	print "Writing to %s" % (outputfile)
	open(outputfile,"w").write(Output)
	return

main()
