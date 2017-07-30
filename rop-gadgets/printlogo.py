import sys, os
import random
import binascii

def ReplaceChars(Line):
	data = ""
	for CurChar in Line:
		if ord(CurChar) not in [0x20, 0x0a]:
			if random.randint(0, 10) == 10:
				data += chr(random.randint(0x8f, 0xff))
			else:
				data += chr(random.randint(0x21, 0x7f))
		else:
			data += CurChar

	return data

def GetBits(Data):
	Bits = ""
	for i in xrange(0, len(Data)):
		Bits += ("0"*8 + bin(ord(Data[i]))[2:])[-8:]
	return Bits

def GetBytes(Data):
	Bytes = ""

	if len(Data) % 8:
		Data += "0"*(8-(len(Data) % 8))
	for i in xrange(0, len(Data), 8):
		Bytes += chr(int(Data[i:i+8], 2))
	return Bytes

Logo = """
                                                         `hNNmso//.
                                                         oMMMMMMMMMh:
                                                        +MMMMMMMMMMMMd.
                                                       /MMMMMMMMMMMMMh
                                                      .mMMMMMMMMMMMMm.
                       .os+//-`                       hMMMMMMMMMMMMm.
                       yMMMMMNNmy+.                  +MMMMMMMMMMMMm.
                      -NMMMMMMMMMMNs                .NMMMMMMMMMMMm.
                      yMMMMMMMMMMMMh                hMMMMMMMMMMMN-
                     -NMMMMMMMMMMMM-               /MMMMMMMMMMMN:
                     hMMMMMMMMMMMMd               /NMMMMMMMMMMh:
                    /MMMMMMMMMMMMy`            ``oNMMMMMMMMMMMysysyhdhhdhyhhdhyo:+:
                   .mMMMMMMMMMMMN:..::/+ososhhdmmMMMMMMMMMMMMMMMMMMMMMMMMMMNmmh+
                  `yMMMMMMMMMMMMMNmNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMNmmdhoo+...`
          .:/+osyhmMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMdsso/-..``
         +mMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMN/:-`
         sMMMMMMMMMMMMMMMMMMMMMMMMMMNmmdhhyo+yMMMMMMMMMMN/
         .dMMMMMMMMMMMMMMMMMdyys+/:--..`    .mMMMMMMMMMM+              `` `````
          `omNmmMMMMMMMMMMMy`              `yMMMMMMMMMMm:/osssssyhhhshhddhyy+:/`
            `-.-MMMMMMMMMMm.```..::/+o+yyyhmMMMMMMMMMMMMMMMMMMMMMMNNmddo/-.`
               +MMMMMMMMMMMddmmmNMMMMMMMMMMMMMMMMMMMMMMMMNmmmhsss+-.```
    ``.-/+osshhmMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMds++.```
   sdmNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMNNMMMMMMMMMMMy`       ```       /oy:
  -MMMMMMMMMMMMMMMMMMMMMMMMNNmmddhso++//:dMMMMMMMMMMho+o+.  .-``       yms`
  `hMMMMMMMMMMMMMMMMMMMdyoo+//ooosyhhdhddMMMMMMMMMNd+-` `              `.
   `:ydmNNNmmMMMMMMMMMNmNNNNNNmmdyhss++:sMMMMMMMMm-
  .ossyhdmmmNMMMMMMMmyso///-..`         sMMMMMMMMo
  -ooo+//+o/dMMMMMMm`                  .NMMMMMMMh`
           `NMMMMMN:                   /MMMMMMMM:
           /MMMMNs.                   .mMMMMMMMm`
           hMMMN/                     sMMMMMMMm-
           mMMh:                     `mMMMMMMM:
          `NNs`                      oMMMMMMMo`
           .                        .NMMMMMm/
                                    sMMMMMh`
                                   .NmNdy-
"""

LogoLines = Logo.split("\n")
LogoLineLen = len(LogoLines[1])
Logo = ""
for i in xrange(0, len(LogoLines)):
	if not len(LogoLines[i]):
		continue

	LogoLines[i] = " "*10 + LogoLines[i]
	Logo += LogoLines[i].rstrip() + "\n"

#now cycle through opcodes and insert them randomly

LogoBits = GetBits(ReplaceChars(Logo))

