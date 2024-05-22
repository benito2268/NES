#include "bus.h"
#include "6502.h"

cDataBus::cDataBus() {
	//set up devices
	this->cpu = new c6502(this);
}

cDataBus::~cDataBus() {
	//free devices
	delete this->cpu;
}

u8 cDataBus::read(u16 addr) {
	buf = mem[addr];
	return buf;
}

u8 cDataBus::write(u16 addr, u8 data) {
	mem[addr] = data;
	return data;
}