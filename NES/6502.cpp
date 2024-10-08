#include "6502.h"
#include "bus.h"
#include "bitops.h"

#include "dbg.h"

// CPU pin functions & internal helpers
// ************************************

c6502::c6502(cDataBus *bus) {

	//NOTE: the initial values of the registers (including the stack pointer!)
	//are not certain on a real 6502, for sanity I'll set them to zero
	a = 0x00;
	x = 0x00;
	y = 0x00;
	sp = 0x00; //this should really be set to 0xFF but it's probably best not to fiddle with what is the ROM's job

	//unused bit always 1
	ps = 0x00; 
	BIT_SET(this->ps, 5);

	this->bus = bus;

	//disable irqs
	BIT_SET(this->ps, ID);

	//load the PC with the contents of the reset vector
	pc = bus->read(0xFFFC);
	pc |= (bus->read(0xFFFD) << 8);

	//set up the instruction matrix
	//this is very messy - but it works okay
	//c6502_Instruction => {name, addrmode, op, cycles}
	using a = c6502;
	opcode_tbl =
	{
		{"brk", &a::addr_imp, &a::brk, 7}, {"ora", &a::addr_idx, &a::ora, 6}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"ora", &a::addr_zp, &a::ora, 3},  {"asl", &a::addr_zp, &a::asl, 5},  {"???", &a::addr_imp, &a::ill, 1}, {"php", &a::addr_imp, &a::php, 3}, {"ora", &a::addr_imm, &a::ora, 2},  {"asl", &a::addr_acc, &a::asl, 2}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"ora", &a::addr_abs, &a::ora, 4},  {"asl", &a::addr_abs, &a::asl, 7}, {"???", &a::addr_imp, &a::ill, 1},
		{"bpl", &a::addr_rel, &a::bpl, 2}, {"ora", &a::addr_idy, &a::ora, 5}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"ora", &a::addr_zpx, &a::ora, 4}, {"asl", &a::addr_zpx, &a::asl, 6}, {"???", &a::addr_imp, &a::ill, 1}, {"clc", &a::addr_imp, &a::ill, 2}, {"ora", &a::addr_aby, &a::ora, 4},  {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"ora", &a::addr_abx, &a::ora, 4},  {"asl", &a::addr_abx, &a::asl, 7}, {"???", &a::addr_imp, &a::ill, 1},
		{"jsr", &a::addr_abs, &a::jsr, 6}, {"ora", &a::addr_idy, &a::ora, 5}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"bit", &a::addr_zp, &a::bit, 3},  {"and", &a::addr_zp, &a::_and, 3}, {"rol", &a::addr_zp, &a::rol, 5},  {"???", &a::addr_imp, &a::ill, 1}, {"plp", &a::addr_imp, &a::plp, 4}, {"and", &a::addr_imm, &a::_and, 2}, {"rol", &a::addr_acc, &a::rol, 2}, {"???", &a::addr_imp, &a::ill, 1}, {"bit", &a::addr_abs, &a::bit, 4}, {"and", &a::addr_abs, &a::_and, 4}, {"rol", &a::addr_abs, &a::rol, 1}, {"???", &a::addr_imp, &a::ill, 1},
		{"bmi", &a::addr_rel, &a::bmi, 2}, {"and", &a::addr_idy, &a::_and, 5},{"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"and", &a::addr_zpx, &a::_and, 4},{"rol", &a::addr_zpx, &a::rol, 6}, {"???", &a::addr_imp, &a::ill, 1}, {"sec", &a::addr_imp, &a::sec, 2}, {"and", &a::addr_aby, &a::_and, 4}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"and", &a::addr_abx, &a::_and, 4}, {"rol", &a::addr_abx, &a::rol, 1}, {"???", &a::addr_imp, &a::ill, 1},
		{"rti", &a::addr_imp, &a::rti, 6}, {"eor", &a::addr_idx, &a::eor, 6}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"eor", &a::addr_zp, &a::eor, 3},  {"lsr", &a::addr_zp, &a::lsr, 5},  {"???", &a::addr_imp, &a::ill, 1}, {"pha", &a::addr_imp, &a::pha, 3}, {"eor", &a::addr_imm, &a::eor, 2},  {"lsr", &a::addr_acc, &a::lsr, 2}, {"???", &a::addr_imp, &a::ill, 1}, {"jmp", &a::addr_abs, &a::jmp, 3}, {"eor", &a::addr_abs, &a::eor, 4},  {"lsr", &a::addr_abs, &a::lsr, 6}, {"???", &a::addr_imp, &a::ill, 1},
		{"bvc", &a::addr_rel, &a::bvc, 2}, {"eor", &a::addr_idy, &a::eor, 5}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"eor", &a::addr_zp, &a::eor, 3},  {"lsr", &a::addr_zpx, &a::lsr, 6}, {"???", &a::addr_imp, &a::ill, 1}, {"cli", &a::addr_imp, &a::cli, 2}, {"eor", &a::addr_aby, &a::eor, 4},  {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"eor", &a::addr_abx, &a::eor, 4},  {"lsr", &a::addr_abx, &a::lsr, 7}, {"???", &a::addr_imp, &a::ill, 1},
		{"rts", &a::addr_imp, &a::rts, 6}, {"adc", &a::addr_idx, &a::adc, 6}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"adc", &a::addr_zp, &a::adc, 3},  {"ror", &a::addr_zp, &a::ror, 5},  {"???", &a::addr_imp, &a::ill, 1}, {"pla", &a::addr_imp, &a::pla, 4}, {"adc", &a::addr_imm, &a::adc, 2},  {"ror", &a::addr_acc, &a::ror, 2}, {"???", &a::addr_imp, &a::ill, 1}, {"jmp", &a::addr_ind, &a::jmp, 5}, {"adc", &a::addr_abs, &a::adc, 4},  {"ror", &a::addr_abs, &a::ror, 6}, {"???", &a::addr_imp, &a::ill, 1},
		{"bvs", &a::addr_rel, &a::bvs, 2}, {"adc", &a::addr_idy, &a::adc, 5}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"adc", &a::addr_zpx, &a::adc, 4}, {"ror", &a::addr_zpx, &a::ror, 6}, {"???", &a::addr_imp, &a::ill, 1}, {"sei", &a::addr_imp, &a::sei, 2}, {"adc", &a::addr_aby, &a::adc, 4},  {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"adc", &a::addr_abx, &a::adc, 4},  {"ror", &a::addr_abx, &a::ror, 7}, {"???", &a::addr_imp, &a::ill, 1},
		{"???", &a::addr_imp, &a::ill, 1}, {"sta", &a::addr_idx, &a::sta, 6}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"sty", &a::addr_zp, &a::sty, 3},  {"sta", &a::addr_zp, &a::sta, 3},  {"stx", &a::addr_zp, &a::stx, 3} , {"???", &a::addr_imp, &a::ill, 1}, {"dey", &a::addr_imp, &a::dey, 2}, {"???", &a::addr_imp, &a::ill, 1},  {"txa", &a::addr_imp, &a::txa, 2}, {"???", &a::addr_imp, &a::ill, 1}, {"sty", &a::addr_abs, &a::sty, 4}, {"sta", &a::addr_abs, &a::sta, 4},  {"stx", &a::addr_abs, &a::stx, 4}, {"???", &a::addr_imp, &a::ill, 1},
		{"bcc", &a::addr_rel, &a::bcc, 2}, {"sta", &a::addr_idy, &a::sta, 6}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"sty", &a::addr_zpx, &a::sty, 4}, {"sta", &a::addr_zpx, &a::sta, 4}, {"stx", &a::addr_zpy, &a::stx, 4}, {"???", &a::addr_imp, &a::ill, 1}, {"tya", &a::addr_imp, &a::tya, 2}, {"sta", &a::addr_aby, &a::sta, 5},  {"txs", &a::addr_imp, &a::txs, 2}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"sta", &a::addr_abx, &a::sta, 5},  {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1},
		{"ldy", &a::addr_imm, &a::ldy, 2}, {"lda", &a::addr_idx, &a::lda, 6}, {"ldx", &a::addr_imm, &a::ldx, 2}, {"???", &a::addr_imp, &a::ill, 1}, {"ldy", &a::addr_zp, &a::ldy, 3},  {"lda", &a::addr_zp, &a::lda, 3},  {"ldx", &a::addr_zp, &a::ldx, 3},  {"???", &a::addr_imp, &a::ill, 1}, {"tay", &a::addr_imp, &a::tay, 2}, {"lda", &a::addr_imm, &a::lda, 2},  {"tax", &a::addr_imp, &a::tax, 2}, {"???", &a::addr_imp, &a::ill, 1}, {"ldy", &a::addr_abs, &a::ldy, 4}, {"lda", &a::addr_abs, &a::lda, 4},  {"ldx", &a::addr_abs, &a::ldx, 4}, {"???", &a::addr_imp, &a::ill, 1},
		{"bcs", &a::addr_rel, &a::bcs, 2}, {"lda", &a::addr_idy, &a::lda, 5}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"ldy", &a::addr_zpx, &a::ldy, 4}, {"lda", &a::addr_zpx, &a::lda, 4}, {"ldx", &a::addr_zpy, &a::ldx, 4}, {"???", &a::addr_imp, &a::ill, 1}, {"clv", &a::addr_imp, &a::clv, 2}, {"lda", &a::addr_aby, &a::lda, 4},  {"tsx", &a::addr_imp, &a::tsx, 2}, {"???", &a::addr_imp, &a::ill, 1}, {"ldy", &a::addr_abx, &a::ldy, 1}, {"lda", &a::addr_abx, &a::lda, 1},  {"ldx", &a::addr_aby, &a::ldx, 1}, {"???", &a::addr_imp, &a::ill, 1},
		{"cpy", &a::addr_imm, &a::cpy, 2}, {"cmp", &a::addr_idx, &a::cmp, 6}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"cpy", &a::addr_zp, &a::cpy, 3},  {"cmp", &a::addr_zp, &a::cmp, 3},  {"dec", &a::addr_zp, &a::dec, 5},  {"???", &a::addr_imp, &a::ill, 1}, {"iny", &a::addr_imp, &a::iny, 2}, {"cmp", &a::addr_imm, &a::cmp, 2},  {"dex", &a::addr_imp, &a::dex, 2}, {"???", &a::addr_imp, &a::ill, 1}, {"cpy", &a::addr_abs, &a::cpy, 4}, {"cmp", &a::addr_abs, &a::cmp, 4},  {"dec", &a::addr_abs, &a::dec, 6}, {"???", &a::addr_imp, &a::ill, 1},
		{"bne", &a::addr_rel, &a::bne, 2}, {"cmp", &a::addr_idy, &a::cmp, 5}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"cmp", &a::addr_zpx, &a::cmp, 4}, {"dec", &a::addr_zpx, &a::cmp, 6}, {"???", &a::addr_imp, &a::ill, 1}, {"cld", &a::addr_imp, &a::cld, 2}, {"cmp", &a::addr_aby, &a::cmp, 4},  {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"cmp", &a::addr_abx, &a::cmp, 4},  {"dec", &a::addr_abx, &a::cmp, 7}, {"???", &a::addr_imp, &a::ill, 1},
		{"cpx", &a::addr_imm, &a::cpx, 2}, {"sbc", &a::addr_idx, &a::sbc, 6}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"cpx", &a::addr_zp, &a::cpx, 3},  {"sbc", &a::addr_zp, &a::sbc, 3},  {"inc", &a::addr_zp, &a::inc, 5},  {"???", &a::addr_imp, &a::ill, 1}, {"inx", &a::addr_imp, &a::inx, 2}, {"sbc", &a::addr_imm, &a::sbc, 2},  {"nop", &a::addr_imp, &a::nop, 2}, {"???", &a::addr_imp, &a::ill, 1}, {"cpx", &a::addr_abs, &a::cpx, 4}, {"sbc", &a::addr_abs, &a::sbc, 4},  {"inc", &a::addr_abs, &a::inc, 6}, {"???", &a::addr_imp, &a::ill, 1},
		{"beq", &a::addr_rel, &a::beq, 2}, {"sbc", &a::addr_idy, &a::sbc, 5}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"sbc", &a::addr_zpx, &a::sbc, 4}, {"inc", &a::addr_zpx, &a::inc, 6}, {"???", &a::addr_imp, &a::ill, 1}, {"sed", &a::addr_imp, &a::sed, 2}, {"sbc", &a::addr_aby, &a::sbc, 4},  {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"???", &a::addr_imp, &a::ill, 1}, {"sbc", &a::addr_abx, &a::sbc, 4},  {"inc", &a::addr_abx, &a::inc, 7}, {"???", &a::addr_imp, &a::ill, 1},
	};
}

