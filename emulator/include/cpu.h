#ifndef CPU_H
#define CPU_H

#include <common.h>

#define reg(x) cpu->registers[x]
#define sp reg(16)
#define ip reg(17)

enum {
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
	u8 instruction[6];
	u8 * memory;
	u32 memory_size;
	u32 mmio_start;
	u8 gpio[0xFFFF];
	u8 hlt;
} cpu_t;

typedef void (* instruction_func)(cpu_t * cpu, u8 mod, u32 value);

void cpu_free(cpu_t * cpu);
cpu_t * cpu_new(u32 memory_size);
void cpu_load_rom(cpu_t * cpu, u8 * rom, u32 len);
u32 cpu_run(cpu_t * cpu);

#endif
