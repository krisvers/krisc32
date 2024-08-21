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

typedef enum {
	OPERAND_TYPE_NONE,
	OPERAND_TYPE_REGISTER,
	OPERAND_TYPE_IMMEDIATE,
	OPERAND_TYPE_IMM8,
} operand_type_t;

typedef struct {
	operand_type_t operand_types[3];
	u32 operand_size;
} instruction_type_info_t;

typedef enum {
	INSTRUCTION_TYPE_NO_OPERAND = 0,
	INSTRUCTION_TYPE_1_REGISTER = 1,
	INSTRUCTION_TYPE_2_REGISTER = 2,
	INSTRUCTION_TYPE_3_REGISTER = 3,
	INSTRUCTION_TYPE_1_REGISTER_1_IMMEDIATE = 4,
	INSTRUCTION_TYPE_1_IMMEDIATE = 5,
	INSTRUCTION_TYPE_SYSTEM = 6,
	INSTRUCTION_TYPE_COUNT,
} instruction_type_t;

instruction_type_info_t instruction_types[INSTRUCTION_TYPE_COUNT] = {
	[INSTRUCTION_TYPE_NO_OPERAND] = {.operand_types = { OPERAND_TYPE_NONE }, .operand_size = 0 },
	[INSTRUCTION_TYPE_1_REGISTER] = {.operand_types = { OPERAND_TYPE_REGISTER, OPERAND_TYPE_NONE }, .operand_size = 1 },
	[INSTRUCTION_TYPE_2_REGISTER] = {.operand_types = { OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, OPERAND_TYPE_NONE }, .operand_size = 2 },
	[INSTRUCTION_TYPE_3_REGISTER] = {.operand_types = { OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, }, .operand_size = 3 },
	[INSTRUCTION_TYPE_1_REGISTER_1_IMMEDIATE] = {.operand_types = { OPERAND_TYPE_REGISTER, OPERAND_TYPE_IMMEDIATE, OPERAND_TYPE_NONE }, .operand_size = 5 },
	[INSTRUCTION_TYPE_1_IMMEDIATE] = {.operand_types = { OPERAND_TYPE_IMMEDIATE, OPERAND_TYPE_NONE }, .operand_size = 4 },
	[INSTRUCTION_TYPE_SYSTEM] = {.operand_types = { OPERAND_TYPE_IMM8, OPERAND_TYPE_NONE }, .operand_size = 1 },
};

typedef struct {
	char* name;
	usize hash;
	instruction_type_t type;
	u8 operand_count;
	u8 opcode;
} instruction_t;

instruction_t instructions[] = {
	{.name = "ldi", .type = INSTRUCTION_TYPE_1_REGISTER_1_IMMEDIATE, .operand_count = 2, .opcode = 0x01 },
	{.name = "ldr", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2, .opcode = 0x02 },
	{.name = "ldm8", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2, .opcode = 0x03 },
	{.name = "ldm16", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2, .opcode = 0x04 },
	{.name = "ldm32", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2, .opcode = 0x05 },
	{.name = "str8", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2, .opcode = 0x06 },
	{.name = "str16", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2, .opcode = 0x07 },
	{.name = "str32", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2, .opcode = 0x08 },
	{.name = "add", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3, .opcode = 0x09 },
	{.name = "sub", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3, .opcode = 0x0A },
	{.name = "mul", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3, .opcode = 0x0B },
	{.name = "div", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3, .opcode = 0x0C },
	{.name = "rem", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3, .opcode = 0x0D },
	{.name = "shr", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3, .opcode = 0x0E },
	{.name = "shl", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3, .opcode = 0x0F },
	{.name = "and", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3, .opcode = 0x10 },
	{.name = "or", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3, .opcode = 0x11 },
	{.name = "not", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2, .opcode = 0x12 },
	{.name = "xor", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3, .opcode = 0x13 },
	{.name = "jnz", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2, .opcode = 0x14 },
	{.name = "jz", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2, .opcode = 0x15 },
	{.name = "jmp", .type = INSTRUCTION_TYPE_1_REGISTER, .operand_count = 1, .opcode = 0x16 },
	{.name = "jnzi", .type = INSTRUCTION_TYPE_1_REGISTER_1_IMMEDIATE, .operand_count = 2, .opcode = 0x40 },
	{.name = "jzi", .type = INSTRUCTION_TYPE_1_REGISTER_1_IMMEDIATE, .operand_count = 2, .opcode = 0x41 },
	{.name = "jmpi", .type = INSTRUCTION_TYPE_1_IMMEDIATE, .operand_count = 1, .opcode = 0x42 },
	{.name = "link", .type = INSTRUCTION_TYPE_1_REGISTER, .operand_count = 1, .opcode = 0x17 },
	{.name = "ret", .type = INSTRUCTION_TYPE_NO_OPERAND, .operand_count = 0, .opcode = 0x18 },
	{.name = "push", .type = INSTRUCTION_TYPE_1_REGISTER, .operand_count = 1, .opcode = 0x19 },
	{.name = "pop", .type = INSTRUCTION_TYPE_1_REGISTER, .operand_count = 1, .opcode = 0x1A },
	{.name = "hlt", .type = INSTRUCTION_TYPE_NO_OPERAND, .operand_count = 0, .opcode = 0x60 },
	{.name = "sys", .type = INSTRUCTION_TYPE_SYSTEM, .operand_count = 1, .opcode = 0x80 },
	{.name = "int", .type = INSTRUCTION_TYPE_SYSTEM, .operand_count = 1, .opcode = 0xF0 },
};

typedef enum {
	PROTECTION_SYSTEM_MODE,
	PROTECTION_ALL_MODES,
} protection_t;

typedef struct {
	char* name;
	usize hash;
	protection_t protection;
	u8 id;
} reg_t;