c6502::~c6502() { }

void c6502::set_flag(u8 fl, bool cond) {
	if(cond) {
		BIT_SET(ps, fl);
	}
	else {
		BIT_CLR(ps, fl);
	}
}

void c6502::set_org(u16 org) {
	this->pc = org;
}

void c6502::clock() {
	if(this->cycles == 0) {
		//we're ready for the next instruction
		//ensure the unused flag stays at 1
		BIT_SET(this->ps, 5);

		//get the next instruction
		u8 opcode = bus->read(pc);
		pc++;

		//update remaining clock cycles
		cycles = this->opcode_tbl[opcode].clocks;

		//set up the effective address
		//as a side note... who came up with this syntax?
		this->addr_is_imm = false;
		this->addr_is_acc = false;
		eff_addr = (this->*opcode_tbl[opcode].addr_mode)();

		//check for debugging after the address is computed, but before the instr. executes
		if(DEBUG_6502)
			bus->debug(this->opcode_tbl[opcode], opcode);

		//execute the instruction
		this->cycles += (this->*opcode_tbl[opcode].inst)();
	}

	cycles--;
}

void c6502::reset() {

}

void c6502::nmi() {

}

void c6502::irq() {

}

// Addressing modes
// ****************

u16 c6502::addr_imp() {
	//this addr mode does nothing
	//except requiring two cycles to complete
	return NO_ADDR;
}

