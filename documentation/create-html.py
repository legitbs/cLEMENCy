import sys, os
import sqlite3

def DoHTMLHeader():
	return """
	<html>
	<style>
	table.linetable {
		border-bottom: 1px solid black;
	}

	table.subtable {
		padding: 2px;
		text-align: left;
		border-spacing: 0px;
	}

	table.subtable tr:nth-child(even) td {
		padding: 5px;
		background-color: #dddddd;
		padding-right: 10px;
		vertical-align: top;
	}

	table.subtable tr:nth-child(odd) td {
		padding: 5px;
		background-color: #eeeeee;
		padding-right: 10px;
		vertical-align: top;
	}

	table.numbers {
		padding: 0px;
		border-spacing: 0px;
		width: 100%;
	}

	table.bittable {
		padding: 0px;
		border-spacing: 0px;
	}

	td.flagclass {
		text-align: center;
		width: 10px;
	}

	td.bittext {
		border-bottom: 1px solid black;
	}

	td.pipeclass {
		border-right: 1px solid black;
	}

	td.instlayout {
		border: 1px solid black;
		text-align: center;
	}

	p.instpagebreak {
		padding-top: 10px;
	}

	font.header1 {
		font-size: 200%;
		font-weight: bold;
		padding-top: 50px;
	}

	font.header2 {
		font-size: 125%;
		font-weight: bold;
	}

	a:link.toc {
		text-decoration: none;
		font-size: 125%;
	}

	font.subtoc {
		font-size: 100%;
		padding-left: 20px;
	}

	font.subtoc2 {
		font-size: 90%;
		padding-left: 20px;
	}

	</style>
	<body>

	<center>
	<table width=60% border=0 cellspacing=0>
	"""

