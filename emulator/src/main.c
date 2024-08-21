#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include <SDL2/SDL.h>

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
	u32 handler_address;
	s32 is_issuing;
	s32 is_issuing_exception;
} interrupts_t;

typedef struct {
    SDL_Window* window;
    SDL_Surface* surface;
} graphical_t;

typedef struct {
    struct {
        u8 scancode;
        u8 state;
    } keystate;
} mapped_t;

typedef struct {
	registers_t regs;
	memory_t memory;
	interrupts_t interrupts;

	cpu_mode_t mode;
	s32 halt;
    
    graphical_t graphical;
    mapped_t mapped;
} cpu_t;

typedef enum {
	EXCEPTION_DIVIDE_BY_ZERO = 0x00,
	EXCEPTION_INVALID_INSTRUCTION = 0x01,
	EXCEPTION_UNPRIVILEDGED_INVOCATION = 0x03,
	EXCEPTION_UNPRIVILEDGED_MEMORY = 0x04,
	EXCEPTION_STACK_OVERFLOW = 0x05,
	EXCEPTION_STACK_UNDERFLOW = 0x06,
} exception_t;

#define MEMORY_SIZE 0x00001000
#define BOOT_VECTOR 0x00000000

#define GRAPHICAL_WIDTH 120
#define GRAPHICAL_HEIGHT 80
#define GRAPHICAL_VECTOR 0xF0000000
#define GRAPHICAL_SIZE GRAPHICAL_WIDTH * GRAPHICAL_HEIGHT
#define GRAPHICAL_SCALE 6

#define KEYINPUT_VECTOR 0xFF000000
#define KEYINPUT_SIZE 0x02

#define KEYINTERRUPT 0x01

#define FETCH_U16(data, offset) (*((u16*) &data[offset]))
#define FETCH_U32(data, offset) (*((u32*) &data[offset]))

s32 issue_opcode(cpu_t* cpu, u8 opcode);
u32* get_register(cpu_t* cpu, u8 reg);
void print_next_instruction(cpu_t* cpu);

void print_help(s32 argc, char** argv) {
	printf("Usage: %s <rom file> [options]\n", argv[0]);
	printf("Flags:\n  [-p, --print-status] [/Ps] Print the status of the processor after each instruction\n");
	printf("  [-m, --memory] [/M] Set emulator memory size (example: 12M or 100K or 9G)\n");
    printf("  [-h, --help] [/H] Print help message\n");
}

s32 is_decimal(char c) {
	return (c >= '0' && c <= '9');
}

u32 decchar_to_u32(char c) {
	return c - '0';
}

Uint32 timer_callback(Uint32 interval, void* param) {
    SDL_UpdateWindowSurface(param);
    return interval;
}

#define SCANCODE_FROM_SDL_SIMPLE(name) case SDL_SCANCODE_##name: return 'name';
#define SCANCODE_FROM_SDL(sdl, value) case SDL_SCANCODE_##sdl: return value;

