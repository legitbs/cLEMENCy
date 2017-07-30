#!/usr/bin/env python
import sys, os
import sqlite3
import struct
import binascii

Sections = dict()
SectionArray = []
CurSection = ""
RelocList = []
UnknownLabelLocs = dict()	#all unknown labels we encountered and where we need to go fix up for them
LabelLocs = dict()		#all known labels and their location
Externs = []			#anything that needs to be put into the extern list so that the elf linker will fix them up
Alignment = 16
Defines = dict()
MacroSkipBlock = False

Registers = []
Instructions = dict()
SymbolsLocal = []
SymbolsGlobal = []
SymbolTextsLocal = []
SymbolsAdded = dict()

SHN_ABS = 0xfff1
SHN_COMMON = 0xfff2
SHN_UNDEF = 0
STB_GLOBAL = 1
STT_OBJECT = 1
STT_FUNC = 2

class DataHandler:
	SEEK_SET = 0
	SEEK_CUR = 1
	SEEK_END = 2

	def __init__(self, SectionName, UseWords = 1):
		self._data = ""
		self._curoffset = 0
		self._datalen = 0
		self.Name = SectionName

		if UseWords:
			self._datasize = 2
		else:
			self._datasize = 1

	def read(self, bytecount):
		if bytecount < 0:
			return ""

		outdata = self._data[self._curoffset:self._curoffset + (bytecount * self._datasize)]
		self._curoffset += bytecount * self._datasize
		return outdata

	def write(self, data):
		if len(data) == 0:
			return

		if len(data) % self._datasize:
			raise Exception("Invalid write length %d" % (len(data)))

		#if pointer is past the end then null pad to the location
		if self._curoffset > self._datalen:
			self._data += "\x00" * (self._curoffset - self._datalen)
			self._datalen = self._curoffset

		#if the offset and end match then add the current data to the end
		if self._curoffset == self._datalen:
			self._data += data
			self._datalen += len(data)
			self._curoffset = self._datalen
			return

		#we must be in the middle of data somewhere
		self._data = self._data[0:self._curoffset] + data + self._data[self._curoffset + len(data):]
		self._curoffset += len(data)
		self._datalen = len(self._data)
		return

	def seek(self, offset, whence):
		newoffset = -1
		if whence == self.SEEK_SET:
			newoffset = offset * self._datasize
		elif whence == self.SEEK_CUR:
			newoffset = self._curoffset + (offset * self._datasize)
		elif whence == self.SEEK_END:
			newoffset = self._datalen - (offset *self._datasize)

		#make sure we don't go negative
		if newoffset >= 0:
			self._curoffset = newoffset

		#return where we landed
		return self._curoffset / self._datasize

	def tell(self):
		return self._curoffset / self._datasize

def PrintBytes(Count, data):
	OutData = "%d bytes: %x" % (Count, data)
	for i in xrange(Count -1, -1, -1):
		CurVal = data >> (i*9)
		CurVal = "0"*9 + bin(CurVal)[2:]
		CurVal = CurVal[-9:]
		OutData += " - " + CurVal

	#print OutData

def Write1Byte(f, data):
	PrintBytes(1, data)

	#output 1 byte
	if data & ~0xff:
		print "Value 0x%x too large for a single byte" % (data)
		return 1

	f.write(chr(data & 0xff) + chr(data >> 8))
	return 0

def Write2Bytes(f, data):
	PrintBytes(2, data)

	if data & ~0x3ffff:
		print "Value 0x%x too large for two bytes" % (data)
		return 1

	#bytedata in little endian format, convert to middle endian as we write it out
	#input: A B
	#output: B A
	Byte1 = ((data >> 9) & 0x1ff)
	Byte2 = (data & 0x1ff)
	f.write(chr(Byte2 & 0xff) + chr(Byte2 >> 8))
	f.write(chr(Byte1 & 0xff) + chr(Byte1 >> 8))
	return 0

def Write3Bytes(f, data):
	PrintBytes(3, data)

	if data & ~0x7ffffff:
		print "Value 0x%x too large for three bytes" % (data)
		return 1

	#bytedata in little endian format, convert to middle endian as we write it out
	#input: A B C
	#output: B A C
	Byte1 = ((data >> 18) & 0x1ff)
	Byte2 = ((data >> 9) & 0x1ff)
	Byte3 = (data & 0x1ff)

	f.write(chr(Byte2 & 0xff) + chr(Byte2 >> 8))
	f.write(chr(Byte1 & 0xff) + chr(Byte1 >> 8))
	f.write(chr(Byte3 & 0xff) + chr(Byte3 >> 8))
	return 0

def Write4Bytes(f, data):
	PrintBytes(4, data)

	if data & ~0xfffffffff:
		print "Value 0x%x too large for four bytes" % (data)
		return 1

	#bytedata in little endian format, convert to middle endian as we write it out
	#input: A B C D
	#output: B A C D
	Byte1 = ((data >> 27) & 0x1ff)
	Byte2 = ((data >> 18) & 0x1ff)
	Byte3 = ((data >> 9) & 0x1ff)
	Byte4 = (data & 0x1ff)
	f.write(chr(Byte2 & 0xff) + chr(Byte2 >> 8))
	f.write(chr(Byte1 & 0xff) + chr(Byte1 >> 8))
	f.write(chr(Byte3 & 0xff) + chr(Byte3 >> 8))
	f.write(chr(Byte4 & 0xff) + chr(Byte4 >> 8))
	return 0

def Read1Byte(f):
	Byte1 = f.read(1)
	return ord(Byte1[0]) | (ord(Byte1[1]) << 8)

def Read2Bytes(f):
	Byte1 = f.read(1)
	Byte2 = f.read(1)

	Byte1 = ord(Byte1[0]) | (ord(Byte1[1]) << 8)
	Byte2 = ord(Byte2[0]) | (ord(Byte2[1]) << 8)

	#input: B A
	#output: A B
	bytedata = (Byte2 << 9) | Byte1

	return bytedata

def Read3Bytes(f):
	Byte1 = f.read(1)
	Byte2 = f.read(1)
	Byte3 = f.read(1)

	Byte1 = ord(Byte1[0]) | (ord(Byte1[1]) << 8)
	Byte2 = ord(Byte2[0]) | (ord(Byte2[1]) << 8)
	Byte3 = ord(Byte3[0]) | (ord(Byte3[1]) << 8)

	#input: B A C
	#output: A B C
	bytedata = (Byte2 << 18) | (Byte1 << 9) | Byte3

	return bytedata


def Read4Bytes(f):
	Byte1 = f.read(1)
	Byte2 = f.read(1)
	Byte3 = f.read(1)
	Byte4 = f.read(1)

	Byte1 = ord(Byte1[0]) | (ord(Byte1[1]) << 8)
	Byte2 = ord(Byte2[0]) | (ord(Byte2[1]) << 8)
	Byte3 = ord(Byte3[0]) | (ord(Byte3[1]) << 8)
	Byte4 = ord(Byte4[0]) | (ord(Byte4[1]) << 8)

	#input: B A C
	#output: A B C
	bytedata = (Byte2 << 27) | (Byte1 << 18) | (Byte3 << 9) | Byte4

	return bytedata