u16 c6502::addr_acc() {
	//this addr mode also does basically nothing
	//the instruction operates directly on the accumulator
	addr_is_acc = true;
	return NO_ADDR;
}

u16 c6502::addr_imm() {
	//requires one extra byte to specify and 8-bit imm. value
	//NOTE! this function (confusingly) returns the imm. NOT an addr.
	u8 addr = bus->read(pc);
	pc++;
	addr_is_imm = true;
	return addr; //implicitly zero extended
}

u16 c6502::addr_zp() {
	//by how we're doing it, this is the same as imm.
	//except it actually returns an address
	u8 addr = bus->read(pc);
	pc++;
	return addr; //implicitly zero extended
}

u16 c6502::addr_abs() {
	//this one requires two bytes
	u8 lo = bus->read(pc);
	pc++;
	u8 hi = bus->read(pc);
	pc++;

	u16 ret = lo;
	return (ret |= (hi << 8));
}

u16 c6502::addr_rel() {
	u16 addr = 0;
	i8 off = (i8)bus->read(pc);
	pc++;

	//PC is incremented first which means
	//addr can be -126 to +129 bytes from the old PC
	addr = off + pc;
	return addr;
}

u16 c6502::addr_ind() {
	//only supported by the JMP instruction
	//a full 16 bit addr. contains the address of the LSB
	//of another full 16 bit addr - which is the target

	//NOTE! there is a hardware bug in this instruction which is
	//simulated here - if addr_lo is 0xFF (pg. boundary) a real 
	//2A03 will (wrongly) wrap around to the beginning of the same page
	//instead of crossing the boundary
	u8 addr_lo = bus->read(pc);
	pc++;
	u8 addr_hi = bus->read(pc);
	pc++;

	u16 ind = (addr_hi << 8) | addr_lo;
	
	if(addr_lo == 0xFF) { //bug
		//For example instead of reading 0x30FF and 0x3100
		//we actually read 0x30FF and 0x3000
		addr_lo = bus->read(ind);
		addr_hi = bus->read(ind - 0xFF);
	}
	else {
		addr_lo = bus->read(ind);
		addr_hi = bus->read(ind + 1);
	}

	u16 ret = addr_lo;
	return ret |= (addr_hi << 8);
}

