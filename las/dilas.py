#!/usr/bin/env python
import sys, os
import struct
import sqlite3

#cLEMENCy disassembler
class DataHandler_RAW():
	SEEK_SET = 0
	SEEK_CUR = 1
	SEEK_END = 2

	def __init__(self, Filename):
		data = open(Filename, "r").read()
		self._CurPos = 0

		self._data = []
		pos = 0
		bitpos = 1
		while((pos+1) < len(data)):
			#bitpos 1 is high bit (0x80), 8 is low bit (0x01)
			curdata = (ord(data[pos]) & ((1 << (9-bitpos)) - 1)) << bitpos
			pos += 1

			#get next chunk of bits
			curdata |= (ord(data[pos]) >> (8-bitpos))

			#store bits
			self._data.append(curdata)

			#setup for next read
			bitpos += 1
			if bitpos == 9:
				pos += 1
				bitpos = 1

	def read(self, size):
		if (self._CurPos + size) >= len(self._data):
			size = len(self._data) - self._CurPos

		if size:
			data = self._data[self._CurPos:self._CurPos+size]
		else:
			data = [0]
		self._CurPos += size
		return data

	def read1byte(self):
		return self.read(1)[0]

	def read2bytes(self):
		data = self.read(2)
		return (data[1] << 9) | data[0]

	def read3bytes(self):
		data = self.read(3)
		return (data[1] << 18) | (data[0] << 9) | data[2]

	def read4bytes(self):
		data = self.read(3)
		return (data[1] << 27) | (data[0] << 18) | (data[2] << 9) | data[3]

	def seek(self, pos, seekset):
		if seekset == self.SEEK_SET:
			pass
		elif seekset == self.SEEK_CUR:
			pos += self._CurPos
		else:
			pos = len(self._data) - pos
			if pos < 0:
				pos = 0

		if pos > len(self._data):
			pos = len(self._data)

		self._CurPos = pos
		return pos

	def left(self):
		return len(self._data) - self._CurPos

	def tell(self):
		return self._CurPos

class DataHandler_ELF(DataHandler_RAW):
	def __init__(self, Filename, section=".cs"):
		data = open(Filename, "r").read()
		self._CurPos = 0

		self._data = []

		#parse up the elf header and find our section which may be a name or number
		e_shoff = 16+2+2+4+4+4
		e_shoff = struct.unpack("<I", data[e_shoff:e_shoff+4])[0]
		e_shentsize = 16+2+2+4+4+4+4+4+2+2+2
		(e_shentsize, e_shnum, e_shstrndx) = struct.unpack("<HHH", data[e_shentsize:e_shentsize+6])

		#parse each section up and see if we can find the matching section
		if section.isdigit():
			section = int(section)
			(sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size) = struct.unpack("<IIIIII", data[e_shoff + (section*0x40):e_shoff + (section*0x40) + 24])
		else:
			#get the string section location
			strpos = struct.unpack("<I", data[e_shoff+(e_shstrndx*0x28)+16 : e_shoff+(e_shstrndx * 0x28)+20])[0]

			#go find the section
			section += "\x00"
			found = False
			for i in xrange(0, e_shnum):
				(sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size) = struct.unpack("<IIIIII", data[e_shoff + (i*0x28):e_shoff + (i*0x28) + 24])
				if data[strpos+sh_name:strpos+sh_name+len(section)] == section:
					#found it, stop
					found = True
					break

			if(not found):
				print "Failed to find section %s" % (section[0:-1])
				sys.exit(0)

		#create the data array
		for i in xrange(0, sh_size, 2):
			self._data.append(struct.unpack("<H", data[sh_offset+i:sh_offset+i+2])[0])

Instructions = dict()
Registers = []

def PrintHelp():
	print ""
	print "Lightning Disassemler for cLEMECy"
	print "The output is an assembly dump of the raw binary data provided"
	print "Usage: %s [-b] [-o output] [-l x] [-c x] input" % (sys.argv[0])
	print "\t-f\tDisassemble a raw firmware file instead of ELF"
	print "\t-o\tWrite data to an output file instead of stdout"
	print "\t-l\tStart disassembling at location x, value in hex"
	print "\t-c x\tOutput x number of lines"
	print ""
	return