s32 issue_interrupt(cpu_t* cpu, u8 interrupt);
u8 sdl_to_scancode(SDL_Scancode sc) {
    switch (sc) {
        SCANCODE_FROM_SDL_SIMPLE(A)
        SCANCODE_FROM_SDL_SIMPLE(B)
        SCANCODE_FROM_SDL_SIMPLE(C)
        SCANCODE_FROM_SDL_SIMPLE(D)
        SCANCODE_FROM_SDL_SIMPLE(E)
        SCANCODE_FROM_SDL_SIMPLE(F)
        SCANCODE_FROM_SDL_SIMPLE(G)
        SCANCODE_FROM_SDL_SIMPLE(H)
        SCANCODE_FROM_SDL_SIMPLE(I)
        SCANCODE_FROM_SDL_SIMPLE(J)
        SCANCODE_FROM_SDL_SIMPLE(K)
        SCANCODE_FROM_SDL_SIMPLE(L)
        SCANCODE_FROM_SDL_SIMPLE(M)
        SCANCODE_FROM_SDL_SIMPLE(N)
        SCANCODE_FROM_SDL_SIMPLE(O)
        SCANCODE_FROM_SDL_SIMPLE(P)
        SCANCODE_FROM_SDL_SIMPLE(Q)
        SCANCODE_FROM_SDL_SIMPLE(R)
        SCANCODE_FROM_SDL_SIMPLE(S)
        SCANCODE_FROM_SDL_SIMPLE(T)
        SCANCODE_FROM_SDL_SIMPLE(U)
        SCANCODE_FROM_SDL_SIMPLE(V)
        SCANCODE_FROM_SDL_SIMPLE(W)
        SCANCODE_FROM_SDL_SIMPLE(X)
        SCANCODE_FROM_SDL_SIMPLE(Y)
        SCANCODE_FROM_SDL_SIMPLE(Z)
            
        SCANCODE_FROM_SDL_SIMPLE(1)
        SCANCODE_FROM_SDL_SIMPLE(2)
        SCANCODE_FROM_SDL_SIMPLE(3)
        SCANCODE_FROM_SDL_SIMPLE(4)
        SCANCODE_FROM_SDL_SIMPLE(5)
        SCANCODE_FROM_SDL_SIMPLE(6)
        SCANCODE_FROM_SDL_SIMPLE(7)
        SCANCODE_FROM_SDL_SIMPLE(8)
        SCANCODE_FROM_SDL_SIMPLE(9)
        SCANCODE_FROM_SDL_SIMPLE(0)
            
        SCANCODE_FROM_SDL(RETURN, '\n')
        SCANCODE_FROM_SDL(ESCAPE, 0x80)
        SCANCODE_FROM_SDL(BACKSPACE, '\b')
        SCANCODE_FROM_SDL(TAB, '\t')
        SCANCODE_FROM_SDL(SPACE, ' ')
        SCANCODE_FROM_SDL(MINUS, '-')
        SCANCODE_FROM_SDL(EQUALS, '=')
        SCANCODE_FROM_SDL(LEFTBRACKET, '[')
        SCANCODE_FROM_SDL(RIGHTBRACKET, ']')
        SCANCODE_FROM_SDL(BACKSLASH, '\\')
        
        SCANCODE_FROM_SDL(SEMICOLON, ';')
        SCANCODE_FROM_SDL(APOSTROPHE, '\'')
        SCANCODE_FROM_SDL(GRAVE, '`')
        SCANCODE_FROM_SDL(COMMA, ',')
        SCANCODE_FROM_SDL(PERIOD, '.')
        SCANCODE_FROM_SDL(SLASH, '/')
        
        SCANCODE_FROM_SDL(HOME, 0x81)
        SCANCODE_FROM_SDL(END, 0x82)
        SCANCODE_FROM_SDL(PAGEUP, 0x83)
        SCANCODE_FROM_SDL(PAGEDOWN, 0x84)
        SCANCODE_FROM_SDL(DELETE, 0x85)
        SCANCODE_FROM_SDL(RIGHT, 0x86)
        SCANCODE_FROM_SDL(LEFT, 0x87)
        SCANCODE_FROM_SDL(DOWN, 0x88)
        SCANCODE_FROM_SDL(UP, 0x89)
        
        SCANCODE_FROM_SDL(LSHIFT, 0x8A)
        SCANCODE_FROM_SDL(LALT, 0x8B)
        SCANCODE_FROM_SDL(LCTRL, 0x8C)
        SCANCODE_FROM_SDL(RSHIFT, 0x8A)
        SCANCODE_FROM_SDL(RALT, 0x8B)
        SCANCODE_FROM_SDL(RCTRL, 0x8C)
        
        SCANCODE_FROM_SDL(F1, 0x90)
        SCANCODE_FROM_SDL(F2, 0x91)
        SCANCODE_FROM_SDL(F3, 0x92)
        SCANCODE_FROM_SDL(F4, 0x93)
        SCANCODE_FROM_SDL(F5, 0x94)
        SCANCODE_FROM_SDL(F6, 0x95)
        SCANCODE_FROM_SDL(F7, 0x96)
        SCANCODE_FROM_SDL(F8, 0x97)
        SCANCODE_FROM_SDL(F9, 0x98)
        SCANCODE_FROM_SDL(F10, 0x99)
        SCANCODE_FROM_SDL(F11, 0x9A)
        SCANCODE_FROM_SDL(F12, 0x9B)
        default: return 0x00;
    }
}