reg_t registers[] = {
	{.name = "r0", .protection = PROTECTION_ALL_MODES, .id = 0x00 },
	{.name = "r1", .protection = PROTECTION_ALL_MODES, .id = 0x01 },
	{.name = "r2", .protection = PROTECTION_ALL_MODES, .id = 0x02 },
	{.name = "r3", .protection = PROTECTION_ALL_MODES, .id = 0x03 },
	{.name = "r4", .protection = PROTECTION_ALL_MODES, .id = 0x04 },
	{.name = "r5", .protection = PROTECTION_ALL_MODES, .id = 0x05 },
	{.name = "r6", .protection = PROTECTION_ALL_MODES, .id = 0x06 },
	{.name = "r7", .protection = PROTECTION_ALL_MODES, .id = 0x07 },
	{.name = "r8", .protection = PROTECTION_ALL_MODES, .id = 0x08 },
	{.name = "r9", .protection = PROTECTION_ALL_MODES, .id = 0x09 },
	{.name = "r10", .protection = PROTECTION_ALL_MODES, .id = 0x0A },
	{.name = "r11", .protection = PROTECTION_ALL_MODES, .id = 0x0B },
	{.name = "r12", .protection = PROTECTION_ALL_MODES, .id = 0x0C },
	{.name = "r13", .protection = PROTECTION_ALL_MODES, .id = 0x0D },
	{.name = "r14", .protection = PROTECTION_ALL_MODES, .id = 0x0E },
	{.name = "r15", .protection = PROTECTION_ALL_MODES, .id = 0x0F },
	{.name = "sp", .protection = PROTECTION_ALL_MODES, .id = 0x10 },
	{.name = "sys0", .protection = PROTECTION_SYSTEM_MODE, .id = 0xF0 },
	{.name = "sys1", .protection = PROTECTION_SYSTEM_MODE, .id = 0xF1 },
	{.name = "sys2", .protection = PROTECTION_SYSTEM_MODE, .id = 0xF2 },
	{.name = "sys3", .protection = PROTECTION_SYSTEM_MODE, .id = 0xF3 },
	{.name = "sys4", .protection = PROTECTION_SYSTEM_MODE, .id = 0xF4 },
	{.name = "sys5", .protection = PROTECTION_SYSTEM_MODE, .id = 0xF5 },
	{.name = "sys6", .protection = PROTECTION_SYSTEM_MODE, .id = 0xF6 },
	{.name = "sys7", .protection = PROTECTION_SYSTEM_MODE, .id = 0xF7 },
};

typedef struct {
	char* start;
	usize length;
} word_t;

typedef struct {
	word_t name;
	usize hash;
	s32 is_defined;
} section_t;

typedef struct {
	word_t name;
	usize hash;
	u32 address;
	usize section;
} label_t;

typedef struct {
	word_t word;
	union {
		reg_t* reg;
		u32 immediate;
		u8 imm8;
		label_t* label;
	};
	s32 is_label;
} operand_t;

section_t text_section = { .name = {.start = ".text", .length = 5 }, .hash = 0, .is_defined = 0 };

typedef enum {
	DEFINE_TYPE_NONE,
	DEFINE_TYPE_8,
	DEFINE_TYPE_16,
	DEFINE_TYPE_32,
	DEFINE_TYPE_64,
} define_type_t;

typedef struct {
	union {
		instruction_t* instruction;
		u8 def8;
		u16 def16;
		u32 def32;
		u64 def64;
	};
	define_type_t define_type;

	operand_t operands[3];
	u8 current_count;
	u32 address;
	usize section;
} line_t;

typedef struct {
	line_t* lines;
	usize line_count;
	u32 current_address;
	usize current_section;

	label_t* labels;
	usize label_count;

	section_t* sections;
	usize section_count;
	usize private_section_count;
} assembler_t;

char* word_cstring(word_t* word);
usize hash_string(char* string) {
	usize hash = 0;
	while (*string != '\0') {
		hash = (hash * 31) + *string;
		string++;
	}

	return hash;
}

usize hash_word(word_t* word) {
	usize hash = 0;
	for (usize i = 0; i < word->length; ++i) {
		hash = (hash * 31) + word->start[i];
	}

	return hash;
}

s32 retrieve_word(char* buffer, usize size, word_t* word) {
	if (buffer[0] == '\0') {
		return 0;
	}

	if (size == 0) {
		return 0;
	}

	usize end = 0;
	while (end < size && buffer[end] != '\0' && buffer[end] != ' ' && buffer[end] != '\t' && buffer[end] != '\n' && buffer[end] != '\r') {
		++end;
	}

	word->start = buffer;
	word->length = end;
	return 1;
}

s32 is_decimal(char c) {
	return c >= '0' && c <= '9';
}

s32 is_lower_hex(char c) {
	return c >= 'a' && c <= 'f';
}

s32 is_capital_hex(char c) {
	return c >= 'A' && c <= 'F';
}

s32 is_hex(char c) {
	return is_decimal(c) || is_lower_hex(c) || is_capital_hex(c);
}

s32 is_alpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

u32 hexchar_to_u32(char c) {
	char base = '0';
	u32 offset = 0;
	if (is_lower_hex(c)) {
		base = 'a';
		offset = 10;
	} else if (is_capital_hex(c)) {
		base = 'A';
		offset = 10;
	}

	return c - base + offset;
}

u32 decchar_to_u32(char c) {
	return c - '0';
}