def ProcessAssembly(asmdata):
	global CurSection, MacroSkipBlock
	SetupInstructions()

	Sections[".symtab"] = dict()
	Sections[".symtab"]["Name"] = ".symtab"
	Sections[".symtab"]["Data"] = DataHandler(".symtab", 0)
	Sections[".symtab"]["Flags"] = 0
	Sections[".symtab"]["Type"] = 2
	Sections[".symtab"]["Link"] = 0
	Sections[".symtab"]["Info"] = 0
	Sections[".symtab"]["SizePerEntry"] = 16
	Sections[".symtab"]["Offset"] = 0
	SectionArray.append(".symtab")

	Sections[".strtab"] = dict()
	Sections[".strtab"]["Name"] = ".strtab"
	Sections[".strtab"]["Data"] = DataHandler(".strtab", 0)
	Sections[".strtab"]["Flags"] = 0
	Sections[".strtab"]["Type"] = 3
	Sections[".strtab"]["Link"] = 0
	Sections[".strtab"]["Info"] = 0
	Sections[".strtab"]["SizePerEntry"] = 1
	Sections[".strtab"]["Offset"] = 0
	Sections[".strtab"]["Strings"] = dict()

	AddSymbolEntry("", 0, 0, 0, 0, 0)

	#setup our default section
	Sections[".cs"] = dict()
	Sections[".cs"]["Name"] = ".cs"
	Sections[".cs"]["Data"] = DataHandler(".cs")
	Sections[".cs"]["Flags"] = 6	#AX
	Sections[".cs"]["Type"] = 1
	Sections[".cs"]["Link"] = 0
	Sections[".cs"]["Info"] = 0
	Sections[".cs"]["SizePerEntry"] = 1
	Sections[".cs"]["Offset"] = 0
	SectionArray.append(".cs")

	CurSection = ".cs"

	#go line by line and setup the assembly
	for curlinenum in xrange(0, len(asmdata)):
		curline = asmdata[curlinenum]
		if curline.find(";") != -1:
			curline = curline[:curline.find(";")]
		curline = curline.strip()

		#skip blank lines
		if len(curline) == 0:
			continue

		#get the first entry to determine if it is a command
		Chunk = curline.split()[0]
		Chunk = Chunk.lower()

		if curline[0] == "%":
			ret = HandleMacro(Sections[CurSection]["Data"], Chunk, curline, curlinenum+1)
		elif MacroSkipBlock:
			continue
		elif Chunk in ["section", "public", "extrn", "db", "dw", "dt"]:
			ret = HandleSpecialCommand(Sections[CurSection]["Data"], Chunk, curline, curlinenum+1)
		elif curline[-1] == ":":
			ret = HandleNewLabel(Sections[CurSection]["Data"], curline, curlinenum+1)
		else:
			ret = HandleInstruction(Sections[CurSection]["Data"], curline, curlinenum+1)

		#if the function failed then exit
		if ret:
			sys.exit(0)

	#fix up all unknown symbols that we can reference
	ret = FixSymbols()
	if ret:
		sys.exit(0)

	return 0

def HandleMacro(f, command, linedata, linenumber):
	global CurSection, MacroSkipBlock
	CurLine = linedata.split()

	if (command == "%define") and (len(CurLine) >= 2):
		if CurLine[1].lower() not in Defines:
			Defines[CurLine[1]] = 0
		if len(CurLine) >= 3:
			Defines[CurLine[1]] = " ".join(CurLine[2:])
	elif (command == "%ifdef") and (len(CurLine) == 2):
		if CurLine[1].lower() in Defines:
			MacroSkipBlock = False
		else:
			MacroSkipBlock = True
	elif (command == "%ifndef") and (len(CurLine) == 2):
		if CurLine[1].lower() in Defines:
			MacroSkipBlock = True
		else:
			MacroSkipBlock = False
	elif command == "%endif":
		MacroSkipBlock =  False
	elif command == "%else":
		MacroSkipBlock = not MacroSkipBlock
	else:
		print "Unable to parse %s, line %d: %s" % (command, linenumber, linedata)
		return 1

def HandleSpecialCommand(f, command, linedata, linenumber):
	global CurSection
	curdata = linedata.split()

	if command == "section":
		if len(curdata) < 2:
			print "Only expected a section name, line %d: %s" % (linenumber, linedata)
			return 1

		#swap to a new section
		CurSection = curdata[1]
		if CurSection not in Sections:
			Sections[CurSection] = dict()
			Sections[CurSection]["Name"] = CurSection
			Sections[CurSection]["Data"] = DataHandler(CurSection)
			Sections[CurSection]["Type"] = 1
			Sections[CurSection]["Link"] = 0
			Sections[CurSection]["Info"] = 0
			Sections[CurSection]["SizePerEntry"] = 1
			Sections[CurSection]["Offset"] = 0
			SectionArray.append(CurSection)

			FlagVal = 3	#WA
			if len(curdata) == 3:
				#we have flags
				Flags = curdata[2]
				FlagVal = 0
				for i in xrange(0, len(Flags)):
					if Flags[i].upper() == 'W':
						FlagVal |= 1
					elif Flags[i].upper() == 'A':
						FlagVal |= 2
					elif Flags[i].upper() == 'X':
						FlagVal |= 4
					else:
						print "Unknown flag options, line %d: %s" % (linenumber, linedata)
						return 1

			#set the flags
			Sections[CurSection]["Flags"] = FlagVal

	elif command == "extrn":
		if len(curdata) != 2:
			print "Only expected one parameter, line %d: %s" % (linenumber, linedata)
			return 1

		#add it to the list if not already marked for public
		if curdata[1] not in Externs:
			Externs.append(curdata[1])

	elif command in ["db", "dw", "dt"]:
		curdata = linedata[2:].split(",")

		#go through each entry and add it to the data
		if command == "db":
			OutputInt = Write1Byte
		elif command == "dw":
			OutputInt = Write2Bytes
		elif command == "dt":
			OutputInt = Write3Bytes

		CurPos = 2
		while(CurPos < len(linedata)):
			#skip space and tab
			while((CurPos < len(linedata)) and (linedata[CurPos] in [' ', '\t'])):
				CurPos += 1

			#if we hit the end without finding data we have an issue
			if (CurPos >= len(linedata)):
				print "Unexpected end of line %d: %s" % (linenumber, linedata)
				return 1

			#if we are on a comma then fail
			if linedata[CurPos] == ',':
				print "Empty field on line %d: %s" % (linenumber, linedata)
				return 1

			#figure out if string, hex, integer, or label
			if linedata[CurPos] == '"':
				#quote, find end of data
				NextPos = CurPos
				while(1):
					NextPos = linedata.find('"', NextPos+1)
					if NextPos == -1:
						print "Failed to find end of string, line %d: %s" % (linenumber, linedata)
						return 1
					elif linedata[NextPos-1] != '\\':
						#not escaped, break
						break

				#found the quote, write the data out after removing any escapes
				TempData = linedata[CurPos+1:NextPos]
				TempData = TempData.replace('\\"', '"')
				for i in xrange(0, len(TempData)):
					f.write(TempData[i] + chr(0))
				CurPos = NextPos + 1

			elif linedata[CurPos:CurPos+2] == '0x' or linedata[CurPos:CurPos+3] == '-0x':
				#hex value
				NextPos = linedata.find(",", CurPos)
				NextPos2 = linedata.find(" ", CurPos)

				#if we found a space and it is before the comma go to it
				if (NextPos2 != -1) and (NextPos2 < NextPos):
					NextPos = NextPos2

				HexStr = linedata[CurPos:NextPos]
				CurPos += len(HexStr)
				try:
					HexVal = int(HexStr, 16)
				except:
					print "Unknown hex value '%s' on line %d: %s" % (HexStr, linenumber, linedata)
					return 1

				#go write it's value
				if OutputInt(f, HexVal):
					return 1

			else:
				#should be an integer or a label reference
				NextPos = linedata.find(",", CurPos)
				NextPos2 = linedata.find(" ", CurPos)

				#if we found a space and it is before the comma go to it
				if (NextPos2 != -1) and (NextPos2 < NextPos):
					NextPos = NextPos2

				#if the end of the line then get it all
				if NextPos == -1:
					NextPos = len(linedata)

				IntStr = linedata[CurPos:NextPos]
				CurPos += len(IntStr)

				if IntStr.isdigit():
					try:
						IntVal = int(IntStr)
					except:
						print "Unknown int value '%s' on line %d: %s" % (HexStr, linenumber, linedata)
						return 1

					#go write it's value
					if OutputInt(f, IntVal):
						return 1
				else:
					#not numeric, must be a label, add it to the unknown list as it may expand sections and
					#should be filled in by the linker
					if IntStr[0] == "@":
						AbsoluteVal = 0
					else:
						AbsoluteVal = 1

					AddUnknownLabel(f, IntStr, linenumber, Absolute = AbsoluteVal, InstFormat=0)

					#make sure we output 3 bytes for it
					if OutputInt != Write3Bytes:
						print "Unable to reference labels unless defined as dt"
						return 1

					Write3Bytes(f, 0)

			#CurPos should be at the end of the data processed, remove any spaces and advance to the comma
			#error if we find anything else
			while(CurPos < len(linedata)):
				if linedata[CurPos] == ',':
					CurPos += 1
					break
				elif linedata[CurPos] in [' ', '\t']:
					CurPos += 1
				else:
					print "Unexpected data on line %d: %s" % (linenumber, linedata)
					return 1


	return 0

