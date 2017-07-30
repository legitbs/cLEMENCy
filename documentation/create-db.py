import sys, os
import sqlite3

CreateHeader = 1
db = sqlite3.connect("instructions.db")

#create our base math instructions
Instructions = [["ad", "add%s - rA = rB + rC", 1],
		["sb", "subtract%s - rA = rB - rC", 1],
		["mu", "multiply%s - rA = rB * rC", 2],
		["dv", "divide%s - rA = rB / rC", 2],
		["md", "modulus%s - rA = rB %% rC", 2],
		["an", "and%s - rA = rB & rC", 3],
		["or", "or%s - rA = rB | rC", 3],
		["xr", "xor%s - rA = rB ^ rC", 3],
		["adc", "add%s with carry - rA = rB + rC + Carry_Bit", 0],
		["sbc", "subtract%s with carry - rA = rB - rC - Carry_Bit", 0],
		]

if CreateHeader:
	header = open("clem-instructions.h","w")

for i in xrange(0, len(Instructions)):
	if CreateHeader:
		header.write("#define CLEM_%s\t\t0x%02x\n" % (Instructions[i][0].upper(), i))

	SubOps = [["", 0, 0, ""], ["f", 0, 1, " floating point"],["m", 1, 0, " multi reg"], ["fm", 1, 1, " floating point multi reg"]]
	for s in xrange(0, len(SubOps)):
		#no floating point for bitwise work or carry bit work
		if (Instructions[i][2] in [0, 3]) and (SubOps[s][2] == 1):
			continue

		db.execute("insert into instructions(name, instruction_format, description) values(?, 1, ?)", (Instructions[i][0] + SubOps[s][0], Instructions[i][1] % (SubOps[s][3])))
		cur = db.cursor()
		cur.execute("select max(id) from instructions")
		OpcodeID = int(cur.fetchone()[0])
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, i))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, ?)", (OpcodeID, SubOps[s][1]))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 2, ?)", (OpcodeID, SubOps[s][2]))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 6, 0)", (OpcodeID, ))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 7, 0)", (OpcodeID, ))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 8, 0)", (OpcodeID, ))

		#signed versions
		if (Instructions[i][2] == 2) and (SubOps[s][2] == 0):
			db.execute("insert into instructions(name, instruction_format, description) values(?, 1, ?)", (Instructions[i][0] + "s" + SubOps[s][0], Instructions[i][1] % (" signed " + SubOps[s][3])))
			cur = db.cursor()
			cur.execute("select max(id) from instructions")
			OpcodeID = int(cur.fetchone()[0])
			db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, i))
			db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, ?)", (OpcodeID, SubOps[s][1]))
			db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 2, ?)", (OpcodeID, SubOps[s][2]))
			db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 6, 0)", (OpcodeID, ))
			db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 7, 1)", (OpcodeID, ))
			db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 8, 0)", (OpcodeID, ))

		#no multi register for bitwise work
		if (Instructions[i][2] == 3) and (SubOps[s][1] == 1):
			continue

		#no floating point for immediate values
		if (SubOps[s][2] == 1):
			continue

		if CreateHeader and (len(SubOps[s][0]) == 0):
			header.write("#define CLEM_%s\t\t0x%02x\n" % (Instructions[i][0].upper() + "I", i))

		#immediate version
		db.execute("insert into instructions(name, instruction_format, description) values(?, 15, ?)", (Instructions[i][0] + "i" + SubOps[s][0], Instructions[i][1]  % (" immediate " + SubOps[s][3])))
		cur = db.cursor()
		cur.execute("select max(id) from instructions")
		OpcodeID = int(cur.fetchone()[0])
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, i))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, ?)", (OpcodeID, SubOps[s][1]))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 2, ?)", (OpcodeID, SubOps[s][2]))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 6, 0)", (OpcodeID, ))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 7, 1)", (OpcodeID, ))

		#signed versions
		if (Instructions[i][2] == 2) and (SubOps[s][2] == 0):
			db.execute("insert into instructions(name, instruction_format, description) values(?, 15, ?)", (Instructions[i][0] + "is" + SubOps[s][0], Instructions[i][1]  % (" immediate signed " + SubOps[s][3])))
			cur = db.cursor()
			cur.execute("select max(id) from instructions")
			OpcodeID = int(cur.fetchone()[0])
			db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, i))
			db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, ?)", (OpcodeID, SubOps[s][1]))
			db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 2, ?)", (OpcodeID, SubOps[s][2]))
			db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 6, 1)", (OpcodeID, ))
			db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 7, 1)", (OpcodeID, ))

