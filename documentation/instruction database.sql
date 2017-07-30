drop table instruction_format;
drop table instructions;
drop table instruction_detail;

BEGIN TRANSACTION;
create table instruction_format(id integer, NumBits integer, FieldOrder integer, FieldName text);
create table instructions(id integer primary key, name text, instruction_format integer, description text);
create table instruction_detail(id integer, FieldEntry integer, Value Integer);

/* registerless (branch return / interrupt return, etc) - 2 bytes */
insert into instruction_format values(0, 5, 0, "Opcode");
insert into instruction_format values(0, 2, 1, "Sub Opcode");
insert into instruction_format values(0, 5, 2, "Sub Opcode 2");
insert into instruction_format values(0, 6, 3, "Reserved");

/*
opcode / MultiReg Flag / FloatingPoint Flag / rA / rB / rc
#2 bits left
*/
insert into instruction_format values(1, 5, 0, "Opcode");
insert into instruction_format values(1, 1, 1, "M");	/* multi reg */
insert into instruction_format values(1, 1, 2, "F");	/* floating point */
insert into instruction_format values(1, 5, 3, "rA");
insert into instruction_format values(1, 5, 4, "rB");
insert into instruction_format values(1, 5, 5, "rC");
insert into instruction_format values(1, 2, 6, "Reserved");
insert into instruction_format values(1, 1, 7, "Signed");
insert into instruction_format values(1, 1, 8, "Use Immediate");	/*see #15 */
insert into instruction_format values(1, 1, 9, "UF");

/* opcode / rA / rB / Immediate - shift/rotates */
insert into instruction_format values(2, 5, 0, "Opcode");
insert into instruction_format values(2, 1, 1, "M");	/* multi reg */
insert into instruction_format values(2, 1, 2, "D");	/* direction */
insert into instruction_format values(2, 5, 3, "rA");
insert into instruction_format values(2, 5, 4, "rB");
insert into instruction_format values(2, 7, 5, "Immediate");
insert into instruction_format values(2, 2, 6, "Reserved");
insert into instruction_format values(2, 1, 7, "UF");

/* opcode / load/store, rA->rX = [rB->rX], [rB-rX] = rA->rX - 6 bytes*/
insert into instruction_format values(3, 5, 0, "Opcode");
insert into instruction_format values(3, 2, 1, "Sub Opcode");
insert into instruction_format values(3, 5, 2, "rA");
insert into instruction_format values(3, 5, 3, "rB");
insert into instruction_format values(3, 5, 4, "Register Count");
insert into instruction_format values(3, 2, 5, "Adjust rB"); /*0 - no adjust, 1 - add size to rB after read/write, 2 - subtract size from rB before read/write, 3 - invalid */
insert into instruction_format values(3, 27, 6, "Memory Offset");
insert into instruction_format values(3, 3, 7, "Reserved");

/* opcode / rA */
insert into instruction_format values(4, 5, 0, "Opcode");
insert into instruction_format values(4, 2, 1, "Sub Opcode");
insert into instruction_format values(4, 1, 2, "M");	/* multi reg */
insert into instruction_format values(4, 1, 3, "F");	/* floating point */
insert into instruction_format values(4, 5, 4, "rA");
insert into instruction_format values(4, 5, 5, "rB");
insert into instruction_format values(4, 2, 6, "Sub Opcode 2");
insert into instruction_format values(4, 5, 7, "Reserved");
insert into instruction_format values(4, 1, 8, "UF");

/* opcode / MultiReg / FloatingPoint / rA / rB - 2 bytes */
insert into instruction_format values(5, 5, 0, "Opcode");
insert into instruction_format values(5, 1, 1, "M");	/* multi reg */
insert into instruction_format values(5, 1, 2, "F");	/* floating point */
insert into instruction_format values(5, 1, 3, "Use Immediate");
insert into instruction_format values(5, 5, 4, "rA");
insert into instruction_format values(5, 5, 5, "rB");

/* branch relative conditional */
insert into instruction_format values(6, 5, 0, "Opcode");
insert into instruction_format values(6, 1, 1, "Call");
insert into instruction_format values(6, 4, 2, "Condition");
insert into instruction_format values(6, 17, 3, "Offset");

/* branch / Call  Relative - 4 bytes */
insert into instruction_format values(7, 5, 0, "Opcode");
insert into instruction_format values(7, 2, 1, "Sub Opcode");
insert into instruction_format values(7, 2, 2, "Reserved");
insert into instruction_format values(7, 27, 3, "Offset");