s32 main(s32 argc, char** argv) {
	s32 print_status = 0;
	char* rom_file = NULL;

	if (argc < 2) {
        print_help(argc, argv);
		return 1;
	}

    u32 memory_size = MEMORY_SIZE;
    s32 is_graphical = 0;
	for (s32 i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--print-status") == 0 || strcmp(argv[i], "/Ps") == 0) {
			print_status = 1;
		} else if ((strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--memory") == 0 || strcmp(argv[i], "/M") == 0) && i + 1 < argc) {
            u32 mult = 1;
            u32 number = 0;
            for (usize j = 0; j < strlen(argv[i + 1]); ++j) {
                char c = argv[i + 1][j];
                if (!is_decimal(c)) {
                    if (c == 'K') {
                        mult = 1000;
                    } else if (c == 'M') {
                        mult = 1000000;
                    } else if (c == 'G') {
                        mult = 1000000000;
                    } else {
			            printf("Unknown argument (%d): %s\n", i + 1, argv[i + 1]);
                        print_help(argc, argv);
            			return 1;
                    }
                    break;
                }

                number = number * 10 + decchar_to_u32(c);
            }
            
            ++i;
            memory_size = mult * number;
        } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--graphical") == 0 || strcmp(argv[i], "/G") == 0) {
            is_graphical = 1;
        } else if (rom_file == NULL) {
			rom_file = argv[i];
		} else {
			printf("Unknown argument (%d): %s\n", i, argv[i]);
            print_help(argc, argv);
			return 1;
		}
	}

    if (memory_size < 0x100) {
        printf("Memory size is required to be at least 256 bytes in headless mode. %u bytes is too small\n", memory_size);
        return 1;
    }

	if (rom_file == NULL) {
		printf("No ROM file specified\n");
        print_help(argc, argv);
		return 1;
	}

	cpu_t cpu = { 0 };
	cpu.memory.size = memory_size;
	cpu.memory.data = (u8*) malloc(memory_size);
    cpu.graphical.window = NULL;
    cpu.interrupts.handler_address = 0xFFFFFFFF;
	if (cpu.memory.data == NULL) {
		printf("Failed to allocate memory for cpu memory\n");
		return 1;
	}

	memset(cpu.memory.data, 0, memory_size);
	{
		FILE* file = fopen(rom_file, "rb");
		if (file == NULL) {
			printf("Failed to open file: %s\n", rom_file);
			return 1;
		}

		usize rom_size = 0;
		fseek(file, 0, SEEK_END);
		rom_size = ftell(file);
		fseek(file, 0, SEEK_SET);

		if (rom_size > memory_size + BOOT_VECTOR) {
			printf("ROM file is too large\n");
			return 1;
		}

		if (fread(&cpu.memory.data[BOOT_VECTOR], 1, rom_size, file) != rom_size) {
			printf("Failed to read ROM file\n");
			return 1;
		}

		fclose(file);
	}
    
    SDL_TimerID timer_id;
    if (is_graphical) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            printf("Failed to init SDL\n");
            return 1;
        }
        
        char* base = "krisc32 emulator (";
        char* title = malloc(strlen(base) + strlen(rom_file) + 2);
        if (title == NULL) {
            printf("Failed to allocate string\n");
            return 1;
        }
        
        memcpy(title, base, strlen(base));
        memcpy(title + strlen(base), rom_file, strlen(rom_file));
        title[strlen(base) + strlen(rom_file)] = ')';
        title[strlen(base) + strlen(rom_file) + 1] = '\0';
        cpu.graphical.window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, GRAPHICAL_WIDTH * GRAPHICAL_SCALE, GRAPHICAL_HEIGHT * GRAPHICAL_SCALE, 0);
        if (cpu.graphical.window == NULL) {
            printf("Failed to create window\n");
            return 1;
        }
        
        cpu.graphical.surface = SDL_GetWindowSurface(cpu.graphical.window);
        timer_id = SDL_AddTimer(16, timer_callback, cpu.graphical.window);
    }
	
	cpu.regs.protected.ip = BOOT_VECTOR;
    
    s32 closed = 0;
	while (!closed) {
        if (is_graphical) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_WINDOWEVENT_CLOSE:
                    case SDL_QUIT:
                        closed = 1;
                        break;
                    case SDL_KEYDOWN:
                    case SDL_KEYUP: {
                        u8 sc = sdl_to_scancode(event.key.keysym.scancode);
                        if (sc == 0) {
                            break;
                        }
                        
                        cpu.mapped.keystate.scancode = sc;
                        cpu.mapped.keystate.state = event.key.state == SDL_PRESSED;
                        issue_interrupt(&cpu, KEYINTERRUPT);
                        break;
                    }
                }
            }
        }
        
        if (!cpu.halt) {
            u8 opcode = cpu.memory.data[cpu.regs.protected.ip];
            ++cpu.regs.protected.ip;
            
            if (!issue_opcode(&cpu, opcode)) {
                break;
            }
            
            if (print_status) {
                printf("Processor state:\n");
                for (u16 i = 0; i < 16; ++i) {
                    if (i % 4 == 0 && i != 0) {
                        printf("\n");
                    }
                    printf("r%u: %s0x%08x    ", i, (i >= 10) ? "" : " ", cpu.regs.gp.r[i]);
                }
                
                printf("\n[ip: 0x%08x]  ", cpu.regs.protected.ip);
                printf("sp: 0x%08x\n", cpu.regs.gp.sp);
                printf("mode:              %s\n", cpu.mode == CPU_MODE_SYSTEM ? "system" : " user");
                printf("issuing interrupt?: %s\n", cpu.interrupts.is_issuing ? "yes" : " no");
                printf("issuing exception?: %s\n", cpu.interrupts.is_issuing_exception ? "yes" : " no");
                printf("halted?:            %s\n", cpu.halt ? "yes" : " no");
                print_next_instruction(&cpu);
                
                cpu.interrupts.is_issuing = 0;
                cpu.interrupts.is_issuing_exception = 0;
            }
        }
	}
    
    if (is_graphical) {
        SDL_RemoveTimer(timer_id);
        SDL_DestroyWindow(cpu.graphical.window);
        SDL_Quit();
    }

	free(cpu.memory.data);
	return 0;
}

