#pragma once
#include "defs.h" 

class c6502;

class cDataBus {
	u8 buf;

public:
	cDataBus();
	~cDataBus();

	u8 read(u16);
	u8 write(u16, u8);

	// devices
	c6502 *cpu;
	u8 mem[0xffff] = { 0 }; //for now, just pretend the the whole addr space is memory
};