s32 process_word(assembler_t* assembler, word_t* word) {
	if (word->start[word->length - 1] == ':') {
		--word->length;
		usize hash = hash_word(word);
		u32 offset = 0;
		if (assembler->line_count != 0) {
            line_t* line = &assembler->lines[assembler->line_count - 1];
            if (line->define_type == DEFINE_TYPE_NONE) {
                offset = instruction_types[line->instruction->type].operand_size + 1;
            } else {
                offset = (line->define_type == DEFINE_TYPE_64) ? 8 : (line->define_type == DEFINE_TYPE_32) ? 4 : (line->define_type == DEFINE_TYPE_16) ? 2 : 1;
            }
		}

		label_t label = { .name = *word, .hash = hash, .address = assembler->current_address + offset, .section = assembler->current_section };
		if (assembler->labels == NULL) {
			assembler->labels = (label_t*) malloc(sizeof(label_t));
			if (assembler->labels == NULL) {
				printf("Failed to allocate memory\n");
				return 0;
			}

			assembler->label_count = 0;
		} else {
			void* p = realloc(assembler->labels, sizeof(label_t) * (assembler->label_count + 1));
			if (p == NULL) {
				printf("Failed to allocate memory\n");
				return 0;
			}

			assembler->labels = (label_t*) p;
		}

		assembler->labels[assembler->label_count] = label;
		assembler->label_count++;
		return 1;
	} else if (word->start[0] == '.') {
		usize hash = hash_word(word);
		if (hash == hash_string(".shstrtab")) {
			printf("'%s' is a reserved section name\n", word_cstring(word));
			return 0;
		}

		for (usize i = 0; i < assembler->section_count; ++i) {
			if (assembler->sections[i].hash == hash) {
				if (assembler->sections[i].is_defined) {
					printf("'%s' defined twice. Separated section definitions are currently unsupported", word_cstring(word));
					return 0;
				}

				assembler->current_section = i;
				assembler->sections[i].is_defined = 1;
				return 1;
			}
		}

		section_t section = { .name = *word, .hash = hash, .is_defined = 1 };
		if (assembler->sections == NULL) {
			assembler->sections = (section_t*) malloc(sizeof(section_t));
			if (assembler->sections == NULL) {
				printf("Failed to allocate memory\n");
				return 0;
			}

			assembler->section_count = 0;
		} else {
			void* p = realloc(assembler->sections, sizeof(section_t) * (assembler->section_count + 1));
			if (p == NULL) {
				printf("Failed to allocate memory\n");
				return 0;
			}

			assembler->sections = (section_t*) p;
		}

		assembler->sections[assembler->section_count] = section;
		assembler->current_section = assembler->section_count;
		++assembler->section_count;
		return 1;
	} else if (word->start[0] == '=') {
		if (word->length < 2) {
			printf("Unexpected sequence '%s'\n", word_cstring(word));
			return 0;
		}

		u32(*char_to_u32)(char) = decchar_to_u32;
		u32 radix = 10;
		u32 start = 1;
		if (word->length > 2 && word->start[1] == '0' && word->start[2] == 'x') {
			char_to_u32 = hexchar_to_u32;
			radix = 16;
			start = 3;
		}

		u64 value = 0;
		for (usize i = start; i < word->length; ++i) {
			value = (value * radix) + char_to_u32(word->start[i]);
		}

		usize len = word->length - start;
		line_t line = { .define_type = DEFINE_TYPE_32, .def64 = value, .current_count = 0, .address = assembler->current_address, .section = assembler->current_section };
		if (radix == 10) {
			if (len > 10 || value > 0xFFFFFFFF) {
				if (len > 20) {
					printf("Warning: '%s': larger than 64-bits, the value will be truncated", word_cstring(word));
				}
				line.define_type = DEFINE_TYPE_64;
				assembler->current_address += 8;
			} else if (len > 5 || value > 0xFFFF) {
				line.define_type = DEFINE_TYPE_32;
				assembler->current_address += 4;
			} else if (len > 3 || value > 0xFF) {
				line.define_type = DEFINE_TYPE_16;
				assembler->current_address += 2;
			} else {
				line.define_type = DEFINE_TYPE_8;
				++assembler->current_address;
			}
		} else if (radix == 16) {
			if (len > 8) {
				if (len > 16) {
					printf("Warning: '%s': larger than 64-bits, the value will be truncated", word_cstring(word));
				}
				line.define_type = DEFINE_TYPE_64;
				assembler->current_address += 8;
			} else if (len > 4) {
				line.define_type = DEFINE_TYPE_32;
				assembler->current_address += 4;
			} else if (len > 2) {
				line.define_type = DEFINE_TYPE_16;
				assembler->current_address += 2;
			} else {
				line.define_type = DEFINE_TYPE_8;
				++assembler->current_address;
			}
		}

		if (assembler->lines == NULL) {
			assembler->lines = (line_t*) malloc(sizeof(line_t));
			if (assembler->lines == NULL) {
				printf("Failed to allocate memory\n");
				return 0;
			}

			assembler->line_count = 0;
		} else {
			void* p = realloc(assembler->lines, sizeof(line_t) * (assembler->line_count + 1));
			if (p == NULL) {
				printf("Failed to allocate memory\n");
				return 0;
			}

			assembler->lines = (line_t*) p;
		}

		assembler->lines[assembler->line_count] = line;
		++assembler->line_count;
		return 1;
	}

	line_t line = (line_t){ .define_type = DEFINE_TYPE_NONE, .instruction = NULL, .current_count = 0, .address = assembler->current_address, };
	if (assembler->line_count != 0) {
		line_t* l = &assembler->lines[assembler->line_count - 1];
		if (l->define_type == DEFINE_TYPE_NONE) {
			line = *l;
		}
	}

	usize hash = hash_word(word);
	for (u16 i = 0; i < sizeof(instructions) / sizeof(instruction_t); ++i) {
		if (instructions[i].hash == hash) {
			line.section = assembler->current_section;
			if (line.define_type == DEFINE_TYPE_NONE && line.instruction != NULL && line.current_count == line.instruction->operand_count) {
				assembler->current_address += instruction_types[line.instruction->type].operand_size + 1;
				line.instruction = &instructions[i];
				line.address = assembler->current_address;
				line.current_count = 0;

				void* p = realloc(assembler->lines, sizeof(line_t) * (assembler->line_count + 1));
				if (p == NULL) {
					printf("Failed to allocate memory\n");
					return 0;
				}

				assembler->lines = (line_t*) p;
				assembler->lines[assembler->line_count] = line;
				++assembler->line_count;
				return 1;
			}

			line.instruction = &instructions[i];
			if (assembler->lines == NULL) {
				assembler->lines = (line_t*) malloc(sizeof(line_t));
				if (assembler->lines == NULL) {
					printf("Failed to allocate memory\n");
					return 0;
				}

				assembler->line_count = 0;
			} else {
				void* p = realloc(assembler->lines, sizeof(line_t) * (assembler->line_count + 1));
				if (p == NULL) {
					printf("Failed to allocate memory\n");
					return 0;
				}

				assembler->lines = (line_t*) p;
			}

			assembler->lines[assembler->line_count] = line;
			++assembler->line_count;
			return 1;
		}
	}

	if (line.instruction == NULL) {
		printf("No instruction found prior to sequence '%s'\n", word_cstring(word));
		return 0;
	}

	if (line.current_count >= line.instruction->operand_count) {
        printf("Instruction '%s' already has %u operands; unexpected sequence '%s' (line %zu)\n", line.instruction->name, line.instruction->operand_count, word_cstring(word), assembler->line_count);
		return 0;
	}

	operand_t operand = { .word = *word };
	switch (line.instruction->type) {
		case INSTRUCTION_TYPE_NO_OPERAND:
			printf("Instruction '%s' does not take any operands; unexpected sequence '%s'\n", line.instruction->name, word_cstring(word));
			return 0;
		case INSTRUCTION_TYPE_1_REGISTER:
		case INSTRUCTION_TYPE_2_REGISTER:
		case INSTRUCTION_TYPE_3_REGISTER:
			for (u16 i = 0; i < sizeof(registers) / sizeof(reg_t); ++i) {
				if (registers[i].hash == hash) {
					operand.reg = &registers[i];
					break;
				}
			}

			if (operand.reg == NULL) {
				printf("No register found for sequence '%s'\n", word_cstring(word));
				return 0;
			}
			break;
		case INSTRUCTION_TYPE_1_REGISTER_1_IMMEDIATE:
			if (line.current_count == 0) {
				for (u16 i = 0; i < sizeof(registers) / sizeof(reg_t); ++i) {
					if (registers[i].hash == hash) {
						operand.reg = &registers[i];
						break;
					}
				}

				if (operand.reg == NULL) {
					printf("No register found for sequence '%s'\n", word_cstring(word));
					return 0;
				}
			} else {
				if (is_alpha(word->start[0])) {
					operand.is_label = 1;
				} else if (word->start[0] == '\'' && word->start[word->length - 1] == '\'' && word->length == 3) {
                    operand.immediate = word->start[1];
                } else {
					u32(*char_to_u32)(char) = decchar_to_u32;
					u32 radix = 10;
					u32 start = 0;
					if (word->length > 2 && word->start[0] == '0' && word->start[1] == 'x') {
						char_to_u32 = hexchar_to_u32;
						radix = 16;
						start = 2;
					}

					operand.immediate = 0;
					for (usize i = start; i < word->length; ++i) {
						operand.immediate = (operand.immediate * radix) + char_to_u32(word->start[i]);
					}
				}
			}
			break;
        case INSTRUCTION_TYPE_1_IMMEDIATE:
            if (is_alpha(word->start[0])) {
				operand.is_label = 1;
			} else {
				u32(*char_to_u32)(char) = decchar_to_u32;
				u32 radix = 10;
				u32 start = 0;
				if (word->length > 2 && word->start[0] == '0' && word->start[1] == 'x') {
					char_to_u32 = hexchar_to_u32;
					radix = 16;
					start = 2;
				}

				operand.immediate = 0;
				for (usize i = start; i < word->length; ++i) {
					operand.immediate = (operand.immediate * radix) + char_to_u32(word->start[i]);
				}
			}
            break;
		case INSTRUCTION_TYPE_SYSTEM: {
			u32(*char_to_u32)(char) = decchar_to_u32;
			u32 radix = 10;
			u32 start = 0;
			if (word->length > 2 && word->start[0] == '0' && word->start[1] == 'x') {
				char_to_u32 = hexchar_to_u32;
				radix = 16;
				start = 2;
			}

			operand.imm8 = 0;
			for (usize i = start; i < word->length; ++i) {
				operand.imm8 = (operand.imm8 * radix) + char_to_u32(word->start[i]);
			}
		} break;
	}

	line.operands[line.current_count] = operand;
	++line.current_count;
	assembler->lines[assembler->line_count - 1] = line;
	return 1;
}

