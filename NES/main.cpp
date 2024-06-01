#include <iostream>
#include "bus.h"
#include "6502.h"
#include "dbg.h"

//NOTE: sfml freaks out if you include windows.h before it
//should always include last
#include <windows.h>

int main(int argc, char *argv[]) {
	
	//start the debugger
	cDataBus *b = new cDataBus(0x0400);
	b->load("functional.bin");
	b->attach_dbg();

	while(1) {
		b->cpu->clock();

		//if(DEBUG_6502)
			//Sleep(100);
	}

	delete b;
	return 0;
}