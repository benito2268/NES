#pragma once

#include "defs.h"
#include "drawable.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class c6502;
class cDataBus;
class cTextBox;
struct c6502_Instruction;

enum ProgState {
	RUNNING,
	WAITING,
	STEPPING,
};

class c6502Dbg {
private:
	u32 win_height;
	u32 win_width;

	sf::RenderWindow *window;

	ProgState state = RUNNING;

	c6502 *cpu;
	cDataBus *bus;

	std::vector<u16> brkpoints;

	//title
	cTextBox *text1;
	sf::RectangleShape box1;
	sf::RectangleShape box2;

	//registers
	cTextBox *registers;
	cTextBox *reg_values;

	//memory
	cTextBox *memory;
	cTextBox *mem_vals;

	//flags
	cTextBox *_c;
	cTextBox *_z;
	cTextBox *_i;
	cTextBox *_d;
	cTextBox *_v;
	cTextBox *_n;
	cTextBox *_b;
	cTextBox *_u;
	std::vector<cTextBox*> flags;

	//disassembler
	cTextBox *instruction;
	cTextBox *disas;

	std::vector<cDrawable*> components;

public:
	c6502Dbg(u32, u32, c6502*, cDataBus*);
	~c6502Dbg();

	u32 update(c6502_Instruction, u8);
	u32 debug(c6502_Instruction, u8); //does one instruction's worth of debugging

	void setbrk(u16);
	void clrbrk(u16);

	void brk();
	void cont();

	std::string disassemble(c6502_Instruction, u8);
};