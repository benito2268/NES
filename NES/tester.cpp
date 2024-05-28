#include <iostream>

#include "tester.h"
#include "bitops.h"
#include "6502.h"
#include "bus.h"

bool Tester::run_tests() {
	bus = new cDataBus();

	bool b =
		test("test_imm", &Tester::test_imm) &&
		test("test_zp", &Tester::test_zp) &&
		test("test_abs", &Tester::test_abs) &&
		test("test_rel", &Tester::test_rel) &&
		test("test_ind", &Tester::test_ind) &&
		test("test_adc", &Tester::test_adc) &&
		test("test_sbc", &Tester::test_sbc);

	return b;
}

bool Tester::test(std::string name, bool (Tester::*fun)()) {
	bool b = (this->*fun)();
	std::cout << name << " " << RESULTS[b] << std::endl;
	return b;
}

//addressing modes
bool Tester::test_imm() {
	u16 org = 0x3000;
	bus->cpu->pc = org;

	//LDA $10
	bus->write(org, 0xA9);
	bus->write(org + 1, 0x0A);

	//advance past the opcode
	bus->cpu->pc++;

	return (
		bus->cpu->addr_imm() == 0x000A
		&& bus->cpu->addr_is_imm
	);
}

bool Tester::test_zp() {
	u16 org = 0x3000;
	bus->cpu->pc = org;

	//INC $55
	bus->write(org, 0xE6);
	bus->write(org + 1, 0x55);

	//advance past the opcode
	bus->cpu->pc++;

	return bus->cpu->addr_zp() == 0x0055;
}

bool Tester::test_abs() {
	u16 org = 0x3000;
	bus->cpu->pc = org;

	//INC $55
	bus->write(org, 0x4C);
	bus->write(org + 1, 0x34);
	bus->write(org + 2, 0x12);

	//advance past the opcode
	bus->cpu->pc++;

	u16 ret = bus->cpu->addr_abs() == 0x1234;
	return ret;
}

bool Tester::test_rel() {
	u16 org = 0x3000;
	bus->cpu->pc = org;

	bus->write(org, 0xF0);
	bus->write(org + 1, 0xFE);

	bus->cpu->pc++;

	return bus->cpu->addr_rel() == 0x3000;
}

bool Tester::test_ind() {
	//test normal
	u16 org = 0x3000;
	bus->cpu->pc = org;

	bus->write(org, 0x6C);
	bus->write(org + 1, 0x64);
	bus->write(org + 2, 0x40);

	bus->cpu->pc++;

	bus->write(0x4064, 0xFC);
	bus->write(0x4065, 0xFF);

	bool b1 = bus->cpu->addr_ind() == 0xFFFC;

	//test bug cond.
	bus->cpu->pc = org;

	//clear mem to be safe
	for(u16 i = 0; i < 0xFFFF; i++)
		bus->write(i, 0);

	bus->write(org, 0x6C);
	bus->write(org + 1, 0xFF);
	bus->write(org + 2, 0x40);

	bus->write(0x40FF, 0x60);
	bus->write(0x4000, 0xDD);
	bus->write(0x4100, 0xCC);

	bus->cpu->pc++;

	//should NOT be 0xCC60
	bool b2 = bus->cpu->addr_ind() == 0xDD60;

	return b1 && b2;
}

bool Tester::test_adc() {
	//normal addition
	bool b1 = false;

	bus->write(0x4250, 2);
	bus->cpu->eff_addr = 0x4250;
	bus->cpu->addr_is_imm = false;
	bus->cpu->a = 2;

	bus->cpu->adc();

	b1 = (bus->cpu->a == 4) &&
		!(BIT_CHK(bus->cpu->ps, CF)) &&
		!(BIT_CHK(bus->cpu->ps, ZF)) &&
		!(BIT_CHK(bus->cpu->ps, OF));

	//multibyte addition
	bool b2 = false;
	BIT_CLR(bus->cpu->ps, CF);

	bus->write(0x4250, 0x01);
	bus->cpu->eff_addr = 0x4250;
	bus->cpu->addr_is_imm = false;
	bus->cpu->a = 0xFF;

	bus->cpu->adc();

	b2 = (bus->cpu->a == 0x00) &&
		 (BIT_CHK(bus->cpu->ps, CF)) &&
		 (BIT_CHK(bus->cpu->ps, ZF)) &&
		 !(BIT_CHK(bus->cpu->ps, OF));

	//overflow
	bool b3 = false;
	BIT_CLR(bus->cpu->ps, CF);

	bus->write(0x4250, 0x7F);
	bus->cpu->eff_addr = 0x4250;
	bus->cpu->addr_is_imm = false;
	bus->cpu->a = 0x01;

	bus->cpu->adc();

	b3 = (bus->cpu->a == 0x80) &&
		!(BIT_CHK(bus->cpu->ps, CF)) &&
		!(BIT_CHK(bus->cpu->ps, ZF)) &&
		(BIT_CHK(bus->cpu->ps, OF));

	return b1 && b2 && b3;
}

bool Tester::test_sbc() {
	//normal subtraction
	bool b1 = false;

	//multibyte subtraction
	bool b2 = false;


	return b1 && b2;
}