u16 c6502::addr_zpx() {
	//add value in x register to the 8 bit imm.
	//wrap around if value exceeds 0xFF
	u8 imm = bus->read(pc);
	pc++;

	return (imm + this->x) & 0x00FF;
}

u16 c6502::addr_zpy() {
	//add value in y register to the 8 bit imm.
	//wrap around if value exceeds 0xFF
	u8 imm = bus->read(pc);
	pc++;

	return (imm + this->y) & 0x00FF;
}

u16 c6502::addr_idx() {
	//similar to zpx - except indirect
	//add value in x register to the 8 bit imm.
	//wrap around if value exceeds 0xFF
	u8 imm = bus->read(pc);
	pc++;

	u16 ind = (imm + this->x) & 0x00FF;

	u16 ret = bus->read(ind);
	ret |= (bus->read(ind + 1) << 8);
	return ret;
}

u16 c6502::addr_idy() {
	//a bit different from zpy and idx in that
	//the address is fetched BEFORE y is added
	u8 imm = bus->read(pc);
	pc++;

	u16 ind = bus->read(imm);
	ind |= (bus->read(imm + 1) << 8);

	u16 ret = ind + this->y;

	//add a cycle if we cross a pg. boundary
	if((ret & 0xFF00) != (ind & 0xFF00)) {
		cycles++;
	}

	return ret;
}