db.commit()

OpCodeStart = len(Instructions)

ShiftInstructions = [
		["sl", "shift left%s - rA = rB << rC"],
		["sr", "shift right%s - rA = rB >> rC"],
		["", ""],
		["sa", "shift arithemetic right%s - rA = rB >> rC"],
		["rl", "rotate left%s - rA = rB rol rC"],
		["rr", "rotate right%s - rA = rB ror rC"]
		]
for i in xrange(0, len(ShiftInstructions)):
	if len(ShiftInstructions[i][0]) == 0:
		continue

	if CreateHeader:
		header.write("#define CLEM_%s\t\t0x%02x\n" % (ShiftInstructions[i][0].upper(), (i / 2) + OpCodeStart))


	SubOps = [["", 0, ""], ["m", 1, " multi reg"]]
	for s in xrange(0, len(SubOps)):
		db.execute("insert into instructions(name, instruction_format, description) values(?, 14, ?)", (ShiftInstructions[i][0] + SubOps[s][0], ShiftInstructions[i][1] % (SubOps[s][2])))
		cur = db.cursor()
		cur.execute("select max(id) from instructions")
		OpcodeID = int(cur.fetchone()[0])
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, (i / 2) + OpCodeStart))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, ?)", (OpcodeID, SubOps[s][1]))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 2, ?)", (OpcodeID, i % 2))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 6, 0)", (OpcodeID, ))

db.commit()
OpCodeStart += 3

DMAInstructions = [
		["dmt", "direct memory transfer - [rA:rA+rC] = [rB+rC]"]
		]
for i in xrange(0, len(DMAInstructions)):
	if CreateHeader:
		header.write("#define CLEM_%s\t\t0x%02x\n" % (DMAInstructions[i][0].upper(), i + OpCodeStart))

	db.execute("insert into instructions(name, instruction_format, description) values(?, 14, ?)", (DMAInstructions[i][0], DMAInstructions[i][1]))
	cur = db.cursor()
	cur.execute("select max(id) from instructions")
	OpcodeID = int(cur.fetchone()[0])
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, i + OpCodeStart))
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, 0)", (OpcodeID, ))
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 2, 0)", (OpcodeID, ))
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 6, 0)", (OpcodeID, ))
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 7, 0)", (OpcodeID, ))

db.commit()
OpCodeStart += len(DMAInstructions)


#shift immediate instructions
ShiftImmInstructions = [
			["sli", "shift left immediate%s - rA = rB << Imm"],
			["sri", "shift right immediate%s - rA = rB >> Imm"],
			["", ""],
			["sai", "shift arithemetic right immediate%s - rA = rB >> Imm"],
			["rli", "rotate left immediate%s - rA = rB rol Imm"],
			["rri", "rotate right immediate%s - rA = rB ror Imm"],
		  ]