s32 evaluate_labels(assembler_t* assembler) {
	for (usize i = 0; i < assembler->line_count; ++i) {
		line_t* line = &assembler->lines[i];
		if (line->define_type != DEFINE_TYPE_NONE) {
			continue;
		}

		for (usize j = 0; j < line->instruction->operand_count; ++j) {
			if (line->operands[j].is_label) {
				usize hash = hash_word(&line->operands[j].word);
				for (usize k = 0; k < assembler->label_count; ++k) {
					if (assembler->labels[k].hash == hash) {
						line->operands[j].label = &assembler->labels[k];
						break;
					}
				}

				if (line->operands[j].label == NULL) {
					printf("Undefined label '%s'\n", word_cstring(&line->operands[j].word));
					return 0;
				}
			}
		}
	}

	return 1;
}

typedef struct {
	char magic[4]; /* ELF + 0x7F */
	u8 class; /* 0x01 (32-bit) */
	u8 data; /* 0x01 (little-endian) */
	u8 version; /* 0x01 (elf version) */
	u8 abi; /* 0x6B (kr32 abi) */
	u8 abiversion; /* 0x00 */
	u8 reserved[7]; /* all zero */
} elf_ident_t;

typedef struct {
	elf_ident_t ident;
	u16 type; /* 0x02 executable */
	u16 machine; /* 0x726B (kr32 machine) */
	u32 version; /* 0x01 */
	u32 entry;
	u32 phoffset;
	u32 shoffset;
	u32 flags;
	u16 size; /* 0x34 or 52 bytes */
	u16 phentry_size; /* 0x20 or 32 bytes (program header size) */
	u16 phcount; /* program header entry count */
	u16 shentry_size; /* 0x28 or 40 bytes (section header entry size) */
	u16 shcount; /* section header entry count */
	u16 shname_index; /* index to section header entry with section names */
} elf_header_t;

typedef struct {
	u32 type;
	u32 offset;
	u32 vaddress;
	u32 paddress;
	u32 file_size;
	u32 memory_size;
	u32 flags;
	u32 align;
} elf_program_header_t;

typedef struct {
	u32 name_offset;
	u32 type;
	u32 flags;
	u32 address;
	u32 offset;
	u32 size;
	u32 link;
	u32 info;
	u32 address_align;
	u32 entry_size;
} elf_section_header_t;

typedef struct {
	u32 offset;
	u32 info;
} elf_relocation_t;

typedef struct {
	elf_header_t header;
	elf_program_header_t program_header;
} elf_t;

typedef struct {
	u8* buffer;
	usize size;
	usize capacity;
	s32 advance_only;
} codegen_buffer_t;

