#include <cpu.h>
#include <common.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

u8 cpu_read_memory(cpu_t * cpu, u32 addr) {
	if (addr >= cpu->memory_size) {
		fprintf(stderr, "can't read from address %u!\n", addr);
		return 0;
	}

	return cpu->memory[addr];
}

u8 cpu_read_port(cpu_t * cpu, u16 addr) {
	return cpu->gpio[addr];
}

void cpu_write_port(cpu_t * cpu, u16 addr, u8 value) {
	cpu->gpio[addr] = value;
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
		case 1:
			if (cpu_read_register(cpu, mod) != 0) {
				ip = reg(15);
			}
			return;
		case 2:
			if (cpu_read_register(cpu, mod) == 0) {
				ip = reg(15);
			}
			return;
		case 3:
			if (cpu_read_register(cpu, mod) == cpu_read_register(cpu, value)) {
				ip = reg(15);
			}
			return;
		case 4:
			if (cpu_read_register(cpu, mod) > cpu_read_register(cpu, value)) {
				ip = reg(15);
			}
			return;
		case 5:
			if (cpu_read_register(cpu, mod) >= cpu_read_register(cpu, value)) {
				ip = reg(15);
			}
			return;
		case 6:
			if (cpu_read_register(cpu, mod) < cpu_read_register(cpu, value)) {
				ip = reg(15);
			}
			return;
		case 7:
			if (cpu_read_register(cpu, mod) <= cpu_read_register(cpu, value)) {
				ip = reg(15);
			}
			return;
		default:
			fprintf(stderr, "invalid jump operation option %u\n", mod >> 4);
			return;
	}
}

void i_in(cpu_t * cpu, u8 mod, u32 value) {
	cpu_write_register(cpu, mod, cpu_read_port(cpu, value));
}

void i_out(cpu_t * cpu, u8 mod, u32 value) {
	cpu_write_port(cpu, value, cpu_read_register(cpu, mod));
}

void i_hlt(cpu_t * cpu, u8 mod, u32 value) {
	(void) mod;
	(void) value;
	cpu->hlt = 1;
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
	i_hlt,
};

char * instr_strs[0xFF] = {
	"ldi",
	"ldm",
	"ldr",
	"str",
	"add",
	"sub",
	"mul",
	"div",
	"bop",
	"jmp",
	"in",
	"out",
	"hlt",
};

void cpu_print_instruction(cpu_t * cpu) {
	printf("instruction: %s\t(%02x%02x %02x%02x%02x%02x)\n", instr_strs[cpu->instruction[0]], cpu->instruction[0], cpu->instruction[1], cpu->instruction[2], cpu->instruction[3], cpu->instruction[4], cpu->instruction[5]);
}

void cpu_print_status(cpu_t * cpu) {
	printf("r0:  %08x\tr1:  %08x\tr2:  %08x\tr3:  %08x\nr4:  %08x\tr5:  %08x\tr6:  %08x\tr7:  %08x\nr8:  %08x\tr9:  %08x\tr10: %08x\tr11: %08x\nr12: %08x\tr13: %08x\tr14: %08x\tr15: %08x\nsp:  %08x\tip:  %08x\n\n", reg(0), reg(1), reg(2), reg(3), reg(4), reg(5), reg(6), reg(7), reg(8), reg(9), reg(10), reg(11), reg(12), reg(13), reg(14), reg(15), sp, ip);
	cpu_print_instruction(cpu);
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

u32 cpu_run(cpu_t * cpu) {
	while (cpu->hlt != 1) {
		cpu_fetch(cpu);
		cpu_execute(cpu);
		cpu_print_status(cpu);
	}

	return reg(0);
}

void cpu_load_rom(cpu_t * cpu, u8 * rom, u32 length) {
	memcpy(cpu->memory, rom, length);
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

void cpu_free(cpu_t * cpu) {
	free(cpu->memory);
	free(cpu);
}
