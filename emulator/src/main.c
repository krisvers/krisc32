#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;

typedef size_t usize;

typedef struct {
    u32 size;
    u8* data;
} memory_t;

typedef struct {
    u32 r[16];
    u32 sp;
} gp_registers_t;

typedef struct {
    u32 ip;
} protected_registers_t;

typedef struct {
    gp_registers_t gp;
    u32 sys[8];
    protected_registers_t protected;
} registers_t;

typedef enum {
    CPU_MODE_SYSTEM,
    CPU_MODE_USER,
} cpu_mode_t;

typedef struct {
    u32 vector_address;
    s32 is_issuing;
    s32 is_issuing_exception;
} interrupts_t;

typedef struct {
    registers_t regs;
    memory_t memory;
    interrupts_t interrupts;

    cpu_mode_t mode;
    s32 halt;
} cpu_t;

typedef enum {
    EXCEPTION_DIVIDE_BY_ZERO = 0x00,
    EXCEPTION_INVALID_INSTRUCTION = 0x01,
    EXCEPTION_INVALID_MEMORY = 0x02,
    EXCEPTION_UNPRIVILEDGED_INVOCATION = 0x03,
    EXCEPTION_UNPRIVILEDGED_MEMORY = 0x04,
    EXCEPTION_STACK_OVERFLOW = 0x05,
    EXCEPTION_STACK_UNDERFLOW = 0x06,
} exception_t;

#define MEMORY_SIZE 0x01000000
#define BOOT_VECTOR 0x00001000
#define FETCH_U16(data, offset) (*((u16*) &data[offset]))
#define FETCH_U32(data, offset) (*((u32*) &data[offset]))

s32 issue_opcode(cpu_t* cpu, u8 opcode);
u32* get_register(cpu_t* cpu, u8 reg);

s32 main(s32 argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <rom file>\n", argv[0]);
        return 1;
    }

    cpu_t cpu = { 0 };
    cpu.memory.size = MEMORY_SIZE;
    cpu.memory.data = (u8*) malloc(MEMORY_SIZE);
    if (cpu.memory.data == NULL) {
        printf("Failed to allocate memory for cpu memory\n");
        return 1;
    }

    memset(cpu.memory.data, 0, MEMORY_SIZE);

    {
        FILE* file = fopen(argv[1], "rb");
        if (file == NULL) {
            printf("Failed to open file: %s\n", argv[1]);
            return 1;
        }

        usize rom_size = 0;
        fseek(file, 0, SEEK_END);
        rom_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (rom_size > MEMORY_SIZE + BOOT_VECTOR) {
            printf("ROM file is too large\n");
            return 1;
        }

        if (fread(&cpu.memory.data[BOOT_VECTOR], 1, rom_size, file) != rom_size) {
            printf("Failed to read ROM file\n");
            return 1;
        }

        fclose(file);
    }
    
    cpu.regs.protected.ip = BOOT_VECTOR;
    while (!cpu.halt) {
        u8 opcode = cpu.memory.data[cpu.regs.protected.ip];
        ++cpu.regs.protected.ip;

        if (!issue_opcode(&cpu, opcode)) {
            break;
        }

        // print all registers
        printf("Registers:\n");
        for (usize i = 0; i < 16; ++i) {
            printf("R%u: 0x%08x\t", i, cpu.regs.gp.r[i]);
        }

        printf("\nSP: 0x%08x\t", cpu.regs.gp.sp);
        printf("IP: 0x%08x\n", cpu.regs.protected.ip);
        printf("Mode: %s\t", cpu.mode == CPU_MODE_SYSTEM ? "System" : "User");
        printf("Halt: %s\t", cpu.halt ? "Yes" : "No");
        printf("Next instruction: 0x%02x\n", cpu.memory.data[cpu.regs.protected.ip]);
        printf("\n");
    }

    free(cpu.memory.data);
    return 0;
}

s32 issue_exception(cpu_t* cpu, u8 type);
s32 issue_interrupt(cpu_t* cpu, u8 interrupt);
s32 push_stack(cpu_t* cpu, u32 value);
s32 pop_stack(cpu_t* cpu, u32* value);