Rops = """
7 - ff40 2980 4677 3c4e 4274 b220 50
7 - ff42 2d88 6251 6d66 b641 7b20 50
7 - ff44 3590 4473 e333 5841 7b20 50
7 - ff46 2d98 42b7 2623 c699 8120 50
7 - ff48 35a0 58f1 6534 3a60 2a20 50
7 - ff4a 35a8 7453 5824 7ee7 5420 50
7 - ff4c 35b0 4c52 f677 722a 8e20 50
7 - ff4e 2db8 7172 cea1 4847 4b20 50
7 - ff50 35c0 7252 5b5f 2230 7620 50
7 - ff52 35c8 5772 5722 4843 6b20 50
7 - ff54 2582 ce97 2d3b 4236 5d20 50
7 - ff56 35d8 73b7 3243 6657 3220 50
7 - ff58 31e0 4cf3 8e21 523a 3520 50
7 - ff5a 31e8 4f24 3658 4e4a e120 50
7 - ff5c 2df0 5b37 2429 4672 df20 50
7 - ff5e 2582 d893 6d41 2e5b 2720 50
7 - ff61 3580 5c37 2031 5a54 4a20 50
7 - ff63 3188 6062 2858 2441 b320 50
7 - ff64 2582 c872 2526 722d f520 50
7 - ff67 2998 7a97 32c3 9a70 6220 50
7 - ff69 35a0 5aa2 25da 6220 9020 50
7 - ff29 35a0 61b2 2a40 5843 3320 50
7 - ff6d 31b0 6072 53a2 726b ad20 50
7 - ff2d 35b0 5d31 6154 4c42 4320 50
7 - ff70 2182 c777 2925 3265 e620 50
7 - ff73 31c8 6632 7732 7235 4820 50
7 - ff33 35c8 5077 2a31 4c86 6b20 50
7 - ff35 35d0 6c73 49a0 5237 2720 50
7 - ff37 2dd8 4057 2a2a 6c42 4320 50
"""

Rops = Rops.replace(" ", "")
RopLines = Rops.split("\n")
RopLines.pop(0)
RopLines.pop()

#create a bit list with just the bits we require
for i in xrange(0, len(RopLines)):
	RopLines[i] = RopLines[i].split("-")
	RopLines[i][0] = int(RopLines[i][0])
	RopLines[i][1] = GetBits(binascii.a2b_hex(RopLines[i][1]))[RopLines[i][0]:] + "00"

#now start walking through to see where we can insert
#random.shuffle(RopLines)

NewRopLines = []
for i in [10, 23, 25, 01, 9, 28, 18, 14, 19, 6, 17, 26, 24, 7, 21, 2, 11, 20, 0, 22, 8, 27, 12, 15, 4, 3, 13, 16, 5]:
	NewRopLines.append(RopLines[i])
RopLines = NewRopLines

CurRopLine = RopLines.pop(0)
i = 0
while i < len(LogoBits):
	#if no space or newline in the block of bits then replace
	Offset = i + (CurRopLine[0]*8) + CurRopLine[0]
	#if (LogoBits[Offset:Offset+len(CurRopLine[1])].find("00100000") == -1) and (LogoBits[Offset:Offset+len(CurRopLine[1])].find("00001010") == -1):
	ByteCount = (len(CurRopLine[1]) / 8) + 2
	CheckData = Logo[Offset / 8: (Offset / 8) + ByteCount]
	if (CheckData.find(" ") == -1) and (CheckData.find("\n") == -1):
		#print "Replacing at offset %x, 9 bit offset %x" % (Offset / 8, Offset / 9)
		print "u %x" % (Offset / 9)
		LogoBits = LogoBits[0:Offset] + CurRopLine[1] + LogoBits[Offset+len(CurRopLine[1]):]

		if len(RopLines) == 0:
			print "All Done"
			break

		CurRopLine = RopLines.pop(0)

		Offset = Offset + len(CurRopLine[1])
		i = Offset + ((8*9) - (Offset % (8*9)))
	else:
		i += (8*9)

if len(RopLines) == 0:
	NewLogo = GetBytes(LogoBits)
	print NewLogo

	open("logo.nfo.data","w").write(NewLogo)
	open("clemency.nfo","w").write(NewLogo + "\n" + open("clemency.nfo.data","r").read())
else:
	print "%d left" % (len(RopLines))
