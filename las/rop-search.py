#!/usr/bin/env python
import sys, os
import struct
import sqlite3
from dilas import *

def DisassembleData(FileData, EntryCount, outputfile):
	of = open(outputfile,"w")

	while(FileData.left() >= 2):
		CurPos = FileData.tell()
		Assembly = "%07x: " % (CurPos)

		#get the instruction info
		for i in xrange(0, EntryCount):
			(CurAssembly, CurInst, CurInstStr, ErrorMsg) = DisassembleInstruction(FileData)
			if(len(ErrorMsg)):
				break
			Assembly += CurAssembly.replace("\t", " ") + " ; "

		#move a byte over
		FileData.seek(CurPos + 1, FileData.SEEK_SET)
		of.write(Assembly + "\n")
	of.close()

def PrintHelp():
	print ""
	print "Lightning ROP Search for cLEMECy"
	print "The output is an assembly dump of the raw binary data provided"
	print "Usage: %s input [-b] [-o output] [-l x]" % (sys.argv[0])
	print "\t-f\tDisassemble a raw firmware file instead of ELF"
	print "\t-o\tWrite data to an output file instead of stdout"
	print "\t-l\tStart disassembling at location x, value in hex"
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
		if sys.argv[i] == "-o":
			if len(sys.argv) < i+1:
				print "Missing parameter to -o"
				return 1
			outputfile = sys.argv[i+1]
			i += 1
		elif sys.argv[i] in ["-h", "--help"]:
			PrintHelp()
			return 0
		elif sys.argv[i] == "-f":
			binformat = 1
		elif sys.argv[i] == "-l":
			if len(sys.argv) < i+1:
				print "Missing parameter to -l"
				return 1
			Offset = int(sys.argv[i+1], 16)
			i += 1
		elif sys.argv[i] == "-c":
			if len(sys.argv) < i+1:
				print "Missing parameter to -c"
				return 1
			EntryCount = int(sys.argv[i+1], 16)
			i += 1
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
	print "Writing to %s" % (outputfile)
	DisassembleData(indata, EntryCount, outputfile)

	return

main()