void graphical_putpixel(cpu_t* cpu, u32 x, u32 y, u8 r, u8 g, u8 b) {
    u8* pixels = (u8*) cpu->graphical.surface->pixels;
    u16 sx = x * GRAPHICAL_SCALE;
    u16 sy = y * GRAPHICAL_SCALE;
    
    u16 sw = GRAPHICAL_WIDTH * GRAPHICAL_SCALE;
    for (u32 d = 0; d < GRAPHICAL_SCALE; ++d) {
        for (u32 c = 0; c < GRAPHICAL_SCALE; ++c) {
            u32 base = (sx + c + (sy + d) * sw) * cpu->graphical.surface->format->BytesPerPixel;
            pixels[base] = r * 36.5;
            pixels[base + 1] = g * 36.5;
            pixels[base + 2] = b * 85;
        }
    }
}

u8 graphical_getpixel(cpu_t* cpu, u32 x, u32 y) {
    u8* pixels = (u8*) cpu->graphical.surface->pixels;
    u16 sx = x * GRAPHICAL_SCALE;
    u16 sy = y * GRAPHICAL_SCALE;
    
    u16 sw = GRAPHICAL_WIDTH * GRAPHICAL_SCALE;
    
    u32 base = (sx + sy * sw) * cpu->graphical.surface->format->BytesPerPixel;
    u8 r = pixels[base] / 36.5;
    u8 g = pixels[base + 1] / 36.5;
    u8 b = pixels[base + 2] / 85;
    
    return ((r & 0x07) << 5) | ((g & 0x07) << 2) | (b & 0x03);
}

s32 issue_exception(cpu_t* cpu, u8 type);
s32 push_stack(cpu_t* cpu, u32 value);
s32 pop_stack(cpu_t* cpu, u32* value);

s32 handle_ldi(cpu_t* cpu) {
	u8 reg = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

	u32 value = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip);
	cpu->regs.protected.ip += 4;

	u32* r = get_register(cpu, reg);
	if (r == NULL) {
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
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
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
		return 0;
	}

	u32* r_b = get_register(cpu, reg_b);
	if (r_b == NULL) {
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
		return 0;
	}

	*r_a = *r_b;
	return 1;
}

s32 handle_ldm8(cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

	u32* r_a = get_register(cpu, reg_a);
	if (r_a == NULL) {
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
		return 0;
	}

	u32* r_b = get_register(cpu, reg_b);
	if (r_b == NULL) {
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
		return 0;
	}
    
    if (cpu->graphical.window != NULL && *r_b >= GRAPHICAL_VECTOR && *r_b < GRAPHICAL_VECTOR + GRAPHICAL_SIZE) {
        u8* pixels = (u8*) cpu->graphical.surface->pixels;
        u32 index = *r_b - GRAPHICAL_VECTOR;
        u16 x = index % GRAPHICAL_WIDTH;
        u16 y = index / GRAPHICAL_WIDTH;
        u8 byte = graphical_getpixel(cpu, x, y);
        *r_a = byte;
        return 1;
    }
    
    if (*r_b >= KEYINPUT_VECTOR && *r_b < KEYINPUT_VECTOR + KEYINPUT_SIZE) {
        if (*r_b == KEYINPUT_VECTOR) {
            *r_a = cpu->mapped.keystate.scancode;
        } else {
            *r_a = cpu->mapped.keystate.state;
        }
        return 1;
    }

	if (*r_b >= cpu->memory.size) {
        *r_a = 0;
        return 1;
	}

	*r_a = cpu->memory.data[*r_b];
	return 1;
}

s32 handle_ldm16(cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

	u32* r_a = get_register(cpu, reg_a);
	if (r_a == NULL) {
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
		return 0;
	}

	u32* r_b = get_register(cpu, reg_b);
	if (r_b == NULL) {
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
		return 0;
	}
    
    if (cpu->graphical.window != NULL && *r_b >= GRAPHICAL_VECTOR && *r_b < GRAPHICAL_VECTOR + GRAPHICAL_SIZE) {
        u8* pixels = (u8*) cpu->graphical.surface->pixels;
        u32 index = *r_b - GRAPHICAL_VECTOR;
        u16 x = index % GRAPHICAL_WIDTH;
        u16 y = index / GRAPHICAL_WIDTH;
        
        u8 byte1 = graphical_getpixel(cpu, x, y);
        
        ++index;
        x = index % GRAPHICAL_WIDTH;
        y = index / GRAPHICAL_WIDTH;
        
        u8 byte2 = graphical_getpixel(cpu, x, y);
        *r_a = byte1 | (byte2 << 8);
        return 1;
    }
    
    if (*r_b >= KEYINPUT_VECTOR && *r_b < KEYINPUT_VECTOR + KEYINPUT_SIZE) {
        *r_a = cpu->mapped.keystate.scancode | (cpu->mapped.keystate.state << 8);
        return 1;
    }

	if (*r_b >= cpu->memory.size) {
        *r_a = 0;
        return 1;
	}

	*r_a = FETCH_U16(cpu->memory.data, *r_b);
	return 1;
}

