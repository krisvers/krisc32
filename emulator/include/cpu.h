#ifndef CPU_H
#define CPU_H

#include <common.h>

#define reg(x) cpu.registers[x]
#define sp R(16)
#define ip R(17)

enum instructions {
	I_LDI = 0,
	I_LDM,
	I_LDR,
	I_STR,
	I_ADD,
	I_SUB,
	I_MUL,
	I_DIV,
	I_BOP,
	I_JMP,
	I_IN,
	I_OUT,
};

typedef struct cpu_t {
	u32 registers[18];
	u8 instruction[7];
	u8 * memory;
	u32 memory_size;
	u8 gpio[0xFFFF];
} cpu_t;

#endif
