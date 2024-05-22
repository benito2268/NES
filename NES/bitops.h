#pragma once

#define BIT_SET(byte, n)	 \
	((byte) |= (1 << (n)))

#define BIT_CLR(byte, n)	 \
	((byte) &= ~(1 << (n))) 

#define BIT_FLIP(byte, n)	 \
	((byte) ^= (1 << (n)))  

#define BIT_CHK(byte, n)	 \
	((byte) & (1 << (n)))    