s32 handle_ldm32(cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

	u32* r_a = get_register(cpu, reg_a);
	if (r_a == NULL) {
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
		return 0;
	}

	u32* r_b = get_register(cpu, reg_b);
	if (r_b == NULL) {
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
		return 0;
	}
    
    if (cpu->graphical.window != NULL && *r_b >= GRAPHICAL_VECTOR && *r_b < GRAPHICAL_VECTOR + GRAPHICAL_SIZE) {
        u8* pixels = (u8*) cpu->graphical.surface->pixels;
        u32 index = *r_b - GRAPHICAL_VECTOR;
        u16 x = index % GRAPHICAL_WIDTH;
        u16 y = index / GRAPHICAL_WIDTH;
        
        u8 byte1 = graphical_getpixel(cpu, x, y);
        
        ++index;
        x = index % GRAPHICAL_WIDTH;
        y = index / GRAPHICAL_WIDTH;
        
        u8 byte2 = graphical_getpixel(cpu, x, y);
        
        ++index;
        x = index % GRAPHICAL_WIDTH;
        y = index / GRAPHICAL_WIDTH;
        
        u8 byte3 = graphical_getpixel(cpu, x, y);
        
        ++index;
        x = index % GRAPHICAL_WIDTH;
        y = index / GRAPHICAL_WIDTH;
        
        u8 byte4 = graphical_getpixel(cpu, x, y);
        *r_a = byte1 | (byte2 << 8) | (byte3 << 16) | (byte4 << 24);
        return 1;
    }

	if (*r_b >= cpu->memory.size) {
        *r_a = 0;
        return 1;
	}

	*r_a = FETCH_U32(cpu->memory.data, *r_b);
	return 1;
}

s32 handle_str8(cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

	u32* r_a = get_register(cpu, reg_a);
	if (r_a == NULL) {
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
		return 0;
	}

	u32* r_b = get_register(cpu, reg_b);
	if (r_b == NULL) {
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
		return 0;
	}
    
    if (cpu->graphical.window != NULL && *r_a >= GRAPHICAL_VECTOR && *r_a < GRAPHICAL_VECTOR + GRAPHICAL_SIZE) {
        u8* pixels = (u8*) cpu->graphical.surface->pixels;
        u8 r = (*r_b & 0xE0) >> 5;
        u8 g = (*r_b & 0x1C) >> 2;
        u8 b = *r_b & 0x03;
        u32 index = *r_a - GRAPHICAL_VECTOR;
        
        u16 x = index % GRAPHICAL_WIDTH;
        u16 y = index / GRAPHICAL_WIDTH;
        graphical_putpixel(cpu, x, y, r, g, b);
        return 1;
    }

	if (*r_a >= cpu->memory.size) {
        return 1;
	}

	cpu->memory.data[*r_a] = *r_b;
	return 1;
}

s32 handle_str16(cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

	u32* r_a = get_register(cpu, reg_a);
	if (r_a == NULL) {
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
		return 0;
	}

	u32* r_b = get_register(cpu, reg_b);
	if (r_b == NULL) {
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
		return 0;
	}
    
    if (cpu->graphical.window != NULL && *r_a >= GRAPHICAL_VECTOR && *r_a + 1 < GRAPHICAL_VECTOR + GRAPHICAL_SIZE) {
        u8* pixels = (u8*) cpu->graphical.surface->pixels;
        u8 r = (*r_b & 0xE0) >> 5;
        u8 g = (*r_b & 0x1C) >> 2;
        u8 b = *r_b & 0x03;
        
        u32 index = *r_a - GRAPHICAL_VECTOR;
        u16 x = index % GRAPHICAL_WIDTH;
        u16 y = index / GRAPHICAL_WIDTH;
        
        graphical_putpixel(cpu, x, y, r, g, b);
        
        r = (*r_b & 0xE000) >> 13;
        g = (*r_b & 0x1C00) >> 10;
        b = (*r_b & 0x0300) >> 8;
        
        ++index;
        x = index % GRAPHICAL_WIDTH;
        y = index / GRAPHICAL_WIDTH;
        
        graphical_putpixel(cpu, x, y, r, g, b);
        return 1;
    }

	if (*r_a >= cpu->memory.size) {
        return 1;
	}

	FETCH_U16(cpu->memory.data, *r_a) = *r_b;
	return 1;
}