u16 c6502::addr_abx() {
	//read 16 bit address from the instruction
	//then add the x register to it
	u8 lo = bus->read(pc);
	pc++;
	u8 hi = bus->read(pc);
	pc++;

	u16 ret = ((hi << 8) | lo) + this->x;

	//take an extra cycle if the add crosses pg. boundary
	if((ret & 0xFF00) != (hi << 8)) {
		cycles++;
	}

	return ret;
}

u16 c6502::addr_aby() {
	//read 16 bit address from the instruction
	//then add the y register to it
	u8 lo = bus->read(pc);
	pc++;
	u8 hi = bus->read(pc);
	pc++;

	u16 ret = ((hi << 8) | lo) + this->y;

	if((ret & 0xFF00) != (hi << 8)) {
		cycles++;
	}

	return ret;
}

// Instructions
// ************

// 1. Load-store

u8 c6502::lda() {
	if(addr_is_imm) {
		a = eff_addr & 0x00FF;
	}
	else {
		a = bus->read(eff_addr);
	}

	//set flags
	set_flag(ZF, a == 0);
	set_flag(NF, a & 0x80);

	return 0;
}

u8 c6502::ldx() {
	if(addr_is_imm) {
		x = eff_addr & 0x00FF;
	}
	else {
		x = bus->read(eff_addr);
	}

	//set flags
	set_flag(ZF, x == 0);
	set_flag(NF, x & 0x80);

	return 0;
}

u8 c6502::ldy() {
	if(addr_is_imm) {
		y = eff_addr & 0x00FF;
	}
	else {
		y = bus->read(eff_addr);
	}

	//set flags
	set_flag(ZF, y == 0);
	set_flag(NF, y & 0x80);

	return 0;
}

u8 c6502::sta() {
	bus->write(eff_addr, a);
	return 0;
}

u8 c6502::stx() {
	bus->write(eff_addr, x);
	return 0;
}

u8 c6502::sty() {
	bus->write(eff_addr, y);
	return 0;
}

// 2. Register Transfers
u8 c6502::tax() {
	x = a;

	set_flag(ZF, x == 0);
	set_flag(NF, x & 0x80);

	return 0;
}

u8 c6502::tay() {
	y = a;

	set_flag(ZF, y == 0);
	set_flag(NF, y & 0x80);

	return 0;
}

u8 c6502::txa() {
	a = x;

	set_flag(ZF, a == 0);
	set_flag(NF, a & 0x80);

	return 0;
}

u8 c6502::tya() {
	a = y;

	set_flag(ZF, a == 0);
	set_flag(NF, a & 0x80);

	return 0;
}

// 3. Stack things
u8 c6502::txs() {
	sp = x;

	return 0;
}

u8 c6502::tsx() {
	x = sp;

	return 0;
}

u8 c6502::pha() {
	//stack starts as 0x100
	bus->write((u16)sp + 0x0100, a);
	sp--;

	return 0;
}

u8 c6502::php() {
	//the U flag should always be pushed as 1
	//the B (break) flag should be 1 when pushed from PHP
	bus->write((u16)sp + 0x0100, ps);
	sp--;

	return 0;
}

u8 c6502::pla() {
	sp++;
	a = bus->read((u16)sp + 0x0100);

	set_flag(ZF, a == 0);
	set_flag(NF, a & 0x80);

	return 0;
}

u8 c6502::plp() {
	sp++;
	ps = bus->read((u16)sp + 0x0100);

	return 0;
}

// 4. Logical operations

u8 c6502::_and() {
	if(addr_is_imm) {
		a &= eff_addr;
	}
	else {
		a &= bus->read(eff_addr);
	}

	set_flag(ZF, a == 0);
	set_flag(NF, a & 0x80);

	return 0;
}

u8 c6502::eor() {
	if(addr_is_imm) {
		a ^= eff_addr;
	}
	else {
		a ^= bus->read(eff_addr);
	}

	set_flag(ZF, a == 0);
	set_flag(NF, a & 0x80);

	return 0;
}

u8 c6502::ora() {
	if(addr_is_imm) {
		a |= eff_addr;
	}
	else {
		a |= bus->read(eff_addr);
	}

	set_flag(ZF, a == 0);
	set_flag(NF, a & 0x80);

	return 0;
}