s32 handle_ldi(cpu_t* cpu) {
    u8 reg = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32 value = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip);
    cpu->regs.protected.ip += 4;

    u32* r = get_register(cpu, reg);
    if (r == NULL) {
        return 0;
    }

    *r = value;
    return 1;
}

s32 handle_ldr(cpu_t* cpu) {
    u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r_a = get_register(cpu, reg_a);
    if (r_a == NULL) {
        return 0;
    }

    u32* r_b = get_register(cpu, reg_b);
    if (r_b == NULL) {
        return 0;
    }

    *r_a = *r_b;
    return 1;
}

s32 handle_ldm8(cpu_t* cpu) {
    u8 reg = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32 address = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip);
    cpu->regs.protected.ip += 4;
    if (address >= cpu->memory.size) {
        issue_exception(cpu, EXCEPTION_INVALID_MEMORY);
        return 0;
    }

    u8 value = cpu->memory.data[address];
    u32* r = get_register(cpu, reg);
    if (r == NULL) {
        return 0;
    }

    *r = value;
    return 1;
}

s32 handle_ldm16(cpu_t* cpu) {
    u8 reg = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32 address = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip);
    cpu->regs.protected.ip += 4;
    if (address >= cpu->memory.size - 1) {
        issue_exception(cpu, EXCEPTION_INVALID_MEMORY);
        return 0;
    }

    u16 value = FETCH_U16(cpu->memory.data, address);
    u32* r = get_register(cpu, reg);
    if (r == NULL) {
        return 0;
    }

    *r = value;
    return 1;
}

s32 handle_ldm32(cpu_t* cpu) {
    u8 reg = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32 address = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip);
    cpu->regs.protected.ip += 4;
    if (address >= cpu->memory.size - 3) {
        issue_exception(cpu, EXCEPTION_INVALID_MEMORY);
        return 0;
    }

    u32 value = FETCH_U32(cpu->memory.data, address);
    u32* r = get_register(cpu, reg);
    if (r == NULL) {
        return 0;
    }

    *r = value;
    return 1;
}

s32 handle_str8(cpu_t* cpu) {
    u8 reg = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32 address = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip);
    cpu->regs.protected.ip += 4;
    if (address >= cpu->memory.size) {
        issue_exception(cpu, EXCEPTION_INVALID_MEMORY);
        return 0;
    }

    u32* r = get_register(cpu, reg);
    if (r == NULL) {
        return 0;
    }

    cpu->memory.data[address] = *r;
    return 1;
}

s32 handle_str16(cpu_t* cpu) {
    u8 reg = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32 address = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip);
    cpu->regs.protected.ip += 4;
    if (address >= cpu->memory.size - 1) {
        issue_exception(cpu, EXCEPTION_INVALID_MEMORY);
        return 0;
    }

    u32* r = get_register(cpu, reg);
    if (r == NULL) {
        return 0;
    }

    FETCH_U16(cpu->memory.data, address) = *r;
    return 1;
}

s32 handle_str32(cpu_t* cpu) {
    u8 reg = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32 address = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip);
    cpu->regs.protected.ip += 4;
    if (address >= cpu->memory.size - 3) {
        issue_exception(cpu, EXCEPTION_INVALID_MEMORY);
        return 0;
    }

    u32* r = get_register(cpu, reg);
    if (r == NULL) {
        return 0;
    }
    
    FETCH_U32(cpu->memory.data, address) = *r;
    return 1;
}

s32 handle_add(cpu_t* cpu) {
    u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_c = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r_a = get_register(cpu, reg_a);
    if (r_a == NULL) {
        return 0;
    }

    u32* r_b = get_register(cpu, reg_b);
    if (r_b == NULL) {
        return 0;
    }

    u32* r_c = get_register(cpu, reg_c);
    if (r_c == NULL) {
        return 0;
    }

    *r_a = *r_b + *r_c;
    return 1;
}

s32 handle_sub(cpu_t* cpu) {
    u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_c = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r_a = get_register(cpu, reg_a);
    if (r_a == NULL) {
        return 0;
    }

    u32* r_b = get_register(cpu, reg_b);
    if (r_b == NULL) {
        return 0;
    }

    u32* r_c = get_register(cpu, reg_c);
    if (r_c == NULL) {
        return 0;
    }

    *r_a = *r_b - *r_c;
    return 1;
}