s32 handle_str32(cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

	u32* r_a = get_register(cpu, reg_a);
	if (r_a == NULL) {
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
		return 0;
	}

	u32* r_b = get_register(cpu, reg_b);
	if (r_b == NULL) {
		issue_exception(cpu, EXCEPTION_INVALID_INSTRUCTION);
		return 0;
	}
    
    if (cpu->graphical.window != NULL && *r_a >= GRAPHICAL_VECTOR && *r_a + 3 < GRAPHICAL_VECTOR + GRAPHICAL_SIZE) {
        u8* pixels = (u8*) cpu->graphical.surface->pixels;
        
        u8 r = (*r_b & 0xE0) >> 5;
        u8 g = (*r_b & 0x1C) >> 2;
        u8 b = *r_b & 0x03;
        
        u32 index = *r_a - GRAPHICAL_VECTOR;
        u16 x = index % GRAPHICAL_WIDTH;
        u16 y = index / GRAPHICAL_WIDTH;
        
        graphical_putpixel(cpu, x, y, r, g, b);
        
        r = (*r_b & 0xE000) >> 13;
        g = (*r_b & 0x1C00) >> 10;
        b = (*r_b & 0x0300) >> 8;
        
        ++index;
        x = index % GRAPHICAL_WIDTH;
        y = index / GRAPHICAL_WIDTH;
        
        graphical_putpixel(cpu, x, y, r, g, b);
        
        r = (*r_b & 0xE00000) >> 21;
        g = (*r_b & 0x1C0000) >> 18;
        b = (*r_b & 0x030000) >> 16;
        
        ++index;
        x = index % GRAPHICAL_WIDTH;
        y = index / GRAPHICAL_WIDTH;
        
        graphical_putpixel(cpu, x, y, r, g, b);
        
        r = (*r_b & 0xE0000000) >> 29;
        g = (*r_b & 0x1C000000) >> 26;
        b = (*r_b & 0x03000000) >> 24;
        
        ++index;
        x = index % GRAPHICAL_WIDTH;
        y = index / GRAPHICAL_WIDTH;
        
        graphical_putpixel(cpu, x, y, r, g, b);
        return 1;
    }

	if (*r_a >= cpu->memory.size) {
        return 1;
	}

	FETCH_U32(cpu->memory.data, *r_a) = *r_b;
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
		return 0;
	}

	cpu->regs.protected.ip = *r;
	return 1;
}

s32 handle_jnzi(cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

    u32 addr = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip);
	cpu->regs.protected.ip += 4;

	u32* r_a = get_register(cpu, reg_a);
	if (r_a == NULL) {
		return 0;
	}

	if (*r_a != 0) {
		if (addr >= cpu->memory.size) {
			return 0;
		}

		cpu->regs.protected.ip = addr;
	}
	return 1;
}

s32 handle_jzi(cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	++cpu->regs.protected.ip;

    u32 addr = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip);
	cpu->regs.protected.ip += 4;

	u32* r_a = get_register(cpu, reg_a);
	if (r_a == NULL) {
		return 0;
	}

	if (*r_a == 0) {
		if (addr >= cpu->memory.size) {
			return 0;
		}

		cpu->regs.protected.ip = addr;
	}
	return 1;
}

s32 handle_jmpi(cpu_t* cpu) {
    u32 addr = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip);
	cpu->regs.protected.ip += 4;
    if (addr >= cpu->memory.size) {
		return 0;
	}

	cpu->regs.protected.ip = addr;
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
		cpu->regs.sys[0] = cpu->memory.size;
		break;
	case 0x02:
		cpu->regs.sys[0] = cpu->interrupts.handler_address;
		break;
	case 0x03:
		cpu->interrupts.handler_address = cpu->regs.sys[0];
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

typedef struct instruction {
	s32(*handler)(cpu_t* cpu);
	char* (*as_string)(struct instruction* inst, cpu_t* cpu);
	char* name;
} instruction_t;

char register_string_buffer[20];
char* reg_as_string(cpu_t* cpu, u8 reg) {
	if (reg < 0x10) {
		snprintf(register_string_buffer, 20, "r%u (0x%08x)", reg, *get_register(cpu, reg));
	} else if (reg == 0x10) {
		snprintf(register_string_buffer, 20, "sp (0x%08x)", *get_register(cpu, reg));
	} else if (reg >= 0xF0 && reg <= 0xF7) {
		snprintf(register_string_buffer, 20, "sys%u (0x%08x)", reg - 0xF0, *get_register(cpu, reg));
	} else {
		snprintf(register_string_buffer, 20, "[invalid register]");
	}

	return register_string_buffer;
}

char instruction_string_buffer[256];
char* ldi_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg = cpu->memory.data[cpu->regs.protected.ip];
	u32 value = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip + 1);
	sprintf(instruction_string_buffer, "%s %s, 0x%08x", inst->name, reg_as_string(cpu, reg), value);
	return instruction_string_buffer;
}