for i in xrange(0, len(ShiftImmInstructions)):
	if len(ShiftImmInstructions[i][0]) == 0:
		continue

	if CreateHeader:
		header.write("#define CLEM_%s\t\t0x%02x\n" % (ShiftImmInstructions[i][0].upper(), (i / 2) + OpCodeStart))

	SubOps = [["", 0, 0, ""], ["m", 1, 0, " multi reg"]]
	for s in xrange(0, len(SubOps)):
		db.execute("insert into instructions(name, instruction_format, description) values(?, 2, ?)", (ShiftImmInstructions[i][0] + SubOps[s][0], ShiftImmInstructions[i][1] % (SubOps[s][3])))
		cur = db.cursor()
		cur.execute("select max(id) from instructions")
		OpcodeID = int(cur.fetchone()[0])
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, (i / 2) + OpCodeStart))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, ?)", (OpcodeID, SubOps[s][1]))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 2, ?)", (OpcodeID, i % 2))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 6, 0)", (OpcodeID, ))
db.commit()

OpCodeStart += 3

#immediate instructions
MoveImmInstructions = [
			["mh", "move high - rA = (Imm << 10) | (rA & 0x3ff)"],
			["ml", "move low - rA = Imm (zero extended)"],
			["ms", "move low signed - rA = Imm (sign extended)"],
		  ]
for i in xrange(0, len(MoveImmInstructions)):
	if CreateHeader:
		header.write("#define CLEM_%s\t\t0x%02x\n" % (MoveImmInstructions[i][0].upper(), i + OpCodeStart))

	db.execute("insert into instructions(name, instruction_format, description) values(?, 8, ?)", (MoveImmInstructions[i][0], MoveImmInstructions[i][1]))
	cur = db.cursor()
	cur.execute("select max(id) from instructions")
	OpcodeID = int(cur.fetchone()[0])
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, i + OpCodeStart))
db.commit()

OpCodeStart += len(MoveImmInstructions)

#our single opcode ones with flags
#branch return, interrupt return
SingleInstructions = [["re", "return", 0],["ir", "interrupt return", 0],["wt", "wait", 0], ["ht", "halt", 0], ["ei", "enable interrupts", 16], ["di", "disable interrupts", 16], ["hs", "hash", 13], ["ses", "Sign extend single", 13], ["sew", "sign extend word", 13], ["zes", "Zero extend single", 13], ["zew", "Zero extend word", 13], ["sf", "set flags", 16], ["rf", "read flags", 16]]
for i in xrange(0, len(SingleInstructions)):
	if(SingleInstructions[i][0] == "hs"):
		print "Skipping the HS instruction"
		continue
	if CreateHeader:
		header.write("#define CLEM_%s\t\t0x%02x\n" % (SingleInstructions[i][0].upper(), OpCodeStart))
		header.write("#define CLEM_%s_SUB\t0x%02x\n" % (SingleInstructions[i][0].upper(), 0))
		header.write("#define CLEM_%s_SUB2\t0x%02x\n" % (SingleInstructions[i][0].upper(), i))

	db.execute("insert into instructions(name, instruction_format, description) values(?, ?, ?)", (SingleInstructions[i][0], SingleInstructions[i][2], SingleInstructions[i][1]))
	cur = db.cursor()
	cur.execute("select max(id) from instructions")
	OpcodeID = int(cur.fetchone()[0])

	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, OpCodeStart))
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, 0)", (OpcodeID, ))
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 2, ?)", (OpcodeID, i))

	if SingleInstructions[i][2] == 0:
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 3, 0)", (OpcodeID,))
	else:
		if SingleInstructions[i][0] in ["ei","di","sf","rf"]:
			db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 4, 0)", (OpcodeID,))
		else:
			db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 5, 0)", (OpcodeID,))
db.commit()

