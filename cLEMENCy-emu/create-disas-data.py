#!/usr/bin/env python
import sys, os
import struct
import sqlite3

Instructions = dict()
Registers = []

def SetupInstructions():
	#go get all of the needed information for instructions

	#yes, this could be done with a lambda, i never liked lambdas
	for i in xrange(0, 28):
		Registers.append("R%d" % i)
	Registers.append("FL")
	Registers.append("ST")
	Registers.append("RA")
	Registers.append("PC")

	#go load up every instruction and start adding it to our list
	db = sqlite3.connect("instructions.db")
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

def main():
	SetupInstructions()

	f = open("disassembler_opcodes.h", "w")
	f.write("""//
//  disassembler_opcodes.h
//  cLEMENCy disassembler opcodes
//
//  Created by Lightning on 2016/10/24.
//  Copyright (c) 2017 Lightning. All rights reserved.
//
""")

	for Top5Bits in Instructions:
		for CurInst in Instructions[Top5Bits]:
			f.write("{0x%07x, 0x%07x, \"%s\", 0x%02x, %d, %d},\n" % (CurInst["InstructionBits"], CurInst["BitMask"], CurInst["Name"], Top5Bits, CurInst["BitCount"], CurInst["Format"]))

	f.close()
	return

main()