def AddUnknownLabel(f, labelname, linenumber, StartBit = 0, Bits = 27, ReadBytes = 3, Adjust = 0, Absolute = 1, InstFormat=-1):
	#ReadBytes = number of bytes to read
	#StartBit = bit from the left (high bit) to start writing at
	#Bits = number of bits to write

	if labelname not in UnknownLabelLocs:
			UnknownLabelLocs[labelname] = dict()
			UnknownLabelLocs[labelname]["name"] = labelname
			UnknownLabelLocs[labelname]["offsets"] = []

	if labelname[0] == "@":
		labelname = labelname[1:]
		Absolute = 0

	#add to the offsets list where we found the label reference
	NewOffset = dict()
	NewOffset["section"] = f.Name
	NewOffset["offset"] = f.tell()
	NewOffset["bits"] = Bits
	NewOffset["start_bit"] = StartBit
	NewOffset["read_bytes"] = ReadBytes
	NewOffset["line"] = linenumber
	NewOffset["adjust"] = Adjust
	NewOffset["absolute"] = Absolute
	NewOffset["instformat"] = InstFormat
	UnknownLabelLocs[labelname]["offsets"].append(NewOffset)
	return 0

def HandleNewLabel(f, linedata, linenumber):
	#if any spaces or tabs then fail
	if len(linedata.split()) != 1:
		print "Label '%s' is invalid. Space or tab found" % (linedata)
		return 1

	#remove the :
	linedata = linedata[0:-1]

	#if it already exists then fail
	if linedata in LabelLocs:
		print "Label '%s' already defined on line %d" % (linedata, LabelLocs[linedata]["line"])
		return 1

	#add a new entry
	LabelLocs[linedata] = dict()
	LabelLocs[linedata]["name"] = linedata
	LabelLocs[linedata]["line"] = linenumber
	LabelLocs[linedata]["section"] = f.Name
	LabelLocs[linedata]["offset"] = f.tell()

	#all good
	return 0

def SetupInstructions():
	#yes, this could be done with a lambda, i never liked lambdas
	for i in xrange(0, 29):
		Registers.append("R%d" % i)
	Registers.append("ST")
	Registers.append("RA")
	Registers.append("PC")

	#go load up every instruction and start adding it to our list
	pathdir = os.path.dirname(os.path.realpath(__file__))
	db = sqlite3.connect(pathdir + "/instructions.db")
	cur = db.cursor()
	cur.execute("select id, numbits, fieldorder, fieldname from instruction_format order by id, fieldorder")
	InstFormatDB = cur.fetchall()

	InstFormat = dict()
	InstBits = []

	#get format of all instructions
	for (InstID, NumBits, FieldOrder, FieldName) in InstFormatDB:
		if InstID not in InstFormat:
			InstFormat[InstID] = dict()
			InstFormat[InstID]["Fields"] = []
			InstFormat[InstID]["TotalBits"] = 0

		NewDict = dict()
		NewDict["Name"] = FieldName.upper()
		NewDict["BitShift"] = 0
		NewDict["NumBits"] = NumBits
		InstFormat[InstID]["Fields"].append(NewDict)
		InstFormat[InstID]["TotalBits"] += NumBits

	#now that we know how many bits exist, setup the bit shift for each
	for InstID in xrange(0, len(InstFormat)):
		BitShift = InstFormat[InstID]["TotalBits"]
		for i in xrange(0, len(InstFormat[InstID]["Fields"])):
			BitShift -= InstFormat[InstID]["Fields"][i]["NumBits"]
			InstFormat[InstID]["Fields"][i]["BitShift"] = BitShift

	cur = db.cursor()
	cur.execute("select id, name, instruction_format, description from instructions order by name")
	DBInstructions = cur.fetchall()

	#get every instruction and store the needed info
	for (InstID, InstName, InstFormatID, InstDescription) in DBInstructions:
		CurInstruction = dict()
		CurInstruction["Name"] = InstName
		CurInstruction["BitCount"] = InstFormat[InstFormatID]["TotalBits"]
		CurInstruction["Fields"] = InstFormat[InstFormatID]["Fields"]
		CurInstruction["Format"] = InstFormatID

		#create the fixed bit mask for this instruction
		cur = db.cursor()
		cur.execute("select id, fieldentry, value from instruction_detail where id=%d" % (int(InstID)))
		InstDetailDB = cur.fetchall()

		#first get all of the fields that are fixed
		InstructionBits = 0
		BitLayout = dict()
		for (InstDetailID, InstDetailFieldEntry, InstDetailValue) in InstDetailDB:
			BitLayout[int(InstDetailFieldEntry)] = int(InstDetailValue)

		#now create the instruction required bits based on fields
		for CurField in xrange(0, len(InstFormat[InstFormatID]["Fields"])):
			Field = InstFormat[InstFormatID]["Fields"][CurField]

			#if we have a bit fill it in
			if CurField in BitLayout:
				InstructionBits |= BitLayout[CurField] << Field["BitShift"]

		#we should have a full mask now for this instruction of fixed bits
		CurInstruction["InstructionBits"] = InstructionBits

		if InstName.lower() in ["b", "br", "c", "cr"]:
			CreateConditionalInstructions(CurInstruction, InstFormat[InstFormatID]["Fields"])
		elif InstName.lower() in ["lds", "ldw", "ldt", "sts", "stw", "stt"]:
			CreateLoadStoreInstructions(CurInstruction, InstFormat[InstFormatID]["Fields"])
		else:
			#add it to our global list
			Instructions[InstName.lower()] = CurInstruction

	#create a special move instruction to move a label address into a register
	#it will output as a low then high move
	CurInstruction = dict()
	CurInstruction["Name"] = "mi"
	CurInstruction["BitCount"] = -1
	CurInstruction["Format"] = -1
	Instructions["mi"] = CurInstruction

	return 0