/* opcode / rA / Immediate */
insert into instruction_format values(8, 5, 0, "Opcode");
insert into instruction_format values(8, 5, 1, "rA");
insert into instruction_format values(8, 17, 2, "Immediate");

/* branch register conditional - 2 bytes */
insert into instruction_format values(9, 5, 0, "Opcode");
insert into instruction_format values(9, 1, 1, "Call");
insert into instruction_format values(9, 4, 2, "Condition");
insert into instruction_format values(9, 5, 3, "rA");
insert into instruction_format values(9, 3, 4, "Reserved");

/* Memory Protection and Hash rA = memory location, rB = number of pages*/
insert into instruction_format values(10, 5, 0, "Opcode");
insert into instruction_format values(10, 2, 1, "Sub Opcode");
insert into instruction_format values(10, 5, 2, "rA");
insert into instruction_format values(10, 5, 3, "rB");
insert into instruction_format values(10, 1, 4, "Write");   /* read or write flag */
insert into instruction_format values(10, 2, 5, "Memory Flags");
insert into instruction_format values(10, 7, 6, "Reserved");

/* opcode / MultiReg / FloatingPoint / rA / rB */
insert into instruction_format values(11, 5, 0, "Opcode");
insert into instruction_format values(11, 1, 1, "M");	/* multi reg */
insert into instruction_format values(11, 1, 2, "F");	/* floating point */
insert into instruction_format values(11, 1, 3, "Use Immediate");
insert into instruction_format values(11, 5, 4, "rA");
insert into instruction_format values(11, 14, 5, "Immediate");

/* conversion from int to float and float to int */
insert into instruction_format values(12, 5, 0, "Opcode");
insert into instruction_format values(12, 2, 1, "Sub Opcode");
insert into instruction_format values(12, 1, 2, "M");   /* multi reg */
insert into instruction_format values(12, 1, 3, "F");   /* float to int vs int to float */
insert into instruction_format values(12, 5, 4, "rA");
insert into instruction_format values(12, 5, 5, "rB");
insert into instruction_format values(12, 8, 6, "Reserved");

/* sign extend single/word */
insert into instruction_format values(13, 5, 0, "Opcode");
insert into instruction_format values(13, 2, 1, "Sub Opcode");
insert into instruction_format values(13, 5, 2, "Sub Opcode 2");
insert into instruction_format values(13, 5, 3, "rA");
insert into instruction_format values(13, 5, 4, "rB");
insert into instruction_format values(13, 5, 5, "Reserved");

/*
opcode / MultiReg Flag / FloatingPoint Flag / rA / rB / rc
#2 bits left
*/
insert into instruction_format values(14, 5, 0, "Opcode");
insert into instruction_format values(14, 1, 1, "M");	/* multi reg */
insert into instruction_format values(14, 1, 2, "D");	/* direction */
insert into instruction_format values(14, 5, 3, "rA");
insert into instruction_format values(14, 5, 4, "rB");
insert into instruction_format values(14, 5, 5, "rC");
insert into instruction_format values(14, 4, 6, "Reserved");
insert into instruction_format values(14, 1, 7, "UF");

/*
opcode / MultiReg Flag / FloatingPoint Flag / rA / rB / Immediate
*/
insert into instruction_format values(15, 5, 0, "Opcode");
insert into instruction_format values(15, 1, 1, "M");	/* multi reg */
insert into instruction_format values(15, 1, 2, "F");	/* floating point */
insert into instruction_format values(15, 5, 3, "rA");
insert into instruction_format values(15, 5, 4, "rB");
insert into instruction_format values(15, 7, 5, "Immediate");
insert into instruction_format values(15, 1, 6, "Signed");
insert into instruction_format values(15, 1, 7, "Use Immediate");
insert into instruction_format values(15, 1, 8, "UF");

/* enable/disable interrupt and flag read */
insert into instruction_format values(16, 5, 0, "Opcode");
insert into instruction_format values(16, 2, 1, "Sub Opcode");
insert into instruction_format values(16, 5, 2, "Sub Opcode 2");
insert into instruction_format values(16, 5, 3, "rA");
insert into instruction_format values(16, 1, 4, "Reserved");

/* branch / Call Absolute - 4 bytes */
insert into instruction_format values(17, 5, 0, "Opcode");
insert into instruction_format values(17, 2, 1, "Sub Opcode");
insert into instruction_format values(17, 2, 2, "Reserved");
insert into instruction_format values(17, 27, 3, "Location");
COMMIT;