char* ldr_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* ldm8_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* ldm16_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* ldm32_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* str8_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* str16_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* str32_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* add_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	u8 reg_c = cpu->memory.data[cpu->regs.protected.ip + 2];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	len += sprintf(&instruction_string_buffer[len], "%s, ", reg_as_string(cpu, reg_b));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_c));
	return instruction_string_buffer;
}

char* sub_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	u8 reg_c = cpu->memory.data[cpu->regs.protected.ip + 2];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	len += sprintf(&instruction_string_buffer[len], "%s, ", reg_as_string(cpu, reg_b));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_c));
	return instruction_string_buffer;
}

char* mul_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	u8 reg_c = cpu->memory.data[cpu->regs.protected.ip + 2];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	len += sprintf(&instruction_string_buffer[len], "%s, ", reg_as_string(cpu, reg_b));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* div_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	u8 reg_c = cpu->memory.data[cpu->regs.protected.ip + 2];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	len += sprintf(&instruction_string_buffer[len], "%s, ", reg_as_string(cpu, reg_b));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* rem_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	u8 reg_c = cpu->memory.data[cpu->regs.protected.ip + 2];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	len += sprintf(&instruction_string_buffer[len], "%s, ", reg_as_string(cpu, reg_b));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* shr_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	u8 reg_c = cpu->memory.data[cpu->regs.protected.ip + 2];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	len += sprintf(&instruction_string_buffer[len], "%s, ", reg_as_string(cpu, reg_b));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* shl_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	u8 reg_c = cpu->memory.data[cpu->regs.protected.ip + 2];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	len += sprintf(&instruction_string_buffer[len], "%s, ", reg_as_string(cpu, reg_b));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* and_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	u8 reg_c = cpu->memory.data[cpu->regs.protected.ip + 2];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	len += sprintf(&instruction_string_buffer[len], "%s, ", reg_as_string(cpu, reg_b));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* or_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	u8 reg_c = cpu->memory.data[cpu->regs.protected.ip + 2];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	len += sprintf(&instruction_string_buffer[len], "%s, ", reg_as_string(cpu, reg_b));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* not_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* xor_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	u8 reg_c = cpu->memory.data[cpu->regs.protected.ip + 2];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	len += sprintf(&instruction_string_buffer[len], "%s, ", reg_as_string(cpu, reg_b));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* jnz_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* jz_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	u8 reg_b = cpu->memory.data[cpu->regs.protected.ip + 1];
	int len = sprintf(instruction_string_buffer, "%s %s, ", inst->name, reg_as_string(cpu, reg_a));
	sprintf(&instruction_string_buffer[len], "%s", reg_as_string(cpu, reg_b));
	return instruction_string_buffer;
}

char* jmp_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
	sprintf(instruction_string_buffer, "%s, %s", inst->name, reg_as_string(cpu, reg_a));
	return instruction_string_buffer;
}

char* jnzi_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    u32 addr = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip + 1);
	int len = sprintf(instruction_string_buffer, "%s %s, 0x%08x", inst->name, reg_as_string(cpu, reg_a), addr);
	return instruction_string_buffer;
}

char* jzi_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg_a = cpu->memory.data[cpu->regs.protected.ip];
    u32 addr = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip + 1);
	int len = sprintf(instruction_string_buffer, "%s %s, 0x%08x", inst->name, reg_as_string(cpu, reg_a), addr);
	return instruction_string_buffer;
}

char* jmpi_string(instruction_t* inst, cpu_t* cpu) {
    u32 addr = FETCH_U32(cpu->memory.data, cpu->regs.protected.ip);
	sprintf(instruction_string_buffer, "%s 0x%08x", inst->name, addr);
	return instruction_string_buffer;
}

char* link_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg = cpu->memory.data[cpu->regs.protected.ip];
	sprintf(instruction_string_buffer, "%s %s", inst->name, reg_as_string(cpu, reg));
	return instruction_string_buffer;
}

char* ret_string(instruction_t* inst, cpu_t* cpu) {
	return inst->name;
}

char* push_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg = cpu->memory.data[cpu->regs.protected.ip];
	sprintf(instruction_string_buffer, "%s %s", inst->name, reg_as_string(cpu, reg));
	return instruction_string_buffer;
}

char* pop_string(instruction_t* inst, cpu_t* cpu) {
	u8 reg = cpu->memory.data[cpu->regs.protected.ip];
	sprintf(instruction_string_buffer, "%s %s", inst->name, reg_as_string(cpu, reg));
	return instruction_string_buffer;
}

char* halt_string(instruction_t* inst, cpu_t* cpu) {
	return inst->name;
}

char* sys_string(instruction_t* inst, cpu_t* cpu) {
	u8 id = cpu->memory.data[cpu->regs.protected.ip];
	sprintf(instruction_string_buffer, "%s %u", inst->name, id);
	return instruction_string_buffer;
}

char* int_string(instruction_t* inst, cpu_t* cpu) {
	u8 interrupt = cpu->memory.data[cpu->regs.protected.ip];
	sprintf(instruction_string_buffer, "%s %u", inst->name, interrupt);
	return instruction_string_buffer;
}