s32 handle_mul(cpu_t* cpu) {
    u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_c = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r_a = get_register(cpu, reg_a);
    if (r_a == NULL) {
        return 0;
    }

    u32* r_b = get_register(cpu, reg_b);
    if (r_b == NULL) {
        return 0;
    }

    u32* r_c = get_register(cpu, reg_c);
    if (r_c == NULL) {
        return 0;
    }

    *r_a = *r_b * *r_c;
    return 1;
}

s32 handle_div(cpu_t* cpu) {
    u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_c = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r_a = get_register(cpu, reg_a);
    if (r_a == NULL) {
        return 0;
    }

    u32* r_b = get_register(cpu, reg_b);
    if (r_b == NULL) {
        return 0;
    }

    u32* r_c = get_register(cpu, reg_c);
    if (r_c == NULL) {
        return 0;
    }

    if (*r_c == 0) {
        issue_exception(cpu, EXCEPTION_DIVIDE_BY_ZERO);
        return 0;
    }

    *r_a = *r_b / *r_c;
    return 1;
}

s32 handle_rem(cpu_t* cpu) {
    u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_c = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r_a = get_register(cpu, reg_a);
    if (r_a == NULL) {
        return 0;
    }

    u32* r_b = get_register(cpu, reg_b);
    if (r_b == NULL) {
        return 0;
    }

    u32* r_c = get_register(cpu, reg_c);
    if (r_c == NULL) {
        return 0;
    }

    if (*r_c == 0) {
        issue_exception(cpu, EXCEPTION_DIVIDE_BY_ZERO);
        return 0;
    }

    *r_a = *r_b % *r_c;
    return 1;
}

s32 handle_shr(cpu_t* cpu) {
    u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_c = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r_a = get_register(cpu, reg_a);
    if (r_a == NULL) {
        return 0;
    }

    u32* r_b = get_register(cpu, reg_b);
    if (r_b == NULL) {
        return 0;
    }

    u32* r_c = get_register(cpu, reg_c);
    if (r_c == NULL) {
        return 0;
    }

    *r_a = *r_b >> *r_c;
    return 1;
}

s32 handle_shl(cpu_t* cpu) {
    u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_c = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r_a = get_register(cpu, reg_a);
    if (r_a == NULL) {
        return 0;
    }

    u32* r_b = get_register(cpu, reg_b);
    if (r_b == NULL) {
        return 0;
    }

    u32* r_c = get_register(cpu, reg_c);
    if (r_c == NULL) {
        return 0;
    }

    *r_a = *r_b << *r_c;
    return 1;
}

s32 handle_and(cpu_t* cpu) {
    u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_c = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r_a = get_register(cpu, reg_a);
    if (r_a == NULL) {
        return 0;
    }

    u32* r_b = get_register(cpu, reg_b);
    if (r_b == NULL) {
        return 0;
    }

    u32* r_c = get_register(cpu, reg_c);
    if (r_c == NULL) {
        return 0;
    }

    *r_a = *r_b & *r_c;
    return 1;
}

s32 handle_or(cpu_t* cpu) {
    u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_c = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r_a = get_register(cpu, reg_a);
    if (r_a == NULL) {
        return 0;
    }

    u32* r_b = get_register(cpu, reg_b);
    if (r_b == NULL) {
        return 0;
    }

    u32* r_c = get_register(cpu, reg_c);
    if (r_c == NULL) {
        return 0;
    }

    *r_a = *r_b | *r_c;
    return 1;
}

s32 handle_not(cpu_t* cpu) {
    u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r_a = get_register(cpu, reg_a);
    if (r_a == NULL) {
        return 0;
    }

    u32* r_b = get_register(cpu, reg_b);
    if (r_b == NULL) {
        return 0;
    }

    *r_a = ~*r_b;
    return 1;
}

s32 handle_xor(cpu_t* cpu) {
    u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_c = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r_a = get_register(cpu, reg_a);
    if (r_a == NULL) {
        return 0;
    }

    u32* r_b = get_register(cpu, reg_b);
    if (r_b == NULL) {
        return 0;
    }

    u32* r_c = get_register(cpu, reg_c);
    if (r_c == NULL) {
        return 0;
    }

    *r_a = *r_b ^ *r_c;
    return 1;
}

