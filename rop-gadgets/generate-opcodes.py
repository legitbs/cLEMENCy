import sys, os
import random

IR = "101000000001000000"
ST = "11101"
ST2 = "11100"

AN = ["0010100", ST, 0, 0, "00001"]
ANM = ["0010110", ST, 0, 0, "00001"]
ANM2 = ["0010110", ST2, 0, 0, "00001"]
OR = ["0011000", ST, 0, 0, "00001"]
ORM = ["0011010", ST, 0, 0, "00001"]
ORM2 = ["0011010", ST2, 0, 0, "00001"]
BF = ["101001100", ST, 0, "10000001" + "101001100" + ST + ST + "10000001"]
MDI = ["0010000", ST, 0, "0000001011"]
MDIM = ["0010010", ST, 0, "0000001011"]
MUI = ["0001000", ST, 0, "0000001011"]
MUIM = ["0001010", ST, 0, "0000001011"]
NG = ["101001100", ST, 0, "00000001" + "101001100" + ST + ST + "00000001"]
ORI = ["0011000", ST, 0, "0000100011"]

def GetRegister(Register):
	return ("0"*5 + bin(Register)[2:])[-5:]

def Rand5Bits(InvalidMatch):
	Bits = ""
	while(1):
		Bits = ("0"*5 + bin(random.randint(0, (1<<5)-1))[2:])[-5:]
		if int(Bits, 2) not in InvalidMatch:
			break

	return Bits

def RandImm(BitLen):
	return ("0"*BitLen + bin(random.randint(0, (1<<BitLen)-1))[2:])[-BitLen:]

def Write2Bytes(Bits):
	return Bits[9:18] + Bits[0:9]

def Write3ByteGroups(Bits):
	OutBits = ""
	for i in xrange(0, len(Bits), 27):
		OutBits += Bits[i+9:i+18] + Bits[i+0:i+9] + Bits[i+18:i+27]
	return OutBits

def GetRandom27BitInstruction():
	RandEntry = random.randint(6, 13)
	if RandEntry == 0:	#RR
		Data = "0110001" + Rand5Bits([29,31]) + Rand5Bits([-1]) + Rand5Bits([-1]) + "00001"
	elif RandEntry == 1:	#RL
		Data = "0110000" + Rand5Bits([29,31]) + Rand5Bits([-1]) + Rand5Bits([-1]) + "00001"
	elif RandEntry == 2:	#SL
		Data = "0101000" + Rand5Bits([29,31]) + Rand5Bits([-1]) + Rand5Bits([-1]) + "00001"
	elif RandEntry == 3:	#SL
		Data = "0101001" + Rand5Bits([29,31]) + Rand5Bits([-1]) + Rand5Bits([-1]) + "00001"
	elif RandEntry == 4:	#RND
		Data = "101001100" + Rand5Bits([29,31]) + "0000011000001"
	elif RandEntry == 5:	#MDS
		Data = "0010000" + Rand5Bits([29,31]) + Rand5Bits([-1]) + Rand5Bits([-1]) + "00101"
	elif RandEntry == 6:	#BF
		Data = "101001100" + Rand5Bits([29,31]) + Rand5Bits([-1]) + "10000001"
	elif RandEntry == 7:	#CMI
		Data = "10111001" + Rand5Bits([-1]) + RandImm(14)
	elif RandEntry == 8:	#MDI
		Data = "0010000" + Rand5Bits([29,31]) + Rand5Bits([-1]) + RandImm(7) + "011"
	elif RandEntry == 9:	#ANI
		Data = "0010100" + Rand5Bits([29,31]) + Rand5Bits([-1]) + RandImm(7) + "011"
	elif RandEntry == 10:	#MH
		Data = "10001" + Rand5Bits([29,31]) + RandImm(17)
	elif RandEntry == 11:	#ML
		Data = "10010" + Rand5Bits([29,31]) + RandImm(17)
	elif RandEntry == 12:	#MS
		Data = "10011" + Rand5Bits([29,31]) + RandImm(17)
	elif RandEntry == 13:	#MUI
		Data = "0001000" + Rand5Bits([29,31]) + Rand5Bits([-1]) + RandImm(7) + "011"

	return Data

