#include "dbg.h"
#include "bitops.h"
#include "6502.h"
#include "text.h"
#include "bus.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <algorithm>

c6502Dbg::c6502Dbg(u32 width, u32 height, c6502 *cpuptr, cDataBus *busptr) 
	: cpu{cpuptr}, win_width{width}, win_height{height}, bus{busptr}
{
	sf::VideoMode v(width, height);
	window = new sf::RenderWindow(v, "Ben's NES Debugger");

	text1 = new cTextBox(20, 20, 24, "Ben's NES Debugger");
	box1.setFillColor(sf::Color::White);
	box1.setPosition(10, 10);
	box1.setSize(sf::Vector2f(280, 50));
	box2.setFillColor(sf::Color::Blue);
	box2.setPosition(15, 15);
	box2.setSize(sf::Vector2f(270, 40));

	registers = new cTextBox(20, 80, 24, "Registers\n\nPC: $\nSP: $\n\nA:  $\nX:  $\nY:  $\n\nP:");
	reg_values = new cTextBox(100, 125, 24, "");

	memory = new cTextBox(360, 20, 24, "Zero-Page");
	mem_vals = new cTextBox(360, 60, 24, "");

	_n = new cTextBox(80, 295, 24, "N");
	_v = new cTextBox(100, 295, 24, "V");
	_u = new cTextBox(120, 295, 24, "-");
	_b = new cTextBox(140, 295, 24, "B");
	_d = new cTextBox(160, 295, 24, "D");
	_i = new cTextBox(180, 295, 24, "I");
	_z = new cTextBox(200, 295, 24, "Z");
	_c = new cTextBox(220, 295, 24, "C");
	flags = {_c, _z, _i, _d, _b, _u, _v, _n};

	instruction = new cTextBox(20, 440, 24, "Disassembly");
	disas = new cTextBox(20, 460, 32, "");

	components = {text1, registers, memory, mem_vals, instruction, disas, reg_values, _n, _v, _u, _b, _d, _i, _z, _c};
}

c6502Dbg::~c6502Dbg() {
	delete window;

	for(auto item : components) {
		delete item;
	}
}

u32 c6502Dbg::update(c6502_Instruction inst, u8 opcode) {
	//update the register view
	char buf[256];
	sprintf(buf, "%04x\n%02x (+0x100)\n\n%02x\n%02x\n%02x\n", cpu->pc, cpu->sp, cpu->a, cpu->x, cpu->y);
	reg_values->set_text(std::string(buf));

	//update memory view
	std::stringstream mem;
	for(int i = 0; i < 16; i++) {
		mem << std::hex << std::setfill('0') << std::setw(4) << i * 16;
		for(int k = 0; k < 16; k++) {
			mem << " " << std::hex << std::setfill('0') << std::setw(2) << (u32)bus->mem[(i * 16) + k];
		}
		mem << std::endl;
	}
	mem_vals->set_text(mem.str());

	//update status flags
	for(int i = 0; i < 8; i++) {
		if(BIT_CHK(cpu->ps, i)) {
			flags[i]->set_color(sf::Color::Green);
		}
		else {
			flags[i]->set_color(sf::Color::Red);
		}
	}

	//draw the disassembly
	disas->set_text(disassemble(inst, opcode));

	//update the window
	sf::Event e;
	while(window->pollEvent(e)) {
		if(e.type == sf::Event::Closed) {
			window->close();
			return 0;
		}
	}

	window->clear(sf::Color::Blue);

	window->draw(box1);
	window->draw(box2);
	for(auto item : components) {
		item->draw(window);
	}
	
	//default constructor picks black
	window->display();

	return 1;
}

u32 c6502Dbg::debug(c6502_Instruction inst, u8 opcode) {
	//first check for debug events
	switch(state) {
	case RUNNING:
		return update(inst, opcode);
	case STEPPING:
		state = WAITING;
		break;
	case WAITING:
		break;
	}

	//wait here until the user steps or continues
	while(state == WAITING)
		update(inst, opcode);

	//update the debugger display


	//returns 1 when the debugger has not quit
	return 1;
}

void c6502Dbg::setbrk(u16 addr) {
	brkpoints.push_back(addr);
}

void c6502Dbg::clrbrk(u16 addr) {
	brkpoints.erase(std::remove(brkpoints.begin(), brkpoints.end(), addr), brkpoints.end());
}

void c6502Dbg::brk() {
	state = WAITING;
}

void c6502Dbg::cont() {
	state = RUNNING;
}

std::string dec2hex(u8 byte) {
	std::stringstream ss;
	ss << std::hex << std::setfill('0') << std::setw(2) << (u32)byte;
	return ss.str();
}

std::string c6502Dbg::disassemble(c6502_Instruction inst, u8 opcode) {

	using c = c6502;
	u16 addr = cpu->pc - 1;
	std::stringstream ss;

	ss << inst.name << " ";

	if(inst.addr_mode == &c::addr_abs) {
		ss << "$" << dec2hex(bus->mem[addr + 2]) << dec2hex(bus->mem[addr + 1]);
		ss << std::endl;
	}
	else if(inst.addr_mode == &c::addr_abx) {
		ss << "$" << dec2hex(bus->mem[addr + 2]) << dec2hex(bus->mem[addr + 1]);
		ss << ", X" << std::endl;
	}
	else if(inst.addr_mode == &c::addr_aby) {
		ss << "$" << dec2hex(bus->mem[addr + 2]) << dec2hex(bus->mem[addr + 1]);
		ss << ", Y" << std::endl;
	}
	else if(inst.addr_mode == &c::addr_acc) {
		ss << "A" << std::endl;
	}
	else if(inst.addr_mode == &c::addr_idx) {
		ss << "($" << dec2hex(bus->mem[addr + 1]) << ", X)";
		ss << std::endl;
	}
	else if(inst.addr_mode == &c::addr_idy) {
		ss << "($" << dec2hex(bus->mem[addr + 1]) << "), Y";
		ss << std::endl;
	}
	else if(inst.addr_mode == &c::addr_imm) {
		ss << "#$" << dec2hex(bus->mem[addr + 1]);
		ss << std::endl;
	}
	else if(inst.addr_mode == &c::addr_imp) {
		ss << std::endl;
	}
	else if(inst.addr_mode == &c::addr_ind) {
		ss << "($" << dec2hex(bus->mem[addr + 2]) << dec2hex(bus->mem[addr + 1]) << ")";
		ss << std::endl;	
	}
	else if(inst.addr_mode == &c::addr_rel) {
		ss << "#$" << dec2hex(bus->mem[addr + 1]);
		ss << std::endl;
	}
	else if(inst.addr_mode == &c::addr_zp) {
		ss << "$" << dec2hex(bus->mem[addr + 1]);
		ss << std::endl;
	}
	else if(inst.addr_mode == &c::addr_zpx) {
		ss << "$" << dec2hex(bus->mem[addr + 1]) << ", X";
		ss << std::endl;
	}
	else if(inst.addr_mode == &c::addr_zpy) {
		ss << "$" << dec2hex(bus->mem[addr + 1]) << ", Y";
		ss << std::endl;
	}

	return ss.str();
}