FtoIConvertInstructions = [["fti", "float to integer", 1],["itf", "integer to float", 0]]
for i in xrange(0, len(FtoIConvertInstructions)):
	if CreateHeader:
		header.write("#define CLEM_%s\t\t0x%02x\n" % (FtoIConvertInstructions[i][0].upper(), OpCodeStart))
		header.write("#define CLEM_%s_SUB\t0x%02x\n" % (FtoIConvertInstructions[i][0].upper(), 1))
		header.write("#define CLEM_%s_SUB2\t0x%02x\n" % (FtoIConvertInstructions[i][0].upper(), FtoIConvertInstructions[i][2]))

	SubOps = [["", 0, ""], ["m", 1, " multi reg"]]
	for s in xrange(0, len(SubOps)):
		db.execute("insert into instructions(name, instruction_format, description) values(?, 12, ?)", (FtoIConvertInstructions[i][0] + SubOps[s][0], FtoIConvertInstructions[i][1] + SubOps[s][2]))
		cur = db.cursor()
		cur.execute("select max(id) from instructions")
		OpcodeID = int(cur.fetchone()[0])

		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, OpCodeStart))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, 1)", (OpcodeID, ))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 2, ?)", (OpcodeID, SubOps[s][1]))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 3, ?)", (OpcodeID, FtoIConvertInstructions[i][2]))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 6, 0)", (OpcodeID, ))
db.commit()

MemProtInstructions = [["smp", "Set Memory Protection", 1], ["rmp", "Read Memory Protection", 0]]
for i in xrange(0, len(MemProtInstructions)):
	if CreateHeader:
		header.write("#define CLEM_%s\t\t0x%02x\n" % (MemProtInstructions[i][0].upper(), OpCodeStart))
		header.write("#define CLEM_%s_SUB\t0x%02x\n" % (MemProtInstructions[i][0].upper(), 2))
		header.write("#define CLEM_%s_SUB2\t0x%02x\n" % (MemProtInstructions[i][0].upper(), MemProtInstructions[i][2]))

	db.execute("insert into instructions(name, instruction_format, description) values(?, 10, ?)", (MemProtInstructions[i][0], MemProtInstructions[i][1]))
	cur = db.cursor()
	cur.execute("select max(id) from instructions")
	OpcodeID = int(cur.fetchone()[0])
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, OpCodeStart))
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, 2)", (OpcodeID, ))
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 4, ?)", (OpcodeID, MemProtInstructions[i][2]))
	if(MemProtInstructions[i][2] == 0):
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 5, 0)", (OpcodeID,))
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 6, 0)", (OpcodeID, ))
db.commit()

#neg, not
SinglePInstructions = [
				["ng", "negate%s - rA = -rB"],
				["nt", "not%s - rA = !rB"],
				["bf", "bit flip%s - rA = ~rB"],
				["rnd", "random%s - rA = random"],
		      ]
for i in xrange(0, len(SinglePInstructions)):
	if CreateHeader:
		header.write("#define CLEM_%s\t\t0x%02x\n" % (SinglePInstructions[i][0].upper(), OpCodeStart))
		header.write("#define CLEM_%s_SUB\t0x%02x\n" % (SinglePInstructions[i][0].upper(), 3))
		header.write("#define CLEM_%s_SUB2\t0x%02x\n" % (SinglePInstructions[i][0].upper(), i))

	SubOps = [["", 0, 0, ""], ["f", 0, 1, " floating point"],["m", 1, 0, " multi reg"], ["fm", 1, 1, " floating point multi reg"]]
	for s in xrange(0, len(SubOps)):
		#random can't do floating point, it's just random bits
		if SinglePInstructions[i][0] != 'ng' and 'f' in SubOps[s][0]:
			continue

		db.execute("insert into instructions(name, instruction_format, description) values(?, 4, ?)", (SinglePInstructions[i][0] + SubOps[s][0], SinglePInstructions[i][1] % (SubOps[s][3])))
		cur = db.cursor()
		cur.execute("select max(id) from instructions")
		OpcodeID = int(cur.fetchone()[0])
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, OpCodeStart))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, 3)", (OpcodeID, ))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 2, ?)", (OpcodeID, SubOps[s][1]))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 3, ?)", (OpcodeID, SubOps[s][2]))
		if SinglePInstructions[i][0] == 'rnd':
			db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 5, 0)", (OpcodeID, ))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 6, ?)", (OpcodeID, i))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 7, 0)", (OpcodeID, ))