def CreateConditionalInstructions(CurInstruction, Fields):
	#go create all of the variations of conditional checks
	Conds = ["n", "e", "l", "le", "g", "ge", "no", "o", "ns", "s", "sl", "sle", "sg", "sge", 0, ""]

	for CondField in Fields:
		if CondField["Name"] == "CONDITION":
			break

	#CondField is the condition area

	for i in xrange(0, len(Conds)):
		#14 is invalid
		if i == 14:
			continue
		NewInstruction = dict(CurInstruction)
		NewInstruction["Name"] += Conds[i]
		NewInstruction["InstructionBits"] |= (i << CondField["BitShift"])
		Instructions[NewInstruction["Name"].lower()] = NewInstruction
	return

def CreateLoadStoreInstructions(CurInstruction, Fields):
	#go create all of the variations of load/store
	Types = ["", "i", "d"]

	for TypeField in Fields:
		if TypeField["Name"] == "ADJUST RB":
			break

	#TypeField is the adjust area to set
	for i in xrange(0, len(Types)):
		NewInstruction = dict(CurInstruction)
		NewInstruction["Name"] += Types[i]
		NewInstruction["InstructionBits"] |= (i << TypeField["BitShift"])
		Instructions[NewInstruction["Name"].lower()] = NewInstruction
	return

def GetIntValue(InStr):
	if type(InStr) == int:
		return InStr

	if InStr[0:2] == '0x' or InStr[0:3] == '-0x':
		NumValue = int(InStr, 16)
	else:
		NumValue = int(InStr)
	return NumValue

def HandleFakeInstruction(f, linedata, linenumber):
	#handle a fake instruction that was found
	LineSplit = linedata.split()
	LineSplit[0] = LineSplit[0].lower()
	if LineSplit[0] == "mi":
		#need to call with ml then mh
		#first, figure out if we have a value to pass in
		try:
			NumValue = GetIntValue(LineSplit[2])
		except:
			AddUnknownLabel(f, LineSplit[2], linenumber, 0, -27, 3, Absolute=1, InstFormat=-2)
			NumValue = 0

		#now do ml then mh
		HandleInstruction(f, "ml %s %d" % (LineSplit[1], NumValue & 0x3ff), linenumber)
		HandleInstruction(f, "mh %s %d" % (LineSplit[1], (NumValue >> 10) & 0x1ffff), linenumber)
	return 0