usize write_buffer(void* src, usize size, usize count, codegen_buffer_t* buffer) {
	u8* source = (u8*) src;
	if (buffer->size + (size * count) > buffer->capacity) {
		usize new_capacity = buffer->capacity * 2;
		if (new_capacity < buffer->size + (size * count)) {
			new_capacity = buffer->size + (size * count);
		}

		void* p = realloc(buffer->buffer, new_capacity);
		if (p == NULL) {
			printf("Failed to allocate memory\n");
			return 0;
		}

		buffer->buffer = (u8*) p;
		buffer->capacity = new_capacity;
	}

	if (!buffer->advance_only) {
		memcpy(buffer->buffer + buffer->size, source, size * count);
	}
	buffer->size += size * count;
	return count;
}

s32 write_elf(codegen_buffer_t* buffer, elf_t* elf) {
	if (write_buffer(elf->header.ident.magic, sizeof(elf->header.ident.magic), 1, buffer) != 1) {
		printf("Failed to write magic\n");
		return 0;
	}

	if (write_buffer(&elf->header.ident.class, sizeof(elf->header.ident.class), 1, buffer) != 1) {
		printf("Failed to write class\n");
		return 0;
	}

	if (write_buffer(&elf->header.ident.data, sizeof(elf->header.ident.data), 1, buffer) != 1) {
		printf("Failed to write data\n");
		return 0;
	}

	if (write_buffer(&elf->header.ident.version, sizeof(elf->header.ident.version), 1, buffer) != 1) {
		printf("Failed to write version\n");
		return 0;
	}

	if (write_buffer(&elf->header.ident.abi, sizeof(elf->header.ident.abi), 1, buffer) != 1) {
		printf("Failed to write abi\n");
		return 0;
	}

	if (write_buffer(&elf->header.ident.abiversion, sizeof(elf->header.ident.abiversion), 1, buffer) != 1) {
		printf("Failed to write abiversion\n");
		return 0;
	}

	if (write_buffer(elf->header.ident.reserved, sizeof(u8) * 7, 1, buffer) != 1) {
		printf("Failed to write reserved\n");
		return 0;
	}

	if (write_buffer(&elf->header.type, sizeof(elf->header.type), 1, buffer) != 1) {
		printf("Failed to write type\n");
		return 0;
	}

	if (write_buffer(&elf->header.machine, sizeof(elf->header.machine), 1, buffer) != 1) {
		printf("Failed to write machine\n");
		return 0;
	}

	if (write_buffer(&elf->header.version, sizeof(elf->header.version), 1, buffer) != 1) {
		printf("Failed to write version\n");
		return 0;
	}

	if (write_buffer(&elf->header.entry, sizeof(elf->header.entry), 1, buffer) != 1) {
		printf("Failed to write entry\n");
		return 0;
	}

	if (write_buffer(&elf->header.phoffset, sizeof(elf->header.phoffset), 1, buffer) != 1) {
		printf("Failed to write phoffset\n");
		return 0;
	}

	if (write_buffer(&elf->header.shoffset, sizeof(elf->header.shoffset), 1, buffer) != 1) {
		printf("Failed to write shoffset\n");
		return 0;
	}

	if (write_buffer(&elf->header.flags, sizeof(elf->header.flags), 1, buffer) != 1) {
		printf("Failed to write flags\n");
		return 0;
	}

	if (write_buffer(&elf->header.size, sizeof(elf->header.size), 1, buffer) != 1) {
		printf("Failed to write size\n");
		return 0;
	}

	if (write_buffer(&elf->header.phentry_size, sizeof(elf->header.phentry_size), 1, buffer) != 1) {
		printf("Failed to write phentry_size\n");
		return 0;
	}

	if (write_buffer(&elf->header.phcount, sizeof(elf->header.phcount), 1, buffer) != 1) {
		printf("Failed to write phcount\n");
		return 0;
	}

	if (write_buffer(&elf->header.shentry_size, sizeof(elf->header.shentry_size), 1, buffer) != 1) {
		printf("Failed to write shentry_size\n");
		return 0;
	}

	if (write_buffer(&elf->header.shcount, sizeof(elf->header.shcount), 1, buffer) != 1) {
		printf("Failed to write shcount\n");
		return 0;
	}

	if (write_buffer(&elf->header.shname_index, sizeof(elf->header.shname_index), 1, buffer) != 1) {
		printf("Failed to write shname_index\n");
		return 0;
	}

	if (write_buffer(&elf->program_header.type, sizeof(elf->program_header.type), 1, buffer) != 1) {
		printf("Failed to write program header type\n");
		return 0;
	}

	if (write_buffer(&elf->program_header.offset, sizeof(elf->program_header.offset), 1, buffer) != 1) {
		printf("Failed to write program header offset\n");
		return 0;
	}

	if (write_buffer(&elf->program_header.vaddress, sizeof(elf->program_header.vaddress), 1, buffer) != 1) {
		printf("Failed to write program header vaddress\n");
		return 0;
	}

	if (write_buffer(&elf->program_header.paddress, sizeof(elf->program_header.paddress), 1, buffer) != 1) {
		printf("Failed to write program header paddress\n");
		return 0;
	}

	if (write_buffer(&elf->program_header.file_size, sizeof(elf->program_header.file_size), 1, buffer) != 1) {
		printf("Failed to write program header file_size\n");
		return 0;
	}

	if (write_buffer(&elf->program_header.memory_size, sizeof(elf->program_header.memory_size), 1, buffer) != 1) {
		printf("Failed to write program header memory_size\n");
		return 0;
	}

	if (write_buffer(&elf->program_header.flags, sizeof(elf->program_header.flags), 1, buffer) != 1) {
		printf("Failed to write program header flags\n");
		return 0;
	}

	if (write_buffer(&elf->program_header.align, sizeof(elf->program_header.align), 1, buffer) != 1) {
		printf("Failed to write program header align\n");
		return 0;
	}

	return 1;
}