db.commit()
OpCodeStart += 1

#load/store
LoadStoreInstructions = [
				["lds","load single - rA->rX = [rB + Memory_Offset]"],
				["ldw","load word - rA->rX = [rB + Memory_Offset]"],
				["ldt","load tri - rA->rX = [rB + Memory_Offset]"],
				["sts","store single - [rB + Memory_Offset] = rA->rX"],
				["stw","store word - [rB + Memory_Offset] = rA->rX"],
				["stt","store tri - [rB + Memory_Offset] = rA->rX"]
			]

for i in xrange(0, len(LoadStoreInstructions)):
	if CreateHeader:
		header.write("#define CLEM_%s\t\t0x%02x\n" % (LoadStoreInstructions[i][0].upper(), (int(i / 3) + OpCodeStart)))
		header.write("#define CLEM_%s_SUB\t\t0x%02x\n" % (LoadStoreInstructions[i][0].upper(), (i % 3)))

	db.execute("insert into instructions(name, instruction_format, description) values(?, 3, ?)", (LoadStoreInstructions[i][0], LoadStoreInstructions[i][1]))
	cur = db.cursor()
	cur.execute("select max(id) from instructions")
	OpcodeID = int(cur.fetchone()[0])
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, (int(i / 3) + OpCodeStart)))
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, ?)", (OpcodeID, (i % 3)))
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 7, 0)", (OpcodeID, ))
db.commit()

OpCodeStart += (len(LoadStoreInstructions) / 3)

#compare
CmpInstructions = [["cm", "Compare"]]
for i in xrange(0, len(CmpInstructions)):
	if CreateHeader:
		header.write("#define CLEM_%s\t\t0x%02x\n" % (CmpInstructions[i][0].upper(), i + OpCodeStart))

	SubOps = [["", 0, 0, ""], ["f", 0, 1, " floating point"],["m", 1, 0, " multi reg"], ["fm", 1, 1, " floating point multi reg"]]
	for s in xrange(0, len(SubOps)):
		db.execute("insert into instructions(name, instruction_format, description) values(?, 5, ?)", (CmpInstructions[i][0] + SubOps[s][0], CmpInstructions[i][1] + SubOps[s][3]))
		cur = db.cursor()
		cur.execute("select max(id) from instructions")
		OpcodeID = int(cur.fetchone()[0])
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, i + OpCodeStart))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, ?)", (OpcodeID, SubOps[s][1]))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 2, ?)", (OpcodeID, SubOps[s][2]))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 3, 0)", (OpcodeID, ))
db.commit()

#compare immediate, uses the same base but low bit is flipped
CmpInstructionsImm = [["cmi", "Compare Immediate"]]
for i in xrange(0, len(CmpInstructionsImm)):
	if CreateHeader:
		header.write("#define CLEM_%s\t\t0x%02x\n" % (CmpInstructionsImm[i][0].upper(), i + OpCodeStart))

	SubOps = [["", 0, 0, ""], ["m", 1, 0, " multi reg"]]
	for s in xrange(0, len(SubOps)):
		db.execute("insert into instructions(name, instruction_format, description) values(?, 11, ?)", (CmpInstructionsImm[i][0] + SubOps[s][0], CmpInstructionsImm[i][1] + SubOps[s][3]))
		cur = db.cursor()
		cur.execute("select max(id) from instructions")
		OpcodeID = int(cur.fetchone()[0])
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, i + OpCodeStart))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, ?)", (OpcodeID, SubOps[s][1]))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 2, ?)", (OpcodeID, SubOps[s][2]))
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 3, 1)", (OpcodeID, ))
db.commit()

OpCodeStart += len(CmpInstructions)