def HandleInstruction(f, linedata, linenumber):
	LineSplit = linedata.split()

	#go check and see if we have the instruction
	LineSplit[0] = LineSplit[0].lower()
	if LineSplit[0][-1] == '.':
		#update flag set
		UF = 1
		LineSplit[0] = LineSplit[0][0:-1]
	else:
		UF = 0

	if LineSplit[0] not in Instructions:
		print "Unable to locate %s in instruction list. Line %d: %s" % (LineSplit[0], linenumber, linedata)
		return 1

	#go see how we need to parse up the data
	if Instructions[LineSplit[0]]["Format"] == -1:
		if(UF):
			print "Update flag set not allowed on instruction, line %d: %s" % (linenumber, linedata)
			return 1

		#fake instruction, call the handler for such
		return HandleFakeInstruction(f, linedata, linenumber)

	if Instructions[LineSplit[0]]["Format"] == 0:
		#no registers
		if(UF):
			print "Update flag set not allowed on instruction, line %d: %s" % (linenumber, linedata)
			return 1

		if len(LineSplit) > 1:
			print "Too many parameters, line %d: %s" % (linenumber, linedata)
			return 1

		#write the instruction out
		Write2Bytes(f, Instructions[LineSplit[0]]["InstructionBits"])

	elif Instructions[LineSplit[0]]["Format"] in [1, 14]:
		#rA, rB, rC

		#we should have 4 entries total
		if len(LineSplit) < 4:
			print "Expected 3 parameters, line %d: %s" % (linenumber, linedata)
			return 1

		if (LineSplit[1][-1] != ",") or (LineSplit[2][-1] != ","):
			print "Missing comma between parameters, line %d: %s" % (linenumber, linedata)
			return 1

		if len(LineSplit) > 4:
			print "Too many parameters, line %d: %s" % (linenumber, linedata)
			return 1

		#get rA, rB, rC
		r = []
		r.append(LineSplit[1][0:-1].upper())
		r.append(LineSplit[2][0:-1].upper())
		r.append(LineSplit[3].upper())
		for reg in r:
			if reg not in Registers:
				print "Unable to locate register %s, line %d: %s" % (reg, linenumber, linedata)
				return 1

		#make sure rA is not PC
		if r[0] == "PC":
			print "Unable to write to PC, line %d: %s" % (linenumber, linedata)
			return 1

		#go fill in the instruction
		Data = Instructions[LineSplit[0]]["InstructionBits"]
		RegOrder = ["RA","RB","RC"]
		for Field in Instructions[LineSplit[0]]["Fields"]:
			if Field["Name"] in RegOrder:
				Data |= Registers.index(r[RegOrder.index(Field["Name"])]) << Field["BitShift"]

		if(UF):
			if "UF" not in Instructions[LineSplit[0]]["Fields"]:
				print "Update flag set not allowed on instruction, line %d: %s" % (linenumber, linedata)
				return 1
			Data |= 1 << Instructions[LineSplit[0]]["Fields"]["UF"]["BitShift"]

		#write the instruction
		Write3Bytes(f, Data)

	elif Instructions[LineSplit[0]]["Format"] in [2, 15]:
		#rA, rB, Immediate

		#we should have 4 entries total
		if len(LineSplit) < 4:
			print "Expected 3 parameters, line %d: %s" % (linenumber, linedata)
			return 1

		if (LineSplit[1][-1] != ",") or (LineSplit[2][-1] != ","):
			print "Missing comma between parameters, line %d: %s" % (linenumber, linedata)
			return 1

		if len(LineSplit) > 4:
			print "Too many parameters, line %d: %s" % (linenumber, linedata)
			return 1

		#get rA, rB
		r = []
		r.append(LineSplit[1][0:-1].upper())
		r.append(LineSplit[2][0:-1].upper())
		for reg in r:
			if reg not in Registers:
				print "Unable to locate register %s, line %d: %s" % (reg, linenumber, linedata)
				return 1

		#make sure rA is not PC
		if r[0] == "PC":
			print "Unable to write to PC, line %d: %s" % (linenumber, linedata)
			return 1

		#go fill in the instruction
		Data = Instructions[LineSplit[0]]["InstructionBits"]
		RegOrder = ["RA","RB"]
		for Field in Instructions[LineSplit[0]]["Fields"]:
			if Field["Name"] in RegOrder:
				Data |= Registers.index(r[RegOrder.index(Field["Name"])]) << Field["BitShift"]

		#go find the immediate field
		for Field in Instructions[LineSplit[0]]["Fields"]:
			if Field["Name"] == "IMMEDIATE":
				break

		#go figure out the number and insert it
		NumValue = 0
		try:
			NumValue = GetIntValue(LineSplit[3])
		except:
			AddUnknownLabel(f, LineSplit[3], linenumber, Field["BitShift"], Field["NumBits"], 3, InstFormat=Instructions[LineSplit[0]]["Format"])
			NumValue = 0

		if abs(NumValue) >= (1 << Field["NumBits"]):
			print "Immediate 0x%x too large for %d bits, line %d: %s" % (NumValue, Field["NumBits"], linenumber, linedata)
			return 1

		Data |= (NumValue & ((1 << Field["NumBits"]) -1)) << Field["BitShift"]

		if(UF):
			if "UF" not in Instructions[LineSplit[0]]["Fields"]:
				print "Update flag set not allowed on instruction, line %d: %s" % (linenumber, linedata)
				return 1
			Data |= 1 << Instructions[LineSplit[0]]["Fields"]["UF"]["BitShift"]

		#write the instruction
		Write3Bytes(f, Data)

	elif Instructions[LineSplit[0]]["Format"] == 3:
		if(UF):
			print "Update flag set not allowed on instruction, line %d: %s" % (linenumber, linedata)
			return 1

		#load/store
		#XXXm rA, [rB + Offset, RegCount]
		#m is already set for us, go parse the rest of it
		if len(LineSplit) < 3:
			print "Expected at least 3 parameters, line %d: %s" % (linenumber, linedata)
			return 1

		if LineSplit[1][-1] != ',':
			print "Missing comma after first parameter, line %d: %s" % (linenumber, linedata)
			return 1

		if LineSplit[2][0] != '[':
			print "Missing opening bracket, line %d: %s" % (linenumber, linedata)
			return 1

		if LineSplit[-1][-1] != ']':
			print "Missing closing bracket, line %d: %s" % (linenumber, linedata)

		#seems formatted properly for our basic requirements
		r = []
		r.append(LineSplit[1][0:-1].upper())

		#see if we have any offset or register count to account for
		RegCountPos = -1
		Offset = 0

		if LineSplit[2][-1] == ',':
			#no offset but do have reg count
			r.append(LineSplit[2][1:-1].upper())
			RegCountPos = 3

		elif len(LineSplit) == 3:
			#no offset or reg count
			r.append(LineSplit[2][1:-1].upper())

		else:
			#we should have an offset
			r.append(LineSplit[2][1:].upper())

			#go get the offset
			if LineSplit[3][-1] == ',':
				#set reg count id
				RegCountPos = 4

				#should have +/- at beginning and 0x
				if LineSplit[3][0] not in ['-', '+']:
					print "Unable to determine offset, line %d: %s" % (linenumber, linedata)
					return 1
				else:
					Direction = 1
					if LineSplit[3][0] == '-':
						Direction = -1

				Offset = LineSplit[3][1:-1]

			elif LineSplit[3] in ["-","+"]:
				#found xxx rA, [rB +/- 0x...]
				Direction = 1
				if LineSplit[3] == '-':
					Direction = -1

				if LineSplit[4][-1] == ',':
					RegCountPos = 5
				elif len(LineSplit) != 5:
					print "Unable to parse offset, line %d: %s" % (linenumber, linedata)
					return 1

				#set the offset as we are either the end of the data or have the count after us
				Offset = LineSplit[4][0:-1]

		#if the register count was set then get it
		RegCountValue = 0
		if(RegCountPos != -1):
			#this should be the end of the data
			if len(LineSplit) != (RegCountPos + 1):
				#make sure there isn't a space after the reg count
				if (len(LineSplit) != (RegCountPos + 2)) or LineSplit[RegCountPos + 1] != ']':
					print "Failed to parse register count, line %d: %s" % (linenumber, linedata)
					return 1
				else:
					RegCount = LineSplit[RegCountPos]
			else:
				#skip ]
				RegCount = LineSplit[RegCountPos][0:-1]

			try:
				RegCountValue = GetIntValue(RegCount)
			except:
				print "Unable to parse register count, line %d: %s" % (linenumber, linedata)
				return 1

			#make sure reg count is valid
			if (RegCountValue < 1) or (RegCountValue > 32):
				print "Invalid register count, line %d: %s" % (linenumber, linedata)
				return 1

			#adjust for 0 to 31 values
			RegCountValue -= 1

		for reg in r:
			if reg not in Registers:
				print "Unable to locate register %s, line %d: %s" % (reg, linenumber, linedata)
				return 1

		#make sure rA is not PC if loading
		if (LineSplit[0][0] == 'L') and (r[0] == "PC"):
			print "Unable to write to PC, line %d: %s" % (linenumber, linedata)
			return 1

		#go fill in the instruction
		Data = Instructions[LineSplit[0]]["InstructionBits"]
		RegOrder = ["RA","RB"]
		for Field in Instructions[LineSplit[0]]["Fields"]:
			if Field["Name"] in RegOrder:
				Data |= Registers.index(r[RegOrder.index(Field["Name"])]) << Field["BitShift"]

		#go find the register count field
		for Field in Instructions[LineSplit[0]]["Fields"]:
			if Field["Name"] == "REGISTER COUNT":
				Data |= RegCountValue << Field["BitShift"]
				break

		#go find the immediate field
		for Field in Instructions[LineSplit[0]]["Fields"]:
			if Field["Name"] == "MEMORY OFFSET":
				break

		#try to convert the offset to a number
		try:
			NumValue = GetIntValue(Offset)
		except:
			AddUnknownLabel(f, Offset, linenumber, Field["BitShift"], Field["NumBits"], 6, InstFormat=Instructions[LineSplit[0]]["Format"])
			NumValue = 0

		Data = Data | (NumValue << Field["BitShift"])
		Write3Bytes(f, Data >> 27)

		#write out the actual value
		Write3Bytes(f, Data & 0x7ffffff)

	elif Instructions[LineSplit[0]]["Format"] in [4, 5, 12, 13, 16]:
		#rA, rB

		#we should have 3 entries total
		if len(LineSplit) == 2:
			#duplicate the 2nd as only 1 register was specified
			LineSplit.append(LineSplit[1])
			LineSplit[1] += ","

		if len(LineSplit) == 1:
			print "Expected 2 parameters, line %d: %s" % (linenumber, linedata)
			return 1

		if (LineSplit[1][-1] != ","):
			print "Missing comma between parameters, line %d: %s" % (linenumber, linedata)
			return 1

		if len(LineSplit) > 3:
			print "Too many parameters, line %d: %s" % (linenumber, linedata)
			return 1

		#get rA, rB
		r = []
		r.append(LineSplit[1][0:-1].upper())
		r.append(LineSplit[2].upper())
		for reg in r:
			if reg not in Registers:
				print "Unable to locate register %s, line %d: %s" % (reg, linenumber, linedata)
				return 1

		#make sure rA is not PC
		if r[0] == "PC":
			print "Unable to write to PC, line %d: %s" % (linenumber, linedata)
			return 1

		#go fill in the instruction
		Data = Instructions[LineSplit[0]]["InstructionBits"]
		RegOrder = ["RA","RB"]
		for Field in Instructions[LineSplit[0]]["Fields"]:
			if Field["Name"] in RegOrder:
				Data |= Registers.index(r[RegOrder.index(Field["Name"])]) << Field["BitShift"]

		if(UF):
			if "UF" not in Instructions[LineSplit[0]]["Fields"]:
				print "Update flag set not allowed on instruction, line %d: %s" % (linenumber, linedata)
				return 1
			Data |= 1 << Instructions[LineSplit[0]]["Fields"]["UF"]["BitShift"]

		#write the instruction
		if Instructions[LineSplit[0]]["Format"] in [5, 16]:
			Write2Bytes(f, Data)
		else:
			Write3Bytes(f, Data)

	elif Instructions[LineSplit[0]]["Format"] in [6, 7, 17]:
		#branch/call conditional
		if(UF):
			print "Update flag set not allowed on instruction, line %d: %s" % (linenumber, linedata)
			return 1

		#we should have 2 entries total
		if len(LineSplit) < 2:
			print "Expected 1 parameter, line %d: %s" % (linenumber, linedata)
			return 1

		if len(LineSplit) > 2:
			print "Too many parameters, line %d: %s" % (linenumber, linedata)
			return 1

		#go find the immediate field
		for Field in Instructions[LineSplit[0]]["Fields"]:
			if Field["Name"] == "OFFSET":
				break
			elif (Instructions[LineSplit[0]]["Format"] == 7) and (Field["Name"] == "LOCATION"):
				break;

		#go figure out the number and insert it
		NumValue = 0
		try:
			NumValue = GetIntValue(LineSplit[1])
		except:
			if Instructions[LineSplit[0]]["Format"] == 6:
				Bytes = 3
			else:
				Bytes = 4

			if LineSplit[0] not in ["bra", "caa"]:
				absolute = 0
			else:
				absolute = 1
			AddUnknownLabel(f, LineSplit[1], linenumber, Field["BitShift"], Field["NumBits"], Bytes, Absolute=absolute, InstFormat=Instructions[LineSplit[0]]["Format"])
			NumValue = 0

		if abs(NumValue) >= (1 << Field["NumBits"]):
			print "Immediate 0x%x too large for %d bits, line %d: %s" % (NumValue, Field["NumBits"], linenumber, linedata)
			return 1

		Data = Instructions[LineSplit[0]]["InstructionBits"]
		Data |= (NumValue & ((1 << Field["NumBits"]) - 1)) << Field["BitShift"]

		#write the instruction
		if Instructions[LineSplit[0]]["Format"] == 6:
			Write3Bytes(f, Data)
		else:
			Write4Bytes(f, Data)

	elif Instructions[LineSplit[0]]["Format"] in [8, 11]:
		#load immediate
		if(UF):
			print "Update flag set not allowed on instruction, line %d: %s" % (linenumber, linedata)
			return 1

		#we should have 3 entries total
		if len(LineSplit) < 3:
			print "Expected 2 parameters, line %d: %s" % (linenumber, linedata)
			return 1

		if (LineSplit[1][-1] != ","):
			print "Missing comma between parameters, line %d: %s" % (linenumber, linedata)
			return 1

		if len(LineSplit) > 3:
			print "Too many parameters, line %d: %s" % (linenumber, linedata)
			return 1

		#get rA
		reg = LineSplit[1][0:-1].upper()
		if reg not in Registers:
			print "Unable to locate register %s, line %d: %s" % (reg, linenumber, linedata)
			return 1

		#make sure rA is not PC
		if reg == "PC":
			print "Unable to write to PC, line %d: %s" % (linenumber, linedata)
			return 1

		#go fill in the instruction
		Data = Instructions[LineSplit[0]]["InstructionBits"]
		for Field in Instructions[LineSplit[0]]["Fields"]:
			if Field["Name"] == "RA":
				Data |= Registers.index(reg) << Field["BitShift"]
				break

		#go find the immediate field
		for Field in Instructions[LineSplit[0]]["Fields"]:
			if Field["Name"] == "IMMEDIATE":
				break

		#go figure out the number and insert it
		NumValue = 0
		try:
			NumValue = GetIntValue(LineSplit[2])
		except:
			AddUnknownLabel(f, LineSplit[2], linenumber, Field["BitShift"], Field["NumBits"], 3, InstFormat=Instructions[LineSplit[0]]["Format"])
			NumValue = 0

		if abs(NumValue) >= (1 << Field["NumBits"]):
			print "Immediate 0x%x too large for %d bits, line %d: %s" % (NumValue, Field["NumBits"], linenumber, linedata)
			return 1

		Data |= NumValue << Field["BitShift"]

		#write the instruction
		Write3Bytes(f, Data)

	elif Instructions[LineSplit[0]]["Format"] == 9:
		#branch register conditional
		if(UF):
			print "Update flag set not allowed on instruction, line %d: %s" % (linenumber, linedata)
			return 1

		#we should have 2 entries total
		if len(LineSplit) < 2:
			print "Expected 1 parameter, line %d: %s" % (linenumber, linedata)
			return 1

		if len(LineSplit) > 2:
			print "Too many parameters, line %d: %s" % (linenumber, linedata)
			return 1

		#get rA
		reg = LineSplit[1].upper()
		if reg not in Registers:
			print "Unable to locate register %s, line %d: %s" % (reg, linenumber, linedata)
			return 1

		#go fill in the instruction
		Data = Instructions[LineSplit[0]]["InstructionBits"]
		for Field in Instructions[LineSplit[0]]["Fields"]:
			if Field["Name"] == "RA":
				Data |= Registers.index(reg) << Field["BitShift"]
				break

		#write the instruction
		Write2Bytes(f, Data)

	elif Instructions[LineSplit[0]]["Format"] == 10:
		#memory protection
		if(UF):
			print "Update flag set not allowed on instruction, line %d: %s" % (linenumber, linedata)
			return 1

		#rA, rB, flags
		if LineSplit[0].upper() == "RMP":
			if len(LineSplit) != 3:
				print "Expected 2 parameters, line %d: %s" % (linenumber, linedata)
				return 1
			else:
				LineSplit[2] += ","
		elif len(LineSplit) != 4:
			print "Expected 3 parameters, line %d: %s" % (linenumber, linedata)
			return 1

		if (LineSplit[1][-1] != ",") or (LineSplit[2][-1] != ","):
			print "Missing comma between parameters, line %d: %s" % (linenumber, linedata)
			return 1

		#get rA, rB
		r = []
		r.append(LineSplit[1][0:-1].upper())
		r.append(LineSplit[2][0:-1].upper())
		for reg in r:
			if reg not in Registers:
				print "Unable to locate register %s, line %d: %s" % (reg, linenumber, linedata)
				return 1

		#go fill in the instruction
		Data = Instructions[LineSplit[0]]["InstructionBits"]
		RegOrder = ["RA","RB"]
		for Field in Instructions[LineSplit[0]]["Fields"]:
			if Field["Name"] in RegOrder:
				Data |= Registers.index(r[RegOrder.index(Field["Name"])]) << Field["BitShift"]

		#go find the flags field
		if len(LineSplit) == 4:
			for Field in Instructions[LineSplit[0]]["Fields"]:
				if Field["Name"] == "MEMORY FLAGS":
					break

			#go figure out the number and insert it
			NumValue = 0
			try:
				TextVals = ["N","R","RW","E"]
				if LineSplit[3].upper() in TextVals:
					NumValue = TextVals.index(LineSplit[3].upper())
				else:
					NumValue = GetIntValue(LineSplit[3])
			except:
				print "Unable to parse protection flag, line %d: %s" % (linenumber, linedata)
				return 1

			if abs(NumValue) >= (1 << Field["NumBits"]):
				print "Immediate 0x%x too large for %d bits, line %d: %s" % (NumValue, Field["NumBits"], linenumber, linedata)
				return 1

			Data |= NumValue << Field["BitShift"]

		#write the instruction
		Write3Bytes(f, Data)

	else:
		print "Unable to determine instruction format. Line %d: %s" % (linenumber, linedata)
		return 1

	return 0