instruction_t instructions[256] = {
	[0x01] = { .handler = handle_ldi, .as_string = ldi_string, .name = "ldi"},
	[0x02] = {.handler = handle_ldr, .as_string = ldr_string, .name = "ldr" },
	[0x03] = {.handler = handle_ldm8, .as_string = ldm8_string, .name = "ldm8" },
	[0x04] = {.handler = handle_ldm16, .as_string = ldm16_string, .name = "ldm16" },
	[0x05] = {.handler = handle_ldm32, .as_string = ldm32_string, .name = "ldm32" },

	[0x06] = {.handler = handle_str8, .as_string = str8_string, .name = "str8" },
	[0x07] = {.handler = handle_str16, .as_string = str16_string, .name = "str16" },
	[0x08] = {.handler = handle_str32, .as_string = str32_string, .name = "str32" },

	[0x09] = {.handler = handle_add, .as_string = add_string, .name = "add" },
	[0x0A] = {.handler = handle_sub, .as_string = sub_string, .name = "sub" },
	[0x0B] = {.handler = handle_mul, .as_string = mul_string, .name = "mul" },
	[0x0C] = {.handler = handle_div, .as_string = div_string, .name = "div" },
	[0x0D] = {.handler = handle_rem, .as_string = rem_string, .name = "rem" },

	[0x0E] = {.handler = handle_shr, .as_string = shr_string, .name = "shr" },
	[0x0F] = {.handler = handle_shl, .as_string = shl_string, .name = "shl" },
	[0x10] = {.handler = handle_and, .as_string = and_string,  .name = "and" },
	[0x11] = {.handler = handle_or, .as_string = or_string,  .name = "or" },
	[0x12] = {.handler = handle_not, .as_string = not_string,  .name = "not" },
	[0x13] = {.handler = handle_xor, .as_string = xor_string,  .name = "xor" },

	[0x14] = {.handler = handle_jnz, .as_string = jnz_string, .name = "jnz" },
	[0x15] = {.handler = handle_jz, .as_string = jz_string, .name = "jz" },
	[0x16] = {.handler = handle_jmp, .as_string = jmp_string, .name = "jmp" },
	[0x17] = {.handler = handle_link, .as_string = link_string, .name = "link" },
	[0x18] = {.handler = handle_ret, .as_string = ret_string, .name = "ret" },

	[0x19] = {.handler = handle_push, .as_string = push_string, .name = "push" },
	[0x1A] = {.handler = handle_pop, .as_string = pop_string, .name = "pop" },

	[0x40] = {.handler = handle_jnzi, .as_string = jnzi_string, .name = "jnzi" },
	[0x41] = {.handler = handle_jzi, .as_string = jzi_string, .name = "jzi" },
	[0x42] = {.handler = handle_jmpi, .as_string = jmpi_string, .name = "jmpi" },

	[0x60] = {.handler = handle_halt, .as_string = halt_string, .name = "halt" },
	[0x80] = {.handler = handle_sys, .as_string = sys_string, .name = "sys" },
	[0xF0] = {.handler = handle_int, .as_string = int_string, .name = "int" },
};

s32 issue_exception(cpu_t* cpu, u8 type) {
	if (cpu->interrupts.is_issuing_exception) {
		cpu->halt = 1;
		printf("Nested exception: 0x%02x\n", type);
		return 0;
	}

	u32 address = cpu->interrupts.handler_address;
	if (address >= cpu->memory.size) {
		cpu->halt = 1;
		printf("Unhandled exception: 0x%02x\n", type);
		return 0;
	}
	printf("Issuing exception: 0x%02x\n", type);

	cpu->regs.sys[7] = 0;
	push_stack(cpu, cpu->regs.protected.ip);
	cpu->regs.sys[0] = type;
	cpu->regs.sys[1] = cpu->regs.protected.ip;
	cpu->regs.protected.ip = address;
	cpu->interrupts.is_issuing_exception = 1;
	return 1;
}

s32 issue_interrupt(cpu_t* cpu, u8 interrupt) {
	if (interrupt == 0) {
		return 0;
	}

	u32 address = cpu->interrupts.handler_address;
	if (address >= cpu->memory.size) {
		return 1;
	}

	cpu->regs.sys[7] = interrupt;
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
	if (cpu->regs.gp.sp >= cpu->memory.size - 4) {
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

void print_next_instruction(cpu_t* cpu) {
	u8 next_opcode = cpu->memory.data[cpu->regs.protected.ip++];
	instruction_t* inst = &instructions[next_opcode];
	if (inst->handler != NULL) {
		printf("Next instruction: %s\n", inst->as_string(inst, cpu));
	} else {
		printf("Next instruction: [invalid instruction] (0x%02x)\n", next_opcode);
	}
	--cpu->regs.protected.ip;
}