s32 write_elf_sections(codegen_buffer_t* buffer, elf_section_header_t* sections, u32 section_count) {
	for (u32 i = 0; i < section_count; ++i) {
		if (write_buffer(&sections[i].name_offset, sizeof(sections[i].name_offset), 1, buffer) != 1) {
			printf("Failed to write section name offset\n");
			return 0;
		}

		if (write_buffer(&sections[i].type, sizeof(sections[i].type), 1, buffer) != 1) {
			printf("Failed to write section type\n");
			return 0;
		}

		if (write_buffer(&sections[i].flags, sizeof(sections[i].flags), 1, buffer) != 1) {
			printf("Failed to write section flags\n");
			return 0;
		}

		if (write_buffer(&sections[i].address, sizeof(sections[i].address), 1, buffer) != 1) {
			printf("Failed to write section address\n");
			return 0;
		}

		if (write_buffer(&sections[i].offset, sizeof(sections[i].offset), 1, buffer) != 1) {
			printf("Failed to write section offset\n");
			return 0;
		}

		if (write_buffer(&sections[i].size, sizeof(sections[i].size), 1, buffer) != 1) {
			printf("Failed to write section size\n");
			return 0;
		}

		if (write_buffer(&sections[i].link, sizeof(sections[i].link), 1, buffer) != 1) {
			printf("Failed to write section link\n");
			return 0;
		}

		if (write_buffer(&sections[i].info, sizeof(sections[i].info), 1, buffer) != 1) {
			printf("Failed to write section info\n");
			return 0;
		}

		if (write_buffer(&sections[i].address_align, sizeof(sections[i].address_align), 1, buffer) != 1) {
			printf("Failed to write section address align\n");
			return 0;
		}

		if (write_buffer(&sections[i].entry_size, sizeof(sections[i].entry_size), 1, buffer) != 1) {
			printf("Failed to write section entry size\n");
			return 0;
		}
	}

	return 1;
}

void hexdump(codegen_buffer_t* buffer) {
	for (usize i = 0; i < buffer->size; ++i) {
		if (i % 16 == 0) {
			printf("0x%08X: ", (u32) i);
		}

		printf("%02X", buffer->buffer[i]);

		if (i % 2 == 1) {
			printf(" ");
		}

		if (i % 16 == 15) {
			for (usize j = 0; j < 16; ++j) {
				if (buffer->buffer[i - 15 + j] >= 0x20 && buffer->buffer[i - 15 + j] <= 0x7E) {
					printf("%c", buffer->buffer[i - 15 + j]);
				} else {
					printf(".");
				}
			}
			printf("\n");
		}
	}
}