def WriteSection(Section, ElfHeaderData, ElfData):
	global Alignment

	#add our section name
	StringID = AddStringEntry(Section["Name"])

	ElfHeaderData += struct.pack("<IIIIIIIIII", StringID, Section["Type"], Section["Flags"], 0, Section["Offset"], Section["Data"]._datalen, Section["Link"], Section["Info"], Alignment, Section["SizePerEntry"])


	#make sure the data is aligned then add in the data
	if len(ElfData) % Alignment:
		ElfData += "\x00" * (Alignment - (len(ElfData) % Alignment))
	ElfData += Section["Data"]._data

	return (ElfHeaderData, ElfData)

def Data_16_to_9(InData):
	#don't feel like doing all the math, make a bit list then reparse it
	Bits = ""
	for i in xrange(0, len(InData), 2):
		ByteBits = (ord(InData[i+1]) << 8) | ord(InData[i])
		Bits += (("0"*9)+bin(ByteBits)[2:])[-9:]

	#now add in enough bits to make sure we have full bytes
	if len(Bits) % 8:
		Bits += "0"*(8-(len(Bits) % 8))

	Output = ""
	for i in xrange(0, len(Bits), 8):
		Output += chr(int(Bits[i:i+8], 2))

	return Output

def CreateRaw():
	if CreateRelSections():
		return (1, "")

	if CreateSymbolSection():
		return (1, "")

	#create a raw file
	DataOffset = 0
	ElfHeaderData = ""
	ElfData = ""
	for SectionName in SectionArray:
		Section = Sections[SectionName]
		if Section["Type"] == 1:
			Section["Offset"] = DataOffset
			(ElfHeaderData, ElfData) = WriteSection(Section, ElfHeaderData, ElfData)
			DataOffset += Section["Data"]._datalen

	#9 bit pack it
	RawData = Data_16_to_9(ElfData)
	return (0, RawData)