def SetupInstructions():
	#go get all of the needed information for instructions

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
		BitMask = 0
		InstructionBits = 0
		BitLayout = dict()
		for (InstDetailID, InstDetailFieldEntry, InstDetailValue) in InstDetailDB:
			BitLayout[int(InstDetailFieldEntry)] = int(InstDetailValue)

		#now create the mask based on fields
		BitOffset = InstFormat[InstFormatID]["TotalBits"]
		for CurField in xrange(0, len(InstFormat[InstFormatID]["Fields"])):
			Field = InstFormat[InstFormatID]["Fields"][CurField]

			#if we have a bit fill it in
			if CurField in BitLayout:
				InstructionBits |= BitLayout[CurField] << Field["BitShift"]
				BitMask |= (0xffffffffffffffff >> (64 - Field["NumBits"])) << Field["BitShift"]

		#we should have a full mask now for this instruction of fixed bits
		CurInstruction["InstructionBits"] = InstructionBits
		CurInstruction["BitMask"] = BitMask

		#add it to our global list grouped on the 5 bit setup for easy lookup
		Top5Bits = CurInstruction["InstructionBits"] >> (CurInstruction["BitCount"] - 5)
		if Top5Bits not in Instructions:
			Instructions[Top5Bits] = []

		if InstName.lower() in ["b", "br", "c", "cr"]:
			CreateConditionalInstructions(CurInstruction, InstFormat[InstFormatID]["Fields"])
		elif InstName.lower() in ["lds", "ldw", "ldt", "sts", "stw", "stt"]:
			CreateLoadStoreInstructions(CurInstruction, InstFormat[InstFormatID]["Fields"])
		else:
			Instructions[Top5Bits].append(CurInstruction)
	return 0

def CreateConditionalInstructions(CurInstruction, Fields):
	#go create all of the variations of conditional checks
	Conds = ["n", "e", "l", "le", "g", "ge", "no", "o", "ns", "s", "sl", "sle", "sg", "sge", 0, ""]

	#get the top 5 bits
	Top5Bits = CurInstruction["InstructionBits"] >> (CurInstruction["BitCount"] - 5)

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
		NewInstruction["BitMask"] |= (0xffffffffffffffff >> (64 - CondField["NumBits"])) << CondField["BitShift"]
		Instructions[Top5Bits].append(NewInstruction)
	return

def CreateLoadStoreInstructions(CurInstruction, Fields):
	#go create all of the variations of load/store
	Types = ["", "i", "d"]

	#get the top 5 bits
	Top5Bits = CurInstruction["InstructionBits"] >> (CurInstruction["BitCount"] - 5)

	for TypeField in Fields:
		if TypeField["Name"] == "ADJUST RB":
			break

	#TypeField is the adjust area to set
	for i in xrange(0, len(Types)):
		NewInstruction = dict(CurInstruction)
		NewInstruction["Name"] += Types[i]
		NewInstruction["InstructionBits"] |= (i << TypeField["BitShift"])
		NewInstruction["BitMask"] |= (0xffffffffffffffff >> (64 - TypeField["NumBits"])) << TypeField["BitShift"]
		Instructions[Top5Bits].append(NewInstruction)
	return

def GetValue(InValue, Field):
	BitShift = Field["BitShift"]
	BitCount = Field["NumBits"]
	Mask = (0xffffffffffffffff >> (64 - BitCount)) << BitShift
	return (InValue & Mask) >> BitShift

def DisassembleData(FileData, LineCount):
	#keep parsing all the data
	Assembly = ""
	CurLines = 0
	while(FileData.left() >= 2):
		if(LineCount and (CurLines >= LineCount)):
			break

		CurLines += 1
		CurPos = FileData.tell()
		Assembly += "%07x: " % (CurPos)

		#get the instruction info
		(CurAssembly, CurInst, CurInstStr, ErrorMsg) = DisassembleInstruction(FileData)
		Assembly += CurInstStr + " "*(16-len(CurInstStr))
		if(len(Assembly) == 0):
			break

		if(len(ErrorMsg)):
			if len(CurInstStr) == 0:
				Assembly += "%03x %03x %03x " % ((CurInst >> 18) & 0x1ff, (CurInst >> 9) & 0x1ff, CurInst & 0x1ff)
			Assembly += ErrorMsg + "\n"
			break

		Assembly += CurAssembly + "\n"

	return Assembly

