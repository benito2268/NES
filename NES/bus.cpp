#include "bus.h"
#include "dbg.h"
#include "6502.h"

#include <algorithm>
#include <iostream>
#include <fstream>

cDataBus::cDataBus(u16 org) {
	//set up devices
	this->cpu = new c6502(this);
	this->cpu->set_org(org);
}

cDataBus::~cDataBus() {
	//free devices
	delete cpu;

	if(debugger != nullptr)
		delete debugger;
}

u8 cDataBus::read(u16 addr) {
	buf = mem[addr];
	return buf;
}

u8 cDataBus::write(u16 addr, u8 data) {
	mem[addr] = data;
	return data;
}

void cDataBus::attach_dbg() {
	debugger = new c6502Dbg(1280, 720, cpu, this);
}

void cDataBus::debug(c6502_Instruction inst, u8 opcode) {
	if(debugger != nullptr) {
		if(!debugger->debug(inst, opcode)) {
			std::exit(0);
		}
	}
}

void cDataBus::load(std::string filename) {
	//move the first 0xFFFF bytes from filename into memory
	std::ifstream file(filename, std::ios::binary);

	if(file.fail()) {
		std::cerr << "could not open object file: " << filename;
		std::exit(1);
	}

	std::size_t bytes;
	file.seekg(0, std::ios::end);
	bytes = file.tellg();
	file.seekg(0, std::ios::beg);

	file.read((char*)mem, bytes);
}