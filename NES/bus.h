#pragma once
#include <string>
#include "defs.h" 

class c6502;
class c6502Dbg;
struct c6502_Instruction;

class cDataBus {
	u8 buf;
	c6502Dbg *debugger = nullptr;

public:
	cDataBus(u16 org = 0);
	~cDataBus();

	u8 read(u16);
	u8 write(u16, u8);

	void attach_dbg();
	void debug(c6502_Instruction, u8);

	void load(std::string filename);

	// devices
	c6502 *cpu;
	u8 mem[0xffff]; //for now, just pretend the the whole addr space is memory
};