def CreateElf():
	global Alignment

	if CreateRelSections():
		return (1, "")

	if CreateSymbolSection():
		return (1, "")

	SectionArray.append(".strtab")

	#create an elf file with all defined sections that have data
	#e_ident
	ElfHeaderData = "\x7fELF\x01\x01\x01\x00\x00" + "\x00"*7

	#e_type, e_machine, e_version, e_entry, e_phoff
	ElfHeaderData += struct.pack("<HHIII", 1, 0x4c43, 1, 0, 0)

	#section header sits after us, so 0x34 offset
	#e_shoff, e_flags, e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx
	ElfHeaderData += struct.pack("<IIHHHHHH", 0x34, 0, 0x34, 0, 0, 0x28, len(SectionArray) + 1, len(SectionArray))

	#add the undefined section header
	ElfHeaderData += "\x00"*(10*4)
	ElfData = ""

	#calculate where data will start at
	DataOffset = len(ElfHeaderData)
	DataOffset += (len(SectionArray) * (10*4))
	if DataOffset % Alignment:
		DataOffset += (Alignment - (DataOffset % Alignment))

	for SectionID in xrange(len(SectionArray)):
		Section = Sections[SectionArray[SectionID]]
		Section["Offset"] = DataOffset
		#print SectionID, binascii.b2a_hex(Section["Data"]._data)
		(ElfHeaderData, ElfData) = WriteSection(Section, ElfHeaderData, ElfData)
		DataOffset += Section["Data"]._datalen
		if (DataOffset % Alignment) and (SectionID != len(SectionArray) - 1):
			ElfData += "\x00" * (Alignment - (DataOffset % Alignment))
			DataOffset += (Alignment - (DataOffset % Alignment))

	#make sure the header is aligned before adding the data
	if len(ElfHeaderData) % Alignment:
		ElfHeaderData += "\x00" * (Alignment - (len(ElfHeaderData) % Alignment))

	#combine header and actual data
	ElfData = ElfHeaderData + ElfData

	return (0, ElfData)

def AddStringEntry(Str):
	#if we already added the string then just return it
	if Str in Sections[".strtab"]["Strings"]:
		return Sections[".strtab"]["Strings"][Str]

	CurIndex = Sections[".strtab"]["Data"].tell()
	Sections[".strtab"]["Data"].write(Str + "\x00")
	Sections[".strtab"]["Strings"][Str] = CurIndex
	return CurIndex

def AddSymbolEntry(Name, Value, Size, Bind, Type, SHNdx):
	NameIndex = AddStringEntry(Name)
	Info = (Bind << 4) | Type

	#print "Symbol", Name, hex(Value), Size, Bind, Type, SHNdx

	#two arrays, we don't want to mess up the order as dictionaries don't always come out the same way
	if Bind == 0:
		if Name in SymbolTextsLocal:
			return SymbolTextsLocal.index(Name)
		SymbolsLocal.append(struct.pack("<IIIBBH", NameIndex, Value, Size, Info, 0, SHNdx))
		SymbolTextsLocal.append(Name)
		Pos = len(SymbolsLocal) - 1
	else:
		SymbolsGlobal.append(struct.pack("<IIIBBH", NameIndex, Value, Size, Info, 0, SHNdx))
		Pos = len(SymbolsGlobal) - 1

	if Name not in SymbolsAdded:
		SymbolsAdded[Name] = [Bind, Pos]

	if Bind == 0:
		return len(SymbolsLocal) - 1

	#we can't report global locations yet as we don't know how many locals there are
	return 0

def FindSymbol(Name):
	if Name in SymbolsAdded:
		if SymbolsAdded[Name][0] == 0:
			return SymbolsAdded[Name][1]
		else:
			return len(SymbolsLocal) + SymbolsAdded[Name][1]
	return 0

def FixSymbols():
	#go through and determine if any unknown labels are now known and if we can fill them in
	#if they span section boundaries though then we can't fill it in
	for UnknownEntry in UnknownLabelLocs:
		#see if the referenced entry exists
		if UnknownEntry in LabelLocs:
			#exists, if it is in the same section then just fill in the offset otherwise we need to add a relocation
			for CurOffset in UnknownLabelLocs[UnknownEntry]["offsets"]:
				if (CurOffset["absolute"] == 0) and (CurOffset["section"] == LabelLocs[UnknownEntry]["section"]):
					#all good, get our offset
					CalcOffset = LabelLocs[UnknownEntry]["offset"] - CurOffset["offset"] - CurOffset["adjust"]

					#if the offset is too large error about it
					if abs(CalcOffset) & ~((1 << CurOffset["bits"]) - 1):
						print "Offset for %s at line %d to line %d is too large for command" % (UnknownEntry, CurOffset["line"], LabelLocs[UnknownEntry]["line"])
						return 1

					#add in the bits
					f = Sections[LabelLocs[UnknownEntry]["section"]]["Data"]
					f.seek(CurOffset["offset"], f.SEEK_SET)
					if CurOffset["read_bytes"] == 1:
						CurData = Read1Byte(f)
					elif CurOffset["read_bytes"] == 2:
						CurData = Read2Bytes(f)
					elif CurOffset["read_bytes"] == 3:
						CurData = Read3Bytes(f)
					elif CurOffset["read_bytes"] == 6:
						CurData = (Read3Bytes(f) << 27) | Read3Bytes(f)
					else:
						CurData = Read4Bytes(f)

					#mask and add in the bits
					CurDataMask = (1 << CurOffset["bits"]) - 1
					CurDataMask <<= CurOffset["start_bit"]
					CurData = CurData & ~CurDataMask

					#mask the offset value
					CalcOffset <<= CurOffset["start_bit"]
					CalcOffset = CalcOffset & CurDataMask

					#combine
					CurData |= CalcOffset

					#write back
					f.seek(CurOffset["offset"], f.SEEK_SET)
					if CurOffset["read_bytes"] == 1:
						Write1Byte(f, CurData)
					elif CurOffset["read_bytes"] == 2:
						Write2Bytes(f, CurData)
					elif CurOffset["read_bytes"] == 3:
						Write3Bytes(f, CurData)
					elif CurOffset["read_bytes"] == 6:
						Write3Bytes(f, CurData >> 27)
						Write3Bytes(f, CurData & 0x7ffffff)
					else:
						Write4Bytes(f, CurData)
				else:
					NewRelEntry = dict()
					NewRelEntry["section"] = CurOffset["section"]
					NewRelEntry["absolute"] = CurOffset["absolute"]
					NewRelEntry["bits"] = CurOffset["bits"]
					NewRelEntry["offset"] = CurOffset["offset"]
					NewRelEntry["label"] = UnknownEntry
					NewRelEntry["adjust"] = CurOffset["adjust"]
					NewRelEntry["unknown"] = 0
					NewRelEntry["instformat"] = CurOffset["instformat"]
					RelocList.append(NewRelEntry)
					continue
		else:
			for CurOffset in UnknownLabelLocs[UnknownEntry]["offsets"]:
				NewRelEntry = dict()
				NewRelEntry["section"] = CurOffset["section"]
				NewRelEntry["absolute"] = CurOffset["absolute"]
				NewRelEntry["bits"] = CurOffset["bits"]
				NewRelEntry["offset"] = CurOffset["offset"]
				NewRelEntry["adjust"] = CurOffset["adjust"]
				NewRelEntry["label"] = UnknownEntry
				NewRelEntry["unknown"] = 1
				NewRelEntry["instformat"] = CurOffset["instformat"]
				RelocList.append(NewRelEntry)
			continue
	return 0