s32 handle_jnz(cpu_t* cpu) {
    u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r_a = get_register(cpu, reg_a);
    if (r_a == NULL) {
        return 0;
    }

    u32* r_b = get_register(cpu, reg_b);
    if (r_b == NULL) {
        return 0;
    }

    if (*r_a != 0) {
        if (*r_b >= cpu->memory.size) {
            issue_exception(cpu, EXCEPTION_INVALID_MEMORY);
            return 0;
        }

        cpu->regs.protected.ip = *r_b;
    }
    return 1;
}

s32 handle_jz(cpu_t* cpu) {
    u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r_a = get_register(cpu, reg_a);
    if (r_a == NULL) {
        return 0;
    }

    u32* r_b = get_register(cpu, reg_b);
    if (r_b == NULL) {
        return 0;
    }

    if (*r_a == 0) {
        if (*r_b >= cpu->memory.size) {
            issue_exception(cpu, EXCEPTION_INVALID_MEMORY);
            return 0;
        }

        cpu->regs.protected.ip = *r_b;
    }
    return 1;
}

s32 handle_jmp(cpu_t* cpu) {
    u8 reg = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r = get_register(cpu, reg);
    if (r == NULL) {
        return 0;
    }

    if (*r >= cpu->memory.size) {
        issue_exception(cpu, EXCEPTION_INVALID_MEMORY);
        return 0;
    }

    cpu->regs.protected.ip = *r;
    return 1;
}

s32 handle_link(cpu_t* cpu) {
    u8 reg = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r = get_register(cpu, reg);
    if (r == NULL) {
        return 0;
    }

    if (*r >= cpu->memory.size) {
        issue_exception(cpu, EXCEPTION_INVALID_MEMORY);
        return 0;
    }

    if (!push_stack(cpu, cpu->regs.protected.ip)) {
        return 0;
    }

    cpu->regs.protected.ip = *r;
    return 1;
}

s32 handle_ret(cpu_t* cpu) {
    u32 address = 0;
    if (!pop_stack(cpu, &address)) {
        return 0;
    }

    if (address >= cpu->memory.size) {
        issue_exception(cpu, EXCEPTION_INVALID_MEMORY);
        return 0;
    }

    cpu->regs.protected.ip = address;
    return 1;
}

s32 handle_push(cpu_t* cpu) {
    u8 reg = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32* r = get_register(cpu, reg);
    if (r == NULL) {
        return 0;
    }

    if (!push_stack(cpu, *r)) {
        return 0;
    }

    return 1;
}

s32 handle_pop(cpu_t* cpu) {
    u8 reg = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;

    u32 value = 0;
    if (!pop_stack(cpu, &value)) {
        return 0;
    }

    u32* r = get_register(cpu, reg);
    if (r == NULL) {
        return 0;
    }

    *r = value;
    return 1;
}

s32 handle_halt(cpu_t* cpu) {
    cpu->halt = 1;
    return 1;
}

s32 handle_sys(cpu_t* cpu) {
    u8 id = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;
    switch (id) {
    case 0x00:
        cpu->regs.sys[0] = BOOT_VECTOR;
        break;
    case 0x01:
        cpu->regs.sys[0] = MEMORY_SIZE;
        break;
    case 0x02:
        cpu->regs.sys[0] = cpu->interrupts.vector_address;
        break;
    case 0x03:
        cpu->interrupts.vector_address = cpu->regs.sys[0];
        break;
    case 0x04:
        if (cpu->mode == CPU_MODE_SYSTEM) {
            cpu->mode = CPU_MODE_USER;
        } else {
            issue_exception(cpu, EXCEPTION_UNPRIVILEDGED_INVOCATION);
            return 0;
        }
        break;
    default:
        issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
        return 0;
    }

    return 1;
}

s32 handle_int(cpu_t* cpu) {
    u8 interrupt = cpu->memory.data[cpu->regs.protected.ip];
    ++cpu->regs.protected.ip;
    return issue_interrupt(cpu, interrupt);
}

typedef struct {
    s32 (*handler)(cpu_t* cpu);
} instruction_t;