def DoInstructionDetails():
	outdata = "<tr><td>\n"
	TOCList = []

	details = open("instruction details.txt","r").read().split("\n")

	InstDetail = dict()
	CurInst = ""

	DetailEntries = ["format", "purpose", "description", "operation", "flags"]
	DetailEntryID = -1

	CreatingTable = False

	for line in details:
		line = line.replace("<", "&lt;").replace(">", "&gt;")

		if (len(line) == 0) or (line[0] == "#"):
			if(CreatingTable):
				InstDetail[CurInst][DetailEntries[DetailEntryID]] += "</table>\n"
			CreatingTable = False
			continue

		elif line[0] == "!":
			if(CreatingTable):
				InstDetail[CurInst][DetailEntries[DetailEntryID]] += "</table>\n"
			CreatingTable = False
			CurInst = line[1:].upper()
			DetailEntryID = -1
			if CurInst in InstDetail:
				print "Found %s in detail list already" % (CurInst)
				sys.exit(0)
			InstDetail[CurInst] = dict()

		elif line[0] == '-':
			if(CreatingTable):
				InstDetail[CurInst][DetailEntries[DetailEntryID]] += "</table>\n"
			CreatingTable = False
			DetailEntryID += 1

			if (DetailEntries[DetailEntryID] == "flags"):
				if line[1:].upper().strip() == "A":
					InstDetail[CurInst][DetailEntries[DetailEntryID]] = "Z C O S"
				elif line[1:].upper().strip() == "B":
					InstDetail[CurInst][DetailEntries[DetailEntryID]] = "Z S"
				else:
					InstDetail[CurInst][DetailEntries[DetailEntryID]] = " ".join(iter(line[1:].strip().replace(" ","")))	#make sure a space between each flag
			else:
				InstDetail[CurInst][DetailEntries[DetailEntryID]] = line[1:]

		elif len(line):
			if line[0:2] == "  ":
				line = line.replace("  ", "&nbsp;&nbsp;&nbsp;&nbsp;")

			if ("-" in line) and (line.split(" ")[1] == '-'):
				if CreatingTable == False:
					InstDetail[CurInst][DetailEntries[DetailEntryID]] += "\n<table border=0 class=\"subtable\">\n"
					CreatingTable = True
				linedata = line.split("-")
				line = "<tr>"
				for l in linedata:
					line += "<td>%s</td>" % (l.strip())
				line += "</tr>\n"
				InstDetail[CurInst][DetailEntries[DetailEntryID]] += line
			else:
				InstDetail[CurInst][DetailEntries[DetailEntryID]] += "<br>\n" + line

	db = sqlite3.connect("instructions.db")

	InstructionTemplate = """
	<p class="instpagebreak" />
	<table width=100%% border=0 cellspacing=10>
	<tr><td>

	<div id=%s></div>
	<table width=100%% border=0 class="linetable">
	<tr>
	<td width=10%%><font class="header2">%s</font></td><td><font class="header2">%s</font></td>
	</tr>
	</table>

	<br>

	<table width=100%% border=0 cellpadding=0 cellspacing=0>
	%s
	</table>

	</td>
	</tr>

	<tr><td>Format: %s</td></tr>
	<tr><td>Purpose: %s</td></tr>
	<tr><td>Description: %s</td></tr>
	<tr><td>Operation: %s</td></tr>
	<tr><td>Flags affected: %s</td></tr>
	</table>
	"""

	cur = db.cursor()
	cur.execute("select id, numbits, fieldorder, fieldname from instruction_format order by id, fieldorder")
	InstFormatDB = cur.fetchall()

	InstFormat = dict()
	InstBits = []

	for (InstID, NumBits, FieldOrder, FieldName) in InstFormatDB:
		if InstID not in InstFormat:
			InstBits = []
			InstFormat[InstID] = dict()
			InstFormat[InstID]["Fields"] = []
			InstFormat[InstID]["TotalBits"] = 0

		InstFormat[InstID]["Fields"].append((NumBits, FieldName))
		InstFormat[InstID]["TotalBits"] += NumBits

	cur = db.cursor()
	cur.execute("select id, name, instruction_format, description from instructions order by name")
	Instructions = cur.fetchall()

	print "Found %d instructions" % (len(Instructions))

	for (InstID, InstName, InstFormatID, InstDescription) in Instructions:
		CurInstruction = dict()
		CurInstruction["Instruction"] = InstName
		if "-" in InstDescription:
			InstDescription = InstDescription[0:InstDescription.find("-") - 1].strip()
		CurInstruction["Instruction_Description"] = InstDescription

		cur = db.cursor()
		cur.execute("select id, fieldentry, value from instruction_detail where id=%d" % (int(InstID)))
		InstDetailDB = cur.fetchall()

		CurInstruction["Layout"] = dict()
		for (InstDetailID, InstDetailFieldEntry, InstDetailValue) in InstDetailDB:
			CurInstruction["Layout"][int(InstDetailFieldEntry)] = int(InstDetailValue)

		#BitNum = InstFormat[InstFormatID]["TotalBits"] - 1
		BitNum = 0
		InstBits = []
		InstData = []
		InstBits.append("")
		InstData.append("")
		BitCombine = 0
		DataCombine = ""
		for i in xrange(0, len(InstFormat[InstFormatID]["Fields"])):
			(NumBits, FieldName) = InstFormat[InstFormatID]["Fields"][i]

			#if 54 bits and we are at a 24 bit multiple then add a new line
			#if (InstFormat[InstFormatID]["TotalBits"] == 54) and (BitNum == 27):
			#	InstBits.append("")
			#	InstData.append("")

			#if current and next are things that can be combined then just add the current one
			if (i in CurInstruction["Layout"]) and ((i+1) in CurInstruction["Layout"]):
				BitCombine += NumBits
				DataCombine += ("0"*NumBits + bin(CurInstruction["Layout"][i])[2:])[-NumBits:]

			#if we have just 1 bit and nothing to combine output just the 1 bit
			elif (NumBits == 1) and (BitCombine == 0):
				InstBits[-1] += "<td width=4%% align=right>%d</td>\n" % (BitNum)
				#BitNum -= NumBits
				BitNum += NumBits
				InstData[-1] += "<td class=\"instlayout\">"
				if i in CurInstruction["Layout"]:
					InstData[-1] += ("0"*NumBits + bin(CurInstruction["Layout"][i])[2:])[-NumBits:]
				else:
					InstData[-1] += FieldName
				InstData[-1] += "</td>"

			#if the current entry is in the layout then add anything that was combinable
			elif (i in CurInstruction["Layout"]):
				DataCombine += ("0"*NumBits + bin(CurInstruction["Layout"][i])[2:])[-NumBits:]
				InstData[-1] += "<td class=\"instlayout\">" + DataCombine + "</td>"
				DataCombine = ""

				NumBits += BitCombine
				InstBits[-1] += "<td width=%d%%><table border=0 class=\"numbers\"><tr><td align=left>%d</td><td align=right>%d</td></tr></table></td>\n" % (4*NumBits, BitNum, BitNum + NumBits - 1)
				#BitNum -= NumBits
				BitNum += NumBits
				BitCombine = 0

			#current entry has no value, just output it
			else:
				InstBits[-1] += "<td width=%d%%><table border=0 class=\"numbers\"><tr><td align=left>%d</td><td align=right>%d</td></tr></table></td>\n" % (4*NumBits, BitNum, BitNum + NumBits - 1)
				#BitNum -= NumBits
				BitNum += NumBits
				InstData[-1] += "<td class=\"instlayout\">" + FieldName + "</td>"

		if InstName.upper() not in InstDetail:
			print "Unable to locate %s" % (InstName.upper())
			continue

		#now combine all of the grouped bits for fields
		InstFields = ""
		for i in xrange(0, len(InstBits)):
			InstFields += "<tr>\n" + InstBits[i] + "\n</tr><tr>\n" + InstData[i] + "</tr>\n"

		if "\n" in InstDetail[InstName.upper()]["operation"]:
			InstDetail[InstName.upper()]["operation"] = "<br>\n" + InstDetail[InstName.upper()]["operation"]

		outdata += InstructionTemplate % (InstName.upper(),
						InstName.upper(), InstDescription.upper(),
						InstFields,
						InstDetail[InstName.upper()]["format"],
						InstDetail[InstName.upper()]["purpose"],
						InstDetail[InstName.upper()]["description"],
						InstDetail[InstName.upper()]["operation"],
						InstDetail[InstName.upper()]["flags"])
		TOCList.append([InstName.upper(), InstDescription.upper()])

	outdata += "</td></tr>"
	return (outdata, TOCList)

