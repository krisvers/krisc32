#include <cpu.h>
#include <common.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

u8 cpu_read_memory(cpu_t * cpu, u32 addr) {
	if (addr >= cpu->memory_size) {
		fprintf(stderr, "can't read from address %u!\n", addr);
		return 0;
	}

	return cpu->memory[addr];
}

void cpu_read_bytes(cpu_t * cpu, u32 addr, u8 * buffer, u32 len) {
	do {
		buffer[len - 1] = cpu_read_memory(cpu, addr + len - 1);
	} while (--len);
}

void cpu_write_memory(cpu_t * cpu, u32 addr, u8 value) {
	if (addr >= cpu->memory_size) {
		fprintf(stderr, "can't write to address %u!\n", addr);
		return;
	}

	cpu->memory[addr] = value;
}

void cpu_write_register(cpu_t * cpu, u8 r, u32 value) {
	reg(r & 0xF) = value;
}

u32 cpu_read_register(cpu_t * cpu, u8 r) {
	return reg(r & 0xF);
}

u32 cpu_rotate_right(u32 a, u32 b) {
	return a << b | a >> (32 - b);
}

u32 cpu_rotate_left(u32 a, u32 b) {
	return a >> b | a << (32 - b);
}

void i_ldi(cpu_t * cpu, u8 mod, u32 value) {
	cpu_write_register(cpu, mod, value);
}

void i_ldm(cpu_t * cpu, u8 mod, u32 addr) {
	u32 value;

	switch (mod >> 4) {
		case 0:
			value = cpu_read_memory(cpu, addr) | (cpu_read_memory(cpu, addr) << 8) | (cpu_read_memory(cpu, addr) << 16) | (cpu_read_memory(cpu, addr) << 24);
			break;
		case 1:
			value = cpu_read_memory(cpu, addr) | (cpu_read_memory(cpu, addr) << 8);
			break;
		case 2:
		default:
			value = cpu_read_memory(cpu, addr);
			break;
	}

	cpu_write_register(cpu, mod, value);
}

void i_ldr(cpu_t * cpu, u8 mod, u32 value) {
	cpu_write_register(cpu, mod, cpu_read_register(cpu, value));
}

void i_str(cpu_t * cpu, u8 mod, u32 addr) {
	u32 value = cpu_read_register(cpu, mod);

	switch (mod >> 4) {
		case 0:
			cpu_write_memory(cpu, addr, value & 0xFF);
			cpu_write_memory(cpu, addr + 1, (value >> 8) & 0xFF);
			cpu_write_memory(cpu, addr + 2, (value >> 16) & 0xFF);
			cpu_write_memory(cpu, addr + 3, (value >> 24) & 0xFF);
			return;
		case 1:
			cpu_write_memory(cpu, addr, value & 0xFF);
			cpu_write_memory(cpu, addr + 1, (value >> 8) & 0xFF);
			return;
		case 4:
			cpu_write_memory(cpu, addr, value >> 24);
			return;
		case 5:
			cpu_write_memory(cpu, addr, (value >> 16) & 0xFF);
			cpu_write_memory(cpu, addr + 1, value >> 24);
			return;
		case 8:
			cpu_write_memory(cpu, addr, (value >> 16) & 0xFF);
			return;
		case 9:
			cpu_write_memory(cpu, addr, (value >> 8) & 0xFF);
			return;
		case 2:
		default:
			cpu_write_memory(cpu, addr, value & 0xFF);
			return;
	}
}

void i_add(cpu_t * cpu, u8 mod, u32 value) {
	cpu_write_register(cpu, 0, cpu_read_register(cpu, mod) + cpu_read_register(cpu, value));
}

void i_sub(cpu_t * cpu, u8 mod, u32 value) {
	cpu_write_register(cpu, 0, cpu_read_register(cpu, mod) - cpu_read_register(cpu, value));
}

void i_mul(cpu_t * cpu, u8 mod, u32 value) {
	cpu_write_register(cpu, 0, cpu_read_register(cpu, mod) * cpu_read_register(cpu, value));
}

void i_div(cpu_t * cpu, u8 mod, u32 value) {
	cpu_write_register(cpu, 0, cpu_read_register(cpu, mod) / cpu_read_register(cpu, value));
}

void i_bop(cpu_t * cpu, u8 mod, u32 value) {
	switch (mod >> 4) {
		case 0:
			cpu_write_register(cpu, 0, cpu_read_register(cpu, mod) & cpu_read_register(cpu, value));
			return;
		case 1:
			cpu_write_register(cpu, 0, cpu_read_register(cpu, mod) | cpu_read_register(cpu, value));
			return;
		case 2:
			cpu_write_register(cpu, 0, ~cpu_read_register(cpu, mod));
			return;
		case 3:
			cpu_write_register(cpu, 0, cpu_read_register(cpu, mod) ^ cpu_read_register(cpu, value));
			return;
		case 4:
			cpu_write_register(cpu, 0, cpu_read_register(cpu, mod) << cpu_read_register(cpu, value));
			return;
		case 5:
			cpu_write_register(cpu, 0, cpu_read_register(cpu, mod) >> cpu_read_register(cpu, value));
			return;
		case 6:
			cpu_write_register(cpu, 0, cpu_rotate_left(cpu_read_register(cpu, mod), cpu_read_register(cpu, value)));
			return;
		case 7:
			cpu_write_register(cpu, 0, cpu_rotate_right(cpu_read_register(cpu, mod), cpu_read_register(cpu, value)));
			return;
		default:
			fprintf(stderr, "invalid bitwise operation option %u\n", mod >> 4);
			return;
	}
}

void i_jmp(cpu_t * cpu, u8 mod, u32 value) {
	switch (mod >> 4) {
		case 0:
			ip = reg(15);
			return;
	}
}

void i_in(cpu_t * cpu, u8 mod, u32 value) {
	
}

void i_out(cpu_t * cpu, u8 mod, u32 value) {
	
}

instruction_func instructions[0xFF] = {
	i_ldi,
	i_ldm,
	i_ldr,
	i_str,
	i_add,
	i_sub,
	i_mul,
	i_div,
	i_bop,
	i_jmp,
	i_in,
	i_out,
};

u32 cpu_run(cpu_t * cpu) {
	return reg(0);
}

void cpu_fetch(cpu_t * cpu) {
	cpu_read_bytes(cpu, ip, cpu->instruction, 6);
	ip += 6;
}

void cpu_execute(cpu_t * cpu) {
	u32 value = cpu->instruction[2] | (cpu->instruction[3] << 8) | (cpu->instruction[4] << 16) | (cpu->instruction[5] << 24);
	u8 opcode = cpu->instruction[0];
	u8 mod = cpu->instruction[1];

	instructions[opcode](cpu, mod, value);
}

cpu_t * cpu_new(u32 memory_size) {
	cpu_t * cpu = malloc(sizeof(cpu_t));
	if (cpu == NULL) {
		fprintf(stderr, "error: failed to allocate memory for cpu structure\n");
		abort();
	}

	cpu->memory = malloc(memory_size);
	if (cpu->memory == NULL) {
		fprintf(stderr, "error: failed to allocate memory for cpu virtual memory\n");
		abort();
	}

	cpu->memory_size = memory_size;
	cpu->mmio_start = memory_size - 1;

	return cpu;
}