#add in branch conditionals
BranchInstructions = [["b", "Branch", 6], ["br", "Branch Register", 9], ["c", "Call", 6], ["cr", "Call Register", 9]]
for i in xrange(0, len(BranchInstructions)):
	SubOps = [	["n", "Not Equal", 0],
			["e", "Equal", 1],
			["l", "Less Than", 2],
			["le", "Less Than or Equal", 3],
			["g", "Greater Than", 4],
			["ge", "Greater Than or Equal", 5],
			["no", "Not Overflow", 6],
			["o", "Overflow", 7],
			["ns", "Not Signed", 8],
			["s", "Signed", 9],
			["sl", "Signed Less Than", 10],
			["sle", "Signed Less Than or Equal", 11],
			["sg", "Signed Greater Than", 12],
			["sge", "Signed Greater Than or Equal", 13],
			#["no", "Not Overflow", 14],
			["", "Always", 15],
		]

	if CreateHeader:
		for s in SubOps:
			label = s[1].replace(" ", "_").upper()
			header.write("#define BRANCH_COND_%s\t%d\n" % (label, s[2]))

		header.write("#define CLEM_%s\t\t0x%02x\n" % (BranchInstructions[i][0].upper(), i + OpCodeStart))
		for s in xrange(0, len(SubOps)):
			header.write("#define CLEM_%s%s_COND\t\t0x%02x\n" % (BranchInstructions[i][0].upper(), SubOps[s][0].upper(), SubOps[s][2]))

	db.execute("insert into instructions(name, instruction_format, description) values(?, ?, ?)", (BranchInstructions[i][0], BranchInstructions[i][2], BranchInstructions[i][1] + " Conditional"))
	cur = db.cursor()
	cur.execute("select max(id) from instructions")
	OpcodeID = int(cur.fetchone()[0])
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, i + OpCodeStart))
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, ?)", (OpcodeID, int(i / 2)))

	#if the register branch then add in the reserved entry
	if(BranchInstructions[i][2] == 9):
		db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 4, 0)", (OpcodeID, ))

db.commit()

OpCodeStart += len(BranchInstructions)

BranchAbsoluteInstructions = [["brr", "Branch Relative",7], ["bra", "Branch Absolute",17], ["car", "Call Relative",7], ["caa", "Call Absolute",17]]
for i in xrange(0, len(BranchAbsoluteInstructions)):
	if CreateHeader:
		header.write("#define CLEM_%s\t\t0x%02x\n" % (BranchAbsoluteInstructions[i][0].upper(), OpCodeStart))
		header.write("#define CLEM_%s_SUB\t\t0x%02x\n" % (BranchAbsoluteInstructions[i][0].upper(), i))

	db.execute("insert into instructions(name, instruction_format, description) values(?, ?, ?)", (BranchAbsoluteInstructions[i][0], BranchAbsoluteInstructions[i][2], BranchAbsoluteInstructions[i][1]))
	cur = db.cursor()
	cur.execute("select max(id) from instructions")
	OpcodeID = int(cur.fetchone()[0])
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, OpCodeStart))
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, ?)", (OpcodeID, i))
	db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 2, 0)", (OpcodeID, ))
db.commit()

OpCodeStart += 1

#add in the debug break command
if(OpCodeStart == 0x1F):
	print "Conflict with DBRK!"

if CreateHeader:
	header.write("#define CLEM_DBRK\t\t0x1F\n")

db.execute("insert into instructions(name, instruction_format, description) values(?, 0, ?)", ("dbrk", "Debug break"))
cur = db.cursor()
cur.execute("select max(id) from instructions")
OpcodeID = int(cur.fetchone()[0])
db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 0, ?)", (OpcodeID, 0x1F))
db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 1, ?)", (OpcodeID, 0x03))
db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 2, ?)", (OpcodeID, 0x1F))
db.execute("insert into instruction_detail(id, FieldEntry, Value) values(?, 3, ?)", (OpcodeID, 0x3F))
db.commit()

db.close()

if CreateHeader:
	header.close()