s32 codegen_obj(assembler_t* assembler, FILE* outfp) {
	/* elf header */
	elf_t elf = {
		.header = {
			.ident = {
				.magic = { 0x7F, 'E', 'L', 'F' },
				.class = 0x01,
				.data = 0x01,
				.version = 0x01,
				.abi = 0x6B,
				.abiversion = 0x00,
				.reserved = { 0 },
			},

			.type = 0x02,
			.machine = 0x726B,
			.version = 0x01,
			.entry = 0x00000000,
			.phoffset = 0x00000034,
			.shoffset = 0x00000000,
			.flags = 0x00000000,
			.size = 0x0034,
			.phentry_size = 0x0020,
			.phcount = 0x0001,
			.shentry_size = 0x0028,
			.shcount = (u32) (assembler->section_count + assembler->private_section_count),
			.shname_index = 0x0001,
		},
		.program_header = {
			.type = 0x01,
			.offset = 0x00000054,
			.vaddress = 0x00000000,
			.paddress = 0x00000000,
			.file_size = assembler->current_address + 1,
			.memory_size = assembler->current_address + 1,
			.flags = 0x00000005,
			.align = 0x00000004,
		}
	};

	elf_section_header_t* headers = (elf_section_header_t*) malloc(sizeof(elf_section_header_t) * (assembler->section_count + assembler->private_section_count));
	if (headers == NULL) {
		printf("Failed to allocate memory\n");
		return 0;
	}

	const char** section_names = (const char**) malloc(sizeof(const char*) * (assembler->section_count + assembler->private_section_count));
	if (section_names == NULL) {
		printf("Failed to allocate memory\n");
		return 0;
	}

	section_names[0] = "";
	headers[0] = (elf_section_header_t){
		.name_offset = 0,
		.type = 0x00,
		.flags = 0x00,
		.address = 0x00000000,
		.offset = 0x00000000,
		.size = 0x00000000,
		.link = 0x00000000,
		.info = 0x00000000,
		.address_align = 0x00000000,
		.entry_size = 0x00000000,
	};

	section_names[1] = ".shstrtab";
	headers[1] = (elf_section_header_t){
		.name_offset = 1,
		.type = 0x03,
		.flags = 0x00,
		.address = 0x00000000,
		.offset = 0x00000000,
		.size = 0x00000000,
		.link = 0x00000000,
		.info = 0x00000000,
		.address_align = 0x00000001,
		.entry_size = 0x00000000,
	};

	for (usize i = 2; i < assembler->section_count + assembler->private_section_count; ++i) {
		section_names[i] = malloc(assembler->sections[i - assembler->private_section_count].name.length + 1);
		if (section_names[i] == NULL) {
			printf("Failed to allocate memory\n");
			return 0;
		}

		memcpy((void*) section_names[i], (const void*) assembler->sections[i - assembler->private_section_count].name.start, assembler->sections[i - assembler->private_section_count].name.length);
		((char*) section_names[i])[assembler->sections[i - assembler->private_section_count].name.length] = '\0';

		if (strcmp(section_names[i], ".text") == 0) {
			headers[i].type = 0x01;
			headers[i].flags = 0x06;
		} else if (strcmp(section_names[i], ".data") == 0) {
			headers[i].type = 0x01;
			headers[i].flags = 0x03;
		} else if (strcmp(section_names[i], ".bss") == 0) {
			headers[i].type = 0x08;
			headers[i].flags = 0x03;
		} else {
			headers[i].type = 0x00;
			headers[i].flags = 0x00;
		}

		headers[i].name_offset = 0x00000000;
		headers[i].address = 0x00000000;
		headers[i].offset = 0x00000000;
		headers[i].size = 0x00000000;
		headers[i].link = 0x00000000;
		headers[i].info = 0x00000000;
		headers[i].address_align = 0x00000004;
		headers[i].entry_size = 0x00000000;
	}

	codegen_buffer_t buffer = {
		.capacity = sizeof(elf_header_t) + sizeof(elf_program_header_t) + sizeof(elf_section_header_t) * (assembler->section_count + assembler->private_section_count) + 20,
		.size = 0,
		.advance_only = 0,
	};

	buffer.buffer = (u8*) malloc(buffer.capacity);
	if (buffer.buffer == NULL) {
		printf("Failed to allocate memory\n");
		return 0;
	}

	buffer.advance_only = 1;
	if (!write_elf(&buffer, &elf)) {
		printf("Failed to write elf header\n");
		return 0;
	}
	buffer.advance_only = 0;

	u32 current_offset = (u32) buffer.size;
	u32 current_sections_name_len = 1;
	for (usize sect = 0; sect < assembler->section_count; ++sect) {
		elf_section_header_t* header = &headers[sect + assembler->private_section_count];
		header->offset = current_offset;

		current_sections_name_len += (u32) strlen(section_names[sect + 1]) + 1;
		header->name_offset = current_sections_name_len;
		header->address = current_offset - 0x54 + elf.program_header.vaddress;

		for (usize i = 0; i < assembler->line_count; ++i) {
			line_t line = assembler->lines[i];
			if (line.section != sect) {
				continue;
			}

			switch (line.define_type) {
				case DEFINE_TYPE_8:
					if (write_buffer(&line.def8, 1, 1, &buffer) != 1) {
						printf("Failed to write define byte\n");
						return 0;
					}
					current_offset += 1;
					continue;
				case DEFINE_TYPE_16:
					if (write_buffer(&line.def16, 2, 1, &buffer) != 1) {
						printf("Failed to write define word\n");
						return 0;
					}
					current_offset += 2;
					continue;
				case DEFINE_TYPE_32:
					if (write_buffer(&line.def32, 4, 1, &buffer) != 1) {
						printf("Failed to write define dword\n");
						return 0;
					}
					current_offset += 4;
					continue;
				case DEFINE_TYPE_64:
					if (write_buffer(&line.def64, 8, 1, &buffer) != 1) {
						printf("Failed to write define qword\n");
						return 0;
					}
					current_offset += 8;
					continue;
				default:
					break;
			}

			u8 opcode = line.instruction->opcode;
			if (write_buffer(&opcode, 1, 1, &buffer) != 1) {
				printf("Failed to write opcode\n");
				return 0;
			}
			current_offset += 1;

			switch (line.instruction->type) {
				case INSTRUCTION_TYPE_NO_OPERAND:
					break;
				case INSTRUCTION_TYPE_1_REGISTER:
					if (write_buffer(&line.operands[0].reg->id, 1, 1, &buffer) != 1) {
						printf("Failed to write register id\n");
						return 0;
					}
					current_offset += 1;
					break;
				case INSTRUCTION_TYPE_2_REGISTER:
					if (write_buffer(&line.operands[0].reg->id, 1, 1, &buffer) != 1) {
						printf("Failed to write register id\n");
						return 0;
					}
					if (write_buffer(&line.operands[1].reg->id, 1, 1, &buffer) != 1) {
						printf("Failed to write register id\n");
						return 0;
					}
					current_offset += 2;
					break;
				case INSTRUCTION_TYPE_3_REGISTER:
					if (write_buffer(&line.operands[0].reg->id, 1, 1, &buffer) != 1) {
						printf("Failed to write register id\n");
						return 0;
					}
					if (write_buffer(&line.operands[1].reg->id, 1, 1, &buffer) != 1) {
						printf("Failed to write register id\n");
						return 0;
					}
					if (write_buffer(&line.operands[2].reg->id, 1, 1, &buffer) != 1) {
						printf("Failed to write register id\n");
						return 0;
					}
					current_offset += 3;
					break;
				case INSTRUCTION_TYPE_1_REGISTER_1_IMMEDIATE:
					if (write_buffer(&line.operands[0].reg->id, 1, 1, &buffer) != 1) {
						printf("Failed to write register id\n");
						return 0;
					}
					if (line.operands[1].is_label) {
						if (write_buffer(&line.operands[1].label->address, 4, 1, &buffer) != 1) {
							printf("Failed to write label address\n");
							return 0;
						}
					} else {
						if (write_buffer(&line.operands[1].immediate, 4, 1, &buffer) != 1) {
							printf("Failed to write immediate\n");
							return 0;
						}
					}
					current_offset += 5;
					break;
                case INSTRUCTION_TYPE_1_IMMEDIATE:
					if (line.operands[0].is_label) {
						if (write_buffer(&line.operands[0].label->address, 4, 1, &buffer) != 1) {
							printf("Failed to write label address\n");
							return 0;
						}
					} else {
						if (write_buffer(&line.operands[0].immediate, 4, 1, &buffer) != 1) {
							printf("Failed to write immediate\n");
							return 0;
						}
					}
                    current_offset += 4;
                    break;
				case INSTRUCTION_TYPE_SYSTEM:
					if (write_buffer(&line.operands[0].imm8, 1, 1, &buffer) != 1) {
						printf("Failed to write imm8\n");
						return 0;
					}
					current_offset += 1;
					break;
			}
		}

		header->size = current_offset - header->offset;
	}

	usize final_strtab_size = 0;
	for (usize i = 0; i < assembler->section_count + assembler->private_section_count; ++i) {
		final_strtab_size += strlen(section_names[i]) + 1;
	}

	char* final_strtab = (char*) malloc(final_strtab_size);
	if (final_strtab == NULL) {
		printf("Failed to allocate memory\n");
		return 0;
	}

	u32 offset = 0;
	for (usize i = 0; i < assembler->section_count + assembler->private_section_count; ++i) {
		memcpy(&final_strtab[offset], section_names[i], strlen(section_names[i]) + 1);
		offset += (u32) strlen(section_names[i]) + 1;
	}

	headers[1].offset = current_offset;
	headers[1].size = offset;

	if (write_buffer(final_strtab, offset, 1, &buffer) != 1) {
		printf("Failed to write section names\n");
		return 0;
	}

	elf.header.shoffset = (u32) buffer.size;
	usize old_size = buffer.size;
	buffer.size = 0;
	if (!write_elf(&buffer, &elf)) {
		printf("Failed to write elf header\n");
		return 0;
	}
	buffer.size = old_size;

	if (!write_elf_sections(&buffer, headers, (u32) (assembler->section_count + assembler->private_section_count))) {
		printf("Failed to write section headers\n");
		return 0;
	}

	if (fwrite(buffer.buffer, buffer.size, 1, outfp) != 1) {
		printf("Failed to write to file\n");
		return 0;
	}

	for (usize i = 2; i < assembler->section_count + assembler->private_section_count; ++i) {
		free((void*) section_names[i]);
	}

	free(section_names);
	free(headers);
	free(buffer.buffer);
	free(final_strtab);
	return 1;
}