def DisassembleInstruction(FileData):
	#keep parsing all the data
	Assembly = ""

	#make sure we have data to process
	if FileData.left() < 2:
		return ("", 0, "", "")

	CurPos = FileData.tell()
	CurInst = FileData.read2bytes()

	#go find the entry for it
	Top5Bits = CurInst >> (18 - 5)
	if Top5Bits not in Instructions:
		return (Assembly, CurInst, "", "Unable to locate %05x" % (CurInst))

	#align for this grouping
	BitCount = 0
	Found = False
	for i in xrange(len(Instructions[Top5Bits])):
		InstBits = Instructions[Top5Bits][i]["InstructionBits"]
		BitMask = Instructions[Top5Bits][i]["BitMask"]
		InstBits = InstBits >> (Instructions[Top5Bits][i]["BitCount"] - 18)
		BitMask = BitMask >> (Instructions[Top5Bits][i]["BitCount"] - 18)
		if (CurInst & BitMask) == (InstBits & BitMask):
			BitCount = Instructions[Top5Bits][i]["BitCount"]
			Found = True
			break

	if not Found:
		return (Assembly, CurInst, "", "Unable to locate %05x" % (CurInst))

	if BitCount == 18:
		CurInstStr = "%05x" % (CurInst)
	elif BitCount == 27:
		#make sure we have data to process
		if FileData.left() < 1:
			return ("", 0, "", "")

		CurInst = (CurInst << 9) | FileData.read1byte()
		CurInstStr = "%07x" % (CurInst)
	elif BitCount == 36:
		#make sure we have data to process
		if FileData.left() < 2:
			return ("", 0, "", "")

		CurInst = (CurInst << 18) | (FileData.read1byte() << 9) | FileData.read1byte()
		CurInstStr = "%09x" % (CurInst)
	elif BitCount == 54:
		#make sure we have data to process
		if FileData.left() < 3:
			return ("", 0, "", "")

		CurInst = (((CurInst << 9) | FileData.read1byte()) << 27) | FileData.read3bytes()
		CurInstStr = "%014x" % (CurInst)

	Found = False
	for CurInstruction in Instructions[Top5Bits]:
		a = CurInstruction["BitMask"] & CurInst
		if (CurInstruction["BitMask"] & CurInst) == CurInstruction["InstructionBits"]:
			Found = True
			break

	if Found == False:
		return (Assembly, CurInst, CurInstStr, "Disassembly Error")

	Assembly += "%s" % (CurInstruction["Name"])

	#figure out the type of instruction we are looking at
	if CurInstruction["Format"] == 0:
		#just print it
		pass

	elif CurInstruction["Format"] in [1, 14]:
		#rA, rB, rC
		r = [0, 0, 0]
		BitPositions = ["RA", "RB", "RC"]
		UF = ""
		for Fields in CurInstruction["Fields"]:
			if Fields["Name"] in BitPositions:
				r[BitPositions.index(Fields["Name"])] = GetValue(CurInst, Fields)
			elif Fields["Name"] == "UF":
				if GetValue(CurInst, Fields):
					UF = "."

		Assembly += "%s\t%s, %s, %s" % (UF, Registers[r[0]], Registers[r[1]], Registers[r[2]])

	elif CurInstruction["Format"] in [2, 15]:
		#rA, rB, Immediate
		r = [0, 0]
		Imm = 0
		BitPositions = ["RA", "RB"]
		UF = ""
		for Fields in CurInstruction["Fields"]:
			if Fields["Name"] in BitPositions:
				r[BitPositions.index(Fields["Name"])] = GetValue(CurInst, Fields)
			elif Fields["Name"] == "IMMEDIATE":
				Imm = GetValue(CurInst, Fields)
			elif Fields["Name"] == "UF":
				if GetValue(CurInst, Fields):
					UF = "."

		Assembly += "%s\t%s, %s, 0x%x" % (UF, Registers[r[0]], Registers[r[1]], Imm)

	elif CurInstruction["Format"] == 3:
		#load/store
		#XXXm rA, [rB + Offset, RegCount]
		r = [0, 0]
		Offset = 0
		RegCount = 0
		BitPositions = ["RA", "RB"]
		for Fields in CurInstruction["Fields"]:
			if Fields["Name"] in BitPositions:
				r[BitPositions.index(Fields["Name"])] = GetValue(CurInst, Fields)
			elif Fields["Name"] == "REGISTER COUNT":
				RegCount = GetValue(CurInst, Fields)
			elif Fields["Name"] == "MEMORY OFFSET":
				Offset = GetValue(CurInst, Fields)
				break


		Assembly += "\t%s, [%s" % (Registers[r[0]], Registers[r[1]])
		if Offset != 0:
			if Offset & 0x4000000:
				Assembly += " - 0x%x" % (-Offset & 0x7ffffff)
			else:
				Assembly += " + 0x%x" % (Offset)

		if(RegCount):
			Assembly += ", %d" % (RegCount + 1)
		Assembly += "]"


	elif CurInstruction["Format"] in [4, 5, 12, 13]:
		#rA, rB
		r = [0, 0]
		BitPositions = ["RA", "RB"]
		UF = ""
		for Fields in CurInstruction["Fields"]:
			if Fields["Name"] in BitPositions:
				r[BitPositions.index(Fields["Name"])] = GetValue(CurInst, Fields)
			elif Fields["Name"] == "UF":
				if GetValue(CurInst, Fields):
					UF = "."

		Assembly += "%s\t%s, %s" % (UF, Registers[r[0]], Registers[r[1]])

	elif CurInstruction["Format"] in [6, 7]:
		#branch/call conditional
		Offset = 0
		for Fields in CurInstruction["Fields"]:
			if Fields["Name"] == "OFFSET":
				Offset = GetValue(CurInst, Fields)
				break

		if (CurInstruction["Format"] == 7) and (Offset & 0x4000000):
			Assembly += "\t-0x%x (0x%07x)" % (-Offset & 0x7ffffff, CurPos - (-Offset & 0x7ffffff))
		elif (CurInstruction["Format"] == 6) and (Offset & 0x10000):
			Assembly += "\t-0x%x (0x%07x)" % (-Offset & 0x1ffff, CurPos - (-Offset & 0x1ffff))
		else:
			Assembly += "\t+0x%x (0x%07x)" % (Offset, (CurPos + Offset) & 0x7ffffff)

	elif CurInstruction["Format"] == 17:
		#branch/call conditional
		Location = 0
		for Fields in CurInstruction["Fields"]:
			if Fields["Name"] == "LOCATION":
				Location = GetValue(CurInst, Fields)
				break

		Assembly += "\t0x%x" % (Location)

	elif CurInstruction["Format"] in [8, 11]:
		#load immediate
		#rA, Imm
		RA = 0
		Imm = 0
		for Fields in CurInstruction["Fields"]:
			if Fields["Name"] == "RA":
				RA = GetValue(CurInst, Fields)
			elif Fields["Name"] == "IMMEDIATE":
				Imm = GetValue(CurInst, Fields)
				break

		Assembly += "\t%s, 0x%x" % (Registers[RA], Imm)

	elif CurInstruction["Format"] == 9:
		#branch register conditional
		RA = 0
		for Fields in CurInstruction["Fields"]:
			if Fields["Name"] == "RA":
				RA = GetValue(CurInst, Fields)
				break

		Assembly += "\t%s" % (Registers[RA])


	elif CurInstruction["Format"] == 10:
		#memory protection
		#rA, rB, flags
		r = [0, 0]
		Flags = 0
		BitPositions = ["RA", "RB"]
		WriteInst = 0
		for Fields in CurInstruction["Fields"]:
			if Fields["Name"] in BitPositions:
				r[BitPositions.index(Fields["Name"])] = GetValue(CurInst, Fields)
			elif Fields["Name"] == "WRITE":
				WriteInst = GetValue(CurInst, Fields)
			elif Fields["Name"] == "MEMORY FLAGS":
				Flags = GetValue(CurInst, Fields)
				break

		FlagType = ["N", "R", "RW", "E"]
		Assembly += "\t%s, %s" % (Registers[r[0]], Registers[r[1]])
		if WriteInst:
			Assembly += ", %s" % (FlagType[Flags])
	elif CurInstruction["Format"] == 16:
		#rA
		r = 0
		for Fields in CurInstruction["Fields"]:
			if Fields["Name"] == "RA":
				r = GetValue(CurInst, Fields)

		Assembly += "\t%s" % (Registers[r])
	else:
		#invalid
		pass

	return (Assembly, CurInst, CurInstStr, "")

def dilas_main():
	inputfile = ""
	outputfile = "/dev/stdout"
	binformat = 0
	Offset = 0
	LineCount = 0

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
			LineCount = int(sys.argv[i+1])
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
	Output = DisassembleData(indata, LineCount)

	print "Writing to %s" % (outputfile)
	open(outputfile,"w").write(Output)
	return

SetupInstructions()

if __name__ == "__main__":
	dilas_main()