f = open("test.bin","w")

Entries = [AN, ANM, ANM2, OR, ORM, ORM2, MDI, MDIM, MUI, MUIM, ORI]
#Entries = [ANM2, ORM2, XRM2]
#for Register in xrange(0, 28):
Register = int(sys.argv[1])
for Entry in Entries:
	Count = 0
	for a in xrange(0, 100):
		if Entry[3] != 0:
			Data = Entry[0] + Entry[1] + GetRegister(Register) + Entry[3]
		elif (Entry[1] == ST2) and (Register > 0):
			Data = Entry[0] + Entry[1] + GetRegister(Register-1) + GetRegister(Register-1) + Entry[4]
		else:
			Data = Entry[0] + Entry[1] + GetRegister(Register) + GetRegister(Register) + Entry[4]

		#bit swap everything for middle endian
		#Data = Write3ByteGroups(Data)

		#if len(Data) % 8:
		#	Data += "1"*(8 - (len(Data) % 8))


		Data += GetRandom27BitInstruction() + GetRandom27BitInstruction()


		#Data += "1"*(27+27)
		Data = Write3ByteGroups(Data) #  "1"*(27+27)
		#pad Data so that IR starts on a 0 offset
		Offset = 8-(len(Data) % 8)
		Data = "1"*Offset + Data
		Data += Write2Bytes(IR)

		"""
		if random.choice([0,1]) == 0:
			Data += Write2Bytes("1011110011111" + Rand5Bits(31)) + Write2Bytes("1100100001" + Rand5Bits(0) + "000") + Write2Bytes(IR)
		else:
			Data += Write2Bytes("1101110111" + Rand5Bits(-1) + "000") + Write2Bytes("1100100111" + Rand5Bits(0) + "000") + Write2Bytes(IR)
		if len(Data) % 8:
			Data += "0"*(8 - (len(Data) % 8))
		"""

		OutData = ""
		while(len(Data)):
			if int(Data[0:8], 2) < 0x20: #in [0, 0x0a, 0x09, 0x0d]:
				break

			OutData += chr(int(Data[0:8], 2))
			Data = Data[8:]

		if len(Data) >= 8:
			continue

		Count += 1

		OutData += "\x00"*(15 - (len(OutData) % 16))
		OutData += chr(Register)

		f.write(OutData)

		OutText = ""
		for curchar in xrange(0, len(OutData), 2):
			OutText += ("00" + hex(ord(OutData[curchar]))[2:])[-2:]
			OutText += ("00" + hex(ord(OutData[curchar + 1]))[2:])[-2:]
			OutText += " "

		print str(Offset) + " - " + OutText + "  " + OutData

		if Count >= 10:
			break
"""
for Offset in xrange(0, 8):
	Data = "0"*Offset + Write2Bytes(IR) + "1"

	OutData = ""
	if len(Data) % 8:
		Data += "1"*(8 - (len(Data) % 8))

	while(len(Data)):
		#if int(Data[0:8], 2) < 0x20: #in [0, 0x0a, 0x09, 0x0d]:
		#	break

		OutData += chr(int(Data[0:8], 2))
		Data = Data[8:]

	if len(Data):
		continue

	OutData += "\x00"*(15 - (len(OutData) % 16))
	OutData += chr(0xff)

	#f.write(OutData)
	OutText = ""
	for curchar in xrange(0, len(OutData), 2):
		OutText += ("00" + hex(ord(OutData[curchar]))[2:])[-2:]
		OutText += ("00" + hex(ord(OutData[curchar + 1]))[2:])[-2:]
		OutText += " "

	print OutText + " " + OutData
"""
f.close()