char word_buffer[64];
char* word_cstring(word_t* word) {
	memcpy(word_buffer, word->start, word->length % 64);
	word_buffer[word->length % 64] = '\0';
	return word_buffer;
}

s32 main(int argc, char** argv) {
	const char* asm_file = NULL;
	const char* out_file = NULL;
	s32 ofile_malloced = 0;

	#ifdef DEBUG_FIXED_FILES
	asm_file = "../../../test.asm";
	out_file = "../../../test.elf";
	#else
	if (argc < 2) {
		printf("Usage: %s <source file> [options]\n", argv[0]);
		printf("Flags:\n  [-o], [/Fo]\n    <output file>  Output file\n");
		return 1;
	}

	for (usize i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "/Fo") == 0) {
			if (i + 1 >= argc) {
				printf("Expected output file after -o\n");
				return 1;
			}

			out_file = argv[i + 1];
			break;
		} else {
			asm_file = argv[i];
		}
	}

	if (asm_file == NULL) {
		printf("No source file specified\n");
		return 1;
	}

	/* asm_file - extension + .elf */
	if (out_file == NULL) {
		usize len = strlen(asm_file);
		usize base_len = len;
		char* ext = strrchr(asm_file, '.');
		if (ext == NULL) {
			len += 4;
		} else {
			base_len -= strlen(ext);
			len = base_len + 4;
		}

		out_file = (char*) malloc(len + 1);
		if (out_file == NULL) {
			printf("Failed to allocate memory\n");
			return 1;
		}

		strncpy((char*) out_file, asm_file, base_len);
		strncpy((char*) &out_file[base_len], ".elf", 4);
		((char*) out_file)[len] = '\0';
		ofile_malloced = 1;
	}
	#endif

	char* buffer = NULL;
	usize size = 0;
	{
		FILE* file = fopen(asm_file, "rb");
		if (file == NULL) {
			printf("Failed to open file: %s\n", asm_file);
			return 1;
		}

		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);

		buffer = (char*) malloc(size + 1);
		if (buffer == NULL) {
			printf("Failed to allocate memory\n");
			return 1;
		}

		fread(buffer, 1, size, file);
		buffer[size] = '\0';
		fclose(file);
	}

	/* init maps */
	for (u16 i = 0; i < sizeof(instructions) / sizeof(instruction_t); ++i) {
		if (instructions[i].name != NULL) {
			instructions[i].hash = hash_string(instructions[i].name);
		}
	}

	for (u16 i = 0; i < sizeof(registers) / sizeof(reg_t); ++i) {
		if (registers[i].name != NULL) {
			registers[i].hash = hash_string(registers[i].name);
		}
	}

	assembler_t assembler = { .lines = NULL, .line_count = 0, .current_address = 0, .current_section = 0, .labels = NULL, .label_count = 0, .sections = NULL, .section_count = 0 };
	assembler.sections = malloc(sizeof(section_t));
	if (assembler.sections == NULL) {
		printf("Failed to allocate memory\n");
		return 1;
	}

	text_section.hash = hash_string(".text");
	assembler.sections[0] = text_section;
	assembler.current_section = 0;
	assembler.section_count = 1;
	assembler.private_section_count = 2;

	word_t word;
	usize index = 0;
	usize s = size;
	while (retrieve_word(&buffer[index], s, &word)) {
		if (word.length == 0) {
			while (buffer[index] == ' ' || buffer[index] == '\t' || buffer[index] == '\n' || buffer[index] == '\r') {
				++index;
				--s;

				if (s == 0) {
					break;
				}

				if (buffer[index] == '\0') {
					break;
				}
			}

			continue;
		}

		if (word.start[0] == '/' && word.length >= 2) {
			if (word.start[1] == '/') {
				while (buffer[index] != '\n') {
					if (buffer[index] == '\r') {
						if (s >= 1 && buffer[index] != '\n') {
							break;
						}
					}

					++index;
					--s;

					if (s == 0) {
						printf("Unterminated comment\n");
						return 0;
					}

					if (buffer[index] == '\0') {
						printf("Unterminated comment\n");
						return 0;
					}
				}

				continue;
			} else if (word.start[1] == '*') {
				word_t w;
				while (1) {
					if (!retrieve_word(&buffer[index], s, &w)) {
						printf("Unterminated comment\n");
						return 0;
					}

					index += w.length + 1;
					s -= w.length + 1;

					if (w.length == 2 && w.start[0] == '*' && w.start[1] == '/') {
						break;
					}
				}

				if (s == 0) {
					break;
				}

				if (buffer[index] == '\0') {
					break;
				}

				continue;
			}
		}

		index += word.length + 1;
		s -= word.length + 1;

		if (word.start[word.length - 1] == ',') {
			--word.length;
		}

		if (!process_word(&assembler, &word)) {
			return 0;
		}

	    if (index >= size) {
			break;
		}

		if (s == 0) {
			break;
		}

		if (index + word.length >= size) {
			break;
		}

		if (buffer[index] == '\0') {
			break;
		}
	}

	if (!evaluate_labels(&assembler)) {
		return 0;
	}

	FILE* outfp = fopen(out_file, "wb");
	if (ofile_malloced) {
		free((void*) out_file);
	}

	if (outfp == NULL) {
		printf("Failed to open output file\n");
		return 1;
	}

	codegen_obj(&assembler, outfp);
	fflush(outfp);
	fclose(outfp);

	if (assembler.lines != NULL) {
		free(assembler.lines);
	}

	if (assembler.labels != NULL) {
		free(assembler.labels);
	}

	if (assembler.labels != NULL) {
		free(assembler.sections);
	}

	free(buffer);
}