u8 c6502::bit() {
	//acc. acts as the mask
	u8 mem = bus->read(eff_addr);
	set_flag(ZF, (mem & a) == 0);

	set_flag(NF, mem & 0x80);
	set_flag(OF, mem & 0x40);

	return 0;
}

// 5. Arithmetic operations

//Performs [a + value + old_carry]
//Carry is SET if an overflow occurs 
u8 c6502::adc() {
	u8 fetched = 0;
	u16 result = 0;


	if(addr_is_imm) {
		fetched = (u8)eff_addr;
	}
	else {
		fetched = bus->read(eff_addr);
	}

	result = a + fetched + (ps & (1 << CF));

	//check if a carry-out happened
	set_flag(CF, result > 255);

	//set the V flag if the result's sign as a two's compl. integer is "incorrect"
	//sign is considered incorrect if the sign of BOTH inputs is different from the result
	set_flag(OF, (((u16)a ^ result) & ((u16)fetched ^ result)) & 0x0080);

	a = result & 0x00FF;

	set_flag(NF, a & 0x80);
	set_flag(ZF, a == 0);

	return 0;
}

//Performs [a - value + old_carry]
//The compliment of the carry flag acts as the borrow bit
//so carry is cleared if an overflow (borrow) occurs
u8 c6502::sbc() {
	//adc but with the value inverted
	u8 fetched = 0;
	u16 result = 0;

	if(addr_is_imm) {
		fetched = (u8)eff_addr;
	}
	else {
		fetched = bus->read(eff_addr);
	}

	//this line is the only difference between sbc and adc
	u16 value = (u16)fetched ^ 0x00FF;

	result = a + value + (ps & (1 << CF));

	//set flags (they work the same as in adc)
	set_flag(CF, result > 255);
	set_flag(OF, (((u16)a ^ result) & ((u16)value ^ result)) & 0x0080);

	a = result & 0x00FF;

	set_flag(ZF, a == 0);
	set_flag(NF, a & 0x80);

	return 0;
}

u8 c6502::cmp() {
	u8 fetched = 0;
	if(addr_is_imm) {
		fetched = (u8)eff_addr;
	}
	else {
		fetched = bus->read(eff_addr);
	}

	u16 result = a - fetched;

	//cmp only sets flags
	set_flag(CF, a >= fetched);
	set_flag(ZF, result == 0);
	set_flag(NF, result & 0x80);

	return 0;
}

u8 c6502::cpx() {
	u8 fetched = 0;
	if(addr_is_imm) {
		fetched = (u8)eff_addr;
	}
	else {
		fetched = bus->read(eff_addr);
	}

	u16 result = x - fetched;

	//cmp only sets flags
	set_flag(CF, x >= fetched);
	set_flag(ZF, result == 0);
	set_flag(NF, result & 0x80);

	return 0;
}

u8 c6502::cpy() {
	u8 fetched = 0;
	if(addr_is_imm) {
		fetched = (u8)eff_addr;
	}
	else {
		fetched = bus->read(eff_addr);
	}

	u16 result = y - fetched;

	//cmp only sets flags
	set_flag(CF, y >= fetched);
	set_flag(ZF, result == 0);
	set_flag(NF, result & 0x80);

	return 0;
}

// 6. Increments and decrements

u8 c6502::inc() {
	u8 value = bus->read(eff_addr);
	bus->write(eff_addr, value++);

	set_flag(ZF, value == 0);
	set_flag(NF, value & 0x80);

	return 0;
}

u8 c6502::inx() {
	x++;

	set_flag(ZF, x == 0);
	set_flag(NF, x & 0x80);

	return 0;
}

u8 c6502::iny() {
	y++;

	set_flag(ZF, y == 0);
	set_flag(NF, y & 0x80);

	return 0;
}

u8 c6502::dec() {
	u8 value = bus->read(eff_addr);
	bus->write(eff_addr, value--);

	set_flag(ZF, value == 0);
	set_flag(NF, value & 0x80);

	return 0;
}

u8 c6502::dex() {
	x--;

	set_flag(ZF, x == 0);
	set_flag(NF, x & 0x80);

	return 0;
}

u8 c6502::dey() {
	y--;

	set_flag(ZF, y == 0);
	set_flag(NF, y & 0x80);

	return 0;
}

// 7. Shifts