instruction_t instructions[256] = {
    [0x01] = { .handler = handle_ldi },
    [0x02] = { .handler = handle_ldr },
    [0x03] = { .handler = handle_ldm8 },
    [0x04] = { .handler = handle_ldm16 },
    [0x05] = { .handler = handle_ldm32 },
    [0x06] = { .handler = handle_str8 },
    [0x07] = { .handler = handle_str16 },
    [0x08] = { .handler = handle_str32 },
    [0x09] = { .handler = handle_add },
    [0x0A] = { .handler = handle_sub },
    [0x0B] = { .handler = handle_mul },
    [0x0C] = { .handler = handle_div },
    [0x0D] = { .handler = handle_rem },
    [0x0E] = { .handler = handle_shr },
    [0x0F] = { .handler = handle_shl },
    [0x10] = { .handler = handle_and },
    [0x11] = { .handler = handle_or },
    [0x12] = { .handler = handle_not },
    [0x13] = { .handler = handle_xor },
    [0x14] = { .handler = handle_jnz },
    [0x15] = { .handler = handle_jz },
    [0x16] = { .handler = handle_jmp },
    [0x17] = { .handler = handle_link },
    [0x18] = { .handler = handle_ret },
    [0x19] = { .handler = handle_push },
    [0x1A] = { .handler = handle_pop },
    [0x60] = { .handler = handle_halt },
    [0x80] = { .handler = handle_sys },
    [0xF0] = { .handler = handle_int },
};

s32 issue_exception(cpu_t* cpu, u8 type) {
    if (cpu->interrupts.is_issuing_exception) {
        cpu->halt = 1;
        printf("Nested exception: 0x%x\n", type);
        return 0;
    }

    u32 address = FETCH_U32(cpu->memory.data, cpu->interrupts.vector_address);
    if (address == 0 || address >= cpu->memory.size) {
        cpu->halt = 1;
        printf("Unhandled exception: 0x%x\n", type);
        return 0;
    }

    push_stack(cpu, cpu->regs.protected.ip);
    cpu->regs.sys[1] = cpu->regs.protected.ip;
    cpu->regs.protected.ip = address;
    cpu->regs.sys[0] = type;
    cpu->interrupts.is_issuing_exception = 1;
    return 1;
}

s32 issue_interrupt(cpu_t* cpu, u8 interrupt) {
    if (interrupt == 0) {
        return 0;
    }

    u32 address = FETCH_U32(cpu->memory.data, cpu->interrupts.vector_address + interrupt * 4);
    if (address == 0) {
        return 1;
    }

    if (address >= cpu->memory.size) {
        issue_exception(cpu, EXCEPTION_INVALID_MEMORY);
        return 0;
    }

    push_stack(cpu, cpu->regs.protected.ip);
    cpu->regs.protected.ip = address;
    cpu->interrupts.is_issuing = 1;
    return 1;
}

s32 issue_opcode(cpu_t* cpu, u8 opcode) {
    instruction_t* inst = &instructions[opcode];
    if (inst->handler == NULL) {
        issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
        return 0;
    }

    return inst->handler(cpu);
}

s32 push_stack(cpu_t* cpu, u32 value) {
    if (cpu->regs.gp.sp < 4) {
        issue_exception(cpu, EXCEPTION_STACK_OVERFLOW);
        return 0;
    }

    cpu->regs.gp.sp -= 4;
    FETCH_U32(cpu->memory.data, cpu->regs.gp.sp) = value;
    return 1;
}

s32 pop_stack(cpu_t* cpu, u32* value) {
    if (cpu->regs.gp.sp >= MEMORY_SIZE - 4) {
        issue_exception(cpu, EXCEPTION_STACK_UNDERFLOW);
        return 0;
    }

    *value = FETCH_U32(cpu->memory.data, cpu->regs.gp.sp);
    cpu->regs.gp.sp += 4;
    return 1;
}

u32* get_register(cpu_t* cpu, u8 reg) {
    if (reg < 0x10) {
        return &cpu->regs.gp.r[reg];
    } else if (reg == 0x10) {
        return &cpu->regs.gp.sp;
    } else if (reg >= 0xF0 && reg <= 0xF7) {
        if (cpu->mode == CPU_MODE_USER) {
            issue_exception(cpu, EXCEPTION_UNPRIVILEDGED_MEMORY);
            return NULL;
        }

        return &cpu->regs.sys[reg - 0xF0];
    }

    issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
    return NULL;
}