def CreateSymbolSection():
	#now create all of the extern entries as they are known locations
	for CurExtern in Externs:
		if CurExtern not in LabelLocs:
			print "extern without a location during relocation"
			return 1

		if Sections[LabelLocs[CurExtern]["section"]]["Flags"] == 6:
			SymbolType = 2
		else:
			SymbolType = 1

		AddSymbolEntry(CurExtern, LabelLocs[CurExtern]["offset"], 0, 1, SymbolType, SectionArray.index(LabelLocs[CurExtern]["section"]) + 1)

	#now add the symbol data
	Sections[".symtab"]["Link"] = len(SectionArray) + 1
	Sections[".symtab"]["Info"] = len(SymbolsLocal)
	for Entry in SymbolsLocal:
		Sections[".symtab"]["Data"].write(Entry)
	for Entry in SymbolsGlobal:
		Sections[".symtab"]["Data"].write(Entry)
	return 0

def CreateRelSections():
	#create section relocation information

	#0x80000 | 1 - 24 bit move low then high
	#0x80000 | 3 - 4 byte relative call or branch
	#0x80000 | 4 - 14 bit move low
	#0x80000 | 5 - 3 byte absolute offset for load/store
	#0x80000 | 6 - 3 byte relative offset for load/store
	#0x80000 | 7 - 4 byte absolute call or branch
	#0x80000 | 26 - 3 byte relative call or branch

	RelsToAdd = []

	#add all relocations that require mapping due to unable to fill it in
	for CurRel in RelocList:
		CurSection = ".rels" + CurRel["section"]
		if CurSection not in Sections:
			NewSection = dict()
			NewSection["Name"] = CurSection
			NewSection["Data"] = DataHandler(CurSection)
			NewSection["Flags"] = 0
			NewSection["Type"] = 9
			NewSection["Link"] = SectionArray.index(".symtab") + 1
			NewSection["Info"] = SectionArray.index(CurRel["section"]) + 1
			NewSection["SizePerEntry"] = 8
			NewSection["Offset"] = 0
			Sections[CurSection] = NewSection
			SectionArray.append(CurSection)

		#if symbol is unknown then set as such otherwise fill in index and type
		if CurRel["unknown"]:
			SectionIndex = 0
			SymbolType = 2
			SymbolValue = 0
			SymbolSize = 0
		else:
			LabelEntry = LabelLocs[CurRel["label"]]
			SectionIndex = SectionArray.index(LabelEntry["section"]) + 1
			SymbolValue = LabelEntry["offset"]
			SymbolSize = 3
			if Sections[LabelEntry["section"]]["Flags"] & 1:
				SymbolType = 1
			else:
				SymbolType = 2

		#if we can find it then mark it as local
		if CurRel["label"] in LabelLocs:
			SymBind = 0
		else:
			SymBind = 1

		SymbolIndex = AddSymbolEntry(CurRel["label"], SymbolValue, SymbolSize, SymBind, SymbolType, SectionIndex)

		#figure out the type of relocation this is
		if CurRel["instformat"] == -2:
			RelType = 1
		elif CurRel["instformat"] == -1:
			print "Error in relocation info, unknown entry"
			return 1
		else:
			#we cheat and use instruction format 0 for values in DS that need absolute addresses filled in
			#example a data buffer
			RelTypeList = [[20, 2], [0], [0], [6, 5], [0], [0], [26], [3, 7], [4], [0], [0], [0], [0], [0], [0], [0], [0]]
			RelType = RelTypeList[CurRel["instformat"]]
			if len(RelType) == 2:
				RelType = RelType[CurRel["absolute"]]
			elif CurRel["absolute"] == 1:
				print "Invalid absolute relocation entry for instruction format %d, label %s" % (CurRel["instformat"], CurRel["label"])
				return 1
			else:
				RelType = RelType[0]
				if RelType == 0:
					print "Invalid relocation entry for instruction format %d, label %s" % (CurRel["instformat"], CurRel["label"])
					return 1

		#print "Reloc: %d %s %d %x" % (SymbolIndex, CurRel["label"], RelType, CurRel["offset"])

		RelsToAdd.append((CurRel["label"], RelType, CurRel["offset"]))

	#now go through each relocation entry and get the indexes as no more updates should occur
	for (LabelName, RelType, Offset) in RelsToAdd:
		RelType = (FindSymbol(LabelName) << 8) | RelType
		RelocData = struct.pack("<II", Offset, RelType)
		Sections[CurSection]["Data"].write(RelocData)

	return

def PrintHelp():
	print ""
	print "Lightning Assembler for cLEMENCy"
	print "The output is an ELF file to be linked with nld"
	print "Please see the provided help file for options and instruction formatting"
	print "Usage: %s asm.S -o asm.o [-r]" % (sys.argv[0])
	print "r - Output a raw file instead of an elf"
	print ""

def main():
	#open up the provided .S file, output the specified .o file
	inputfile = ""
	outputfile = ""
	rawfile = False

	i = 1
	while i < len(sys.argv):
		if sys.argv[i] == "-o" and (len(outputfile) == 0):
			if len(sys.argv) < i+1:
				print "Missing parameter to -o"
				return 1

			outputfile = sys.argv[i+1]
			i += 1
		elif sys.argv[i] == "-h":
			PrintHelp()
			return 0
		elif len(inputfile) == 0:
			inputfile = sys.argv[i]
		elif sys.argv[i] == "-r":
			rawfile = True
		elif sys.argv[i][0:2] == "-D":
			Defines[sys.argv[i][2:].lower()] = 0
		else:
			print "Unknown parameter:", sys.argv[i]
			return 1
		i += 1

	#if no input file then fail
	if len(inputfile) == 0:
		PrintHelp()
		return 0

	#make sure we can locate the file
	if os.path.isfile(inputfile) == False:
		print "Unable to open %s for input" % (inputfile)
		return 1

	indata = open(inputfile, "r").read().split("\n")
	if(ProcessAssembly(indata)):
		return 1

	#create the elf itself
	if rawfile:
		(ret, outdata) = CreateRaw()
	else:
		(ret, outdata) = CreateElf()

	#if failure then exit
	if ret:
		return 1

	#if no output file then use the current file and replace the .S with .o
	if(len(outputfile) == 0):
		outputfile = inputfile[:-1] + "o"

	try:
		open(outputfile,"w").write(outdata)
	except:
		print "Error writing to %s" % (outputfile)
		return 1

	print "Wrote %d bytes to %s" % (len(outdata), outputfile)
	return 0

sys.exit(main())
