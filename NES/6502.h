#pragma once

#include "defs.h"
#include <string>
#include <vector>

class cDataBus;
class Tester;

#define DEBUG_6502 1
#define NO_ADDR 0

// processor status flags
#define CF 0 //carry
#define ZF 1 //zero
#define ID 2 //interrupt disable
#define OF 6 //overflow
#define NF 7 //negetive

//flags the are technically not used
#define DF 3
#define BF 4
#define UF 5

/*
* a.k.a. the Ricoh 2A03
* 
* This naming scheme is admittedly confusing, the 'c' is supposed to
* signify that this is a class - not that it is emulating
* the different yet related 65c02 processor
*/
class c6502 {
private:
	//registers
	u16 pc;
	u8  sp, a, x, y;
	u8  ps;

	//internal variables
	u16 eff_addr;
	u32 cycles;
	bool addr_is_imm;
	bool addr_is_acc;
	cDataBus *bus;

private: 

	struct c6502_Instruction {
		std::string name = ""; //for debugging
		u16(c6502::*addr_mode)(void) = nullptr;
		u8(c6502::*inst)(void) = nullptr;
		u32 clocks = 0;
	};

public:
	c6502(cDataBus *bus);
	~c6502();

	std::vector<c6502_Instruction> opcode_tbl;
	void set_flag(u8 fl, bool cond);

	friend class Tester;

public:
	//pin functions
	void clock();
	void reset();
	void nmi();
	void irq();

private:
	//addressing modes
	u16 addr_imp();
	u16 addr_acc();
	u16 addr_imm();
	u16 addr_zp();
	u16 addr_abs();
	u16 addr_rel();
	u16 addr_ind();
	u16 addr_zpx();
	u16 addr_zpy();
	u16 addr_idx();
	u16 addr_idy();
	u16 addr_abx();
	u16 addr_aby();

private:
	// instructions - return any *extra* cycles they take
	// sorted by type, without addressing modes
	// 1. Load-store
	u8 lda();
	u8 ldx();
	u8 ldy();
	u8 sta();
	u8 stx();
	u8 sty();

	// 2. Register transfers
	u8 tax();
	u8 tay();
	u8 txa();
	u8 tya();

	// 3. Stack things
	u8 tsx();
	u8 txs();
	u8 pha();
	u8 php();
	u8 pla();
	u8 plp();

	// 4. Logical operations
	u8 _and(); //note! need '_' to avoid c++ keyword
	u8 eor();
	u8 ora();
	u8 bit();

	// 5. Arithmetic operations
	u8 adc();
	u8 sbc();
	u8 cmp();
	u8 cpx();
	u8 cpy();

	// 6. Increments and decrements
	u8 inc();
	u8 inx();
	u8 iny();
	u8 dec();
	u8 dex();
	u8 dey();

	// 7. Shifts
	u8 asl();
	u8 lsr();
	u8 rol();
	u8 ror();

	// 8. Jumps and calls
	u8 jmp();
	u8 jsr();
	u8 rts();

	// 9. Branches
	u8 bcc();
	u8 bcs();
	u8 beq();
	u8 bmi();
	u8 bne();
	u8 bpl();
	u8 bvc();
	u8 bvs();

	// 10. Stauts flag changes
	u8 clc();
	u8 cld(); //note! unused on the NES
	u8 cli();
	u8 clv();
	u8 sec();
	u8 sed(); //note! unused on the NES
	u8 sei();

	// 11. Misc.
	u8 brk();
	u8 nop();
	u8 rti();

	// 12. Illegal Opcode
	u8 ill();
};