u8 c6502::asl() {
	u16 result = 0;

	if(addr_is_acc) {
		//accumulator mode
		result = a << 1;
		a = (u8)result;
	} 
	else {
		//otherwise operate on memory
		u8 value = bus->read(eff_addr);
		result = bus->write(eff_addr, value << 1);
	}

	set_flag(ZF, result == 0);
	set_flag(CF, (result & 0xFF00) != 0);
	set_flag(NF, result & 0x80);

	return 0;
}

u8 c6502::lsr() {
	u16 result = 0;

	//we can set the carry before we shift out the LSB
	if(addr_is_acc) {
		//accumulator mode
		set_flag(CF, a & 0x01);
		result = a >> 1;
		a = (u8)result;
	}
	else {
		//otherwise operate on memory
		u8 value = bus->read(eff_addr);
		set_flag(CF, value & 0x01);
		result = bus->write(eff_addr, value >> 1);
	}

	set_flag(ZF, (u8)result == 0);

	//wouldn't this always be cleared??
	//I can't seem to see this mentioned anywhere
	set_flag(NF, (u8)result & 0x80);

	return 0;
}

//note: rol and ror rotate through the carry bit
//i.e. for rol [carry -> bit0] and [bit7 -> carry]
u8 c6502::rol() {
	u16 result = 0;

	if(addr_is_acc) {
		result = (a << 1) | (ps & CF);
		set_flag(CF, result & 0xFF00);

		a = (u8)(result & 0x00FF);
	}
	else {
		result = (bus->read(eff_addr) << 1) | (ps & CF); 
		set_flag(CF, result & 0xFF00);

		bus->write(eff_addr, (u8)(result & 0x00FF));
	}

	set_flag(ZF, (u8)result == 0);
	set_flag(NF, (u8)result & 0x80);

	return 0;
}

//perform a rotate right through the carry bit
//[carry -> bit7] and [bit0 -> carry]
u8 c6502::ror() {
	u16 result = 0;

	if(addr_is_acc) {
		result = ((ps & CF) << 7) | (a >> 1);
		set_flag(CF, result & 0xFF00);

		a = (u8)(result & 0x00FF);
	}
	else {
		//we should set the carry before we discard the LSB
		u8 value = bus->read(eff_addr);
		set_flag(CF, value & 0x01);

		result = ((ps & CF) << 7) | (value >> 1);

		bus->write(eff_addr, (u8)(result & 0x00FF));
	}

	set_flag(ZF, (u8)result == 0);
	set_flag(NF, (u8)result & 0x80);

	return 0;
}

// 8. Jumps and calls

u8 c6502::jmp() {
	//this one's quite simple
	pc = eff_addr;

	return 0;
}

u8 c6502::jsr() {
	//first we should un-increment the PC
	pc--;

	/* 
	* push the PC - high byte first
	* not sure if the 2A03 pushes the low or high byte first
	* but in the emulator it doesn't matter as long as we pull
	* in the correct order in RTS
	*/
	bus->write((u16)(0x0100 + sp), (pc & 0xFF00) >> 8);
	sp--;
	bus->write((u16)(0x0100 + sp), pc & 0x00FF);
	sp--;

	//jump to the target
	pc = eff_addr;

	return 0;
}

u8 c6502::rts() {
	sp++;
	pc = bus->read(0x100 + sp);
	sp++;
	pc |= bus->read(0x100 + sp) << 8;

	pc++;

	return 0;
}

// 9. Branches

u8 c6502::bcc() {

	if(BIT_CHK(ps, CF) == 0) {
		//add a cycle if the branch succeeds
		cycles++;

		//add 1 more if the branch is to a new page
		//i.e. the high byte of the PC is changed
		if((pc & 0xFF00) != (eff_addr & 0xFF00)) {
			cycles++;
		}

		pc = eff_addr;
	}

	return 0;
}

u8 c6502::bcs() {

	if(BIT_CHK(ps, CF)) {
		//add a cycle if the branch succeeds
		cycles++;

		//add 1 more if the branch is to a new page
		//i.e. the high byte of the PC is changed
		if((pc & 0xFF00) != (eff_addr & 0xFF00)) {
			cycles++;
		}

		pc = eff_addr;
	}

	return 0;
}

u8 c6502::beq() {

	if(BIT_CHK(ps, ZF)) {
		//add a cycle if the branch succeeds
		cycles++;

		//add 1 more if the branch is to a new page
		//i.e. the high byte of the PC is changed
		if((pc & 0xFF00) != (eff_addr & 0xFF00)) {
			cycles++;
		}

		pc = eff_addr;
	}

	return 0;
}

u8 c6502::bmi() {

	if(BIT_CHK(ps, NF)) {
		//add a cycle if the branch succeeds
		cycles++;

		//add 1 more if the branch is to a new page
		//i.e. the high byte of the PC is changed
		if((pc & 0xFF00) != (eff_addr & 0xFF00)) {
			cycles++;
		}

		pc = eff_addr;
	}

	return 0;
}

u8 c6502::bne() {

	if(BIT_CHK(ps, ZF) == 0) {
		//add a cycle if the branch succeeds
		cycles++;

		//add 1 more if the branch is to a new page
		//i.e. the high byte of the PC is changed
		if((pc & 0xFF00) != (eff_addr & 0xFF00)) {
			cycles++;
		}

		pc = eff_addr;
	}

	return 0;
}

u8 c6502::bpl() {

	if(BIT_CHK(ps, NF) == 0) {
		//add a cycle if the branch succeeds
		cycles++;

		//add 1 more if the branch is to a new page
		//i.e. the high byte of the PC is changed
		if((pc & 0xFF00) != (eff_addr & 0xFF00)) {
			cycles++;
		}

		pc = eff_addr;
	}

	return 0;
}

u8 c6502::bvc() {

	if(BIT_CHK(ps, OF) == 0) {
		//add a cycle if the branch succeeds
		cycles++;

		//add 1 more if the branch is to a new page
		//i.e. the high byte of the PC is changed
		if((pc & 0xFF00) != (eff_addr & 0xFF00)) {
			cycles++;
		}

		pc = eff_addr;
	}

	return 0;
}

u8 c6502::bvs() {

	if(BIT_CHK(ps, OF)) {
		//add a cycle if the branch succeeds
		cycles++;

		//add 1 more if the branch is to a new page
		//i.e. the high byte of the PC is changed
		if((pc & 0xFF00) != (eff_addr & 0xFF00)) {
			cycles++;
		}

		pc = eff_addr;
	}

	return 0;
}

// 10. Status flag operations

u8 c6502::clc() {
	set_flag(CF, false);

	return 0;
}

u8 c6502::cld() {
	//doesn't "exist" on the NES
	//but we'll do it for complete-ness sake
	set_flag(DF, false);

	return 0;
}

u8 c6502::cli() {
	set_flag(ID, false);

	return 0;
}

u8 c6502::clv() {
	set_flag(OF, false);

	return 0;
}

u8 c6502::sec() {
	set_flag(CF, true);

	return 0;
}

u8 c6502::sed() {
	//doesn't "exist" on the NES - but again
	//we'll do it for the sake of sanity
	set_flag(DF, true);

	return 0;
}

u8 c6502::sei() {
	set_flag(ID, true);

	return 0;
}

// 11. Misc.

u8 c6502::brk() {
	//force an irq to take place
	//brk double-incerments the pc
	pc++;

	//first push the pc, then the status register
	bus->write((u16)(0x0100 + sp), (pc & 0xFF00) >> 8);
	sp--;
	bus->write((u16)(0x0100 + sp), pc & 0x00FF);
	sp--;

	set_flag(BF, true);
	bus->write((u16)(0x0100 + sp), ps);
	sp--;

	//the cpu only maintains that BF will be pushed as 1
	//after the push (even before RTI is executed) all bets are off
	set_flag(BF, false);

	// PC <- 0xFFFE-FFFF
	pc = bus->read(0xFFFE) & 0x00FF;
	pc |= bus->read(0xFFFF) << 8;

	return 0;
}

u8 c6502::nop() {
	//do nothing and take 2 cycles while doing it!
	return 0;
}

u8 c6502::rti() {
	//pull the status regsiter and then the PC
	sp++;
	ps = bus->read(sp);

	//we need to make sure that the break flag is not set
	//and that the unused flag is
	set_flag(UF, true);
	set_flag(BF, false);

	sp++;
	pc = bus->read(0x100 + sp);
	sp++;
	pc |= bus->read(0x100 + sp) << 8;

	return 0;
}

// 12. Illegal opcode

u8 c6502::ill() {
	/*
	* pretend nothing happened
	* but in reality these actually do something, sometimes
	* and there are games that use them
	* TODO possibly implement later
	*/
	return 0;
}