def DoHTMLFooter():
	return """
	</table>
	</center>
	</body>
	</html>
	"""

def DoChapters():
	data = open("chapters.txt","r").read().split("\n")

	outdata = "<tr><td>\n"
	TOCList = []

	ChapterData = ""
	CurChapter = 0
	CurSubChapter = 0
	CreatingTable = False
	CreatingLineTable = False
	for line in data:
		if len(line) and line[0] == '-':
			if(CreatingTable):
				CreatingTable = False
				ChapterData += "</table>"

			#new chapter or sub chapter, see how many dashes
			DashCount = 0
			while(line[DashCount] == '-'):
				DashCount+=1

			#if 1 dash then write out the previous chapter and start a new one
			if DashCount == 1:
				if len(ChapterData):
					outdata += ChapterData
					ChapterData = ""
				TOCList.append([])
				CurChapter += 1
				CurSubChapter = 0
			else:
				CurSubChapter += 1

			SubChapterID = ""
			if(CurSubChapter):
				SubChapterID = chr(0x61 + CurSubChapter)
			ChapterText = "chapter%d%s" % (CurChapter, SubChapterID)
			TOCList[-1].append([ChapterText, line[DashCount:-DashCount]])

			ChapterData += "<div id=%s></div>\n<font class=\"header%d\">%s</font><hr>\n" % (ChapterText, DashCount, line[DashCount:-DashCount])
		else:
			if (" - " in line):
				if CreatingTable == False:
					ChapterData += "\n<table border=0 class=\"subtable\">\n"
					CreatingTable = True
				linedata = line.split(" - ")
				line = "<tr>"
				for l in linedata:
					line += "<td>%s</td>" % (l.strip())
				line += "</tr>\n"
				ChapterData += line
			elif len(line) and (line[0] == "+"):
				if CreatingLineTable:
					CreatingLineTable = False
					ChapterData += "<td colspan=%d class=pipeclass></td><td class=bittext></td><td colspan=%d class=bittext>%s</td>" % (LastLineOffset, MaxLineColumns - LastLineOffset + 1, NextLineData)
					ChapterData += "</table>\n"
				else:
					CreatingLineTable = True
					ChapterData += "<table border=0 class=\"bittable\">\n"
					NextLineData = ""
					MaxLineColumns = 0
					LastLineOffset = 0

			elif CreatingLineTable:
				#custom format here
				newline = "<tr>"

				#two cells per entry so lines are in the middle
				PipeOffset = line.find("|")
				SlashOffset = line.find("\\")
				LastLineOffset = SlashOffset
				if (PipeOffset == -1) and (SlashOffset == -1):
					for i in xrange(0, len(line)):
						if ((i+1) != len(line)) and (line[i+1] == "0"):
							newline += "<td class=flagclass>" + line[i] + "</td>"
						else:
							newline += "<td class=pipeclass>" + line[i] + "</td>"
						MaxLineColumns += 1
					#add an entry at the end to expand it for the first block of text
					newline += "<td class=bittext></td>"
				else:
					#figure out how many columns to skip from spaces
					if PipeOffset != -1:
						newline += "<td colspan=%d class=\"pipeclass\"></td>" % (PipeOffset * 1)
					else:
						newline += "<td colspan=%d class=\"pipeclass\"></td>" % (SlashOffset * 1)

					#continue adding in cells while the pipe can be found
					while(PipeOffset != -1):
						newline += "<td class=\"pipeclass\"></td>"
						PipeOffset = line.find("|", PipeOffset + 1)

					#account for the slash
					newline += "<td class=\"pipeclass\"></td>"

					if len(NextLineData):
						newline += "<td class=bittext></td><td colspan=%d class=bittext>%s</td>" % (MaxLineColumns - SlashOffset, NextLineData)

					#store what the next block will contain
					NextLineData = line[SlashOffset+1:]

				ChapterData += newline + "</tr>\n"
			else:
				if CreatingTable:
					CreatingTable = False
					ChapterData += "</table>"
				ChapterData += line + "<br>\n"

			#ChapterData += line + "<br>\n"

	if len(ChapterData):
		outdata += ChapterData

	outdata += "</td></tr>\n"

	return (outdata, TOCList)

def DoTOC(ChapterTOC, InstTOC):
	outdata = "<tr><td><font class=header1>Table of Contents</font><br>\n"

	for Entry in ChapterTOC:
		outdata += "<a href=#%s class=\"toc\">%s</a><br>" % (Entry[0][0], Entry[0][1])

		if len(Entry) != 1:
			for i in xrange(1, len(Entry)):
				outdata += "<font class=\"subtoc\"><a href=#%s class=\"toc\">%s</a></font><br>" % (Entry[i][0], Entry[i][1])

	for Entry in InstTOC:
		outdata += "<font class=\"subtoc2\"><a href=#%s class=\"toc\">%s - %s</a></font><br>" % (Entry[0], Entry[0], Entry[1])

	outdata += "</td></tr>"
	return outdata

HTML = DoHTMLHeader()
(Chapters, TOCEntries) = DoChapters()
(InstDetails, TOCInstEntries) = DoInstructionDetails()
Footer = DoHTMLFooter()
TOC = DoTOC(TOCEntries, TOCInstEntries)

f = open("instructions.html","w")
f.write(HTML + TOC + Chapters + InstDetails + Footer)
f.close()
