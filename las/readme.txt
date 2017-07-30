las documentation

The format of the instructions is already specified in the instructions.html file. Here are extra options to help during code creation.

Labels end with a colon unless followed by a db, dw, or dt
Any data after a ; is considered a comment
Parameters required a comma then space between them
Numbers can be decimal or start with 0x to indicate hex
Memory protections can use N, R, RW, or E as alternatives to the numeric version
Any label reference by an instruction that allows an immediate but is not a conditional branch or a relative instruction is treated as an absolute label reference. To make a label reference relative add @ to the beginning of the label name

Load and store instructions take the number of registers and automatically subtracts 1 from them if provided.

section XXXX yy	- All commands following will end up in section XXXX, yy is optional and specifies A (alloc), W (write), or X (execute) flags to add to the section. Default is AW
extrn XXXX	- The specified name can be seen external to the assembly
db		- define byte, can be hex values starting with 0x, strings, or numbers below 256. Any combination can be used as long as they are comma seperated
dw		- same as .db but for word (2 byte) setups. Each entry will be a minimum of 2 bytes, strings will not be 2 byte aligned though
dt		- same as .db but for tri-byte (3 byte) setups. Same rules as .dw

%define		- define a macro, current code stores the value but does not replace it if used at the moment
%ifdef		- see if a macro is defined
%ifndef		- see if a macro is not defined
%else		- alternative in %ifdef/%ifndef block
%endif		- end of %ifdef/%ifndef block
