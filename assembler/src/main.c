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
    INSTRUCTION_TYPE_SYSTEM = 5,
    INSTRUCTION_TYPE_COUNT,
} instruction_type_t;

instruction_type_info_t instruction_types[INSTRUCTION_TYPE_COUNT] = {
    [INSTRUCTION_TYPE_NO_OPERAND] = { .operand_types = { OPERAND_TYPE_NONE }, .operand_size = 0 },
    [INSTRUCTION_TYPE_1_REGISTER] = { .operand_types = { OPERAND_TYPE_REGISTER, OPERAND_TYPE_NONE }, .operand_size = 1 },
    [INSTRUCTION_TYPE_2_REGISTER] = { .operand_types = { OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, OPERAND_TYPE_NONE }, .operand_size = 2 },
    [INSTRUCTION_TYPE_3_REGISTER] = { .operand_types = { OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, }, .operand_size = 3 },
    [INSTRUCTION_TYPE_1_REGISTER_1_IMMEDIATE] = { .operand_types = { OPERAND_TYPE_REGISTER, OPERAND_TYPE_IMMEDIATE, OPERAND_TYPE_NONE }, .operand_size = 5 },
    [INSTRUCTION_TYPE_SYSTEM] = { .operand_types = { OPERAND_TYPE_IMM8, OPERAND_TYPE_NONE }, .operand_size = 1 },
};

typedef struct {
    char* name;
    usize hash;
    instruction_type_t type;
    u8 operand_count;
} instruction_t;

instruction_t instructions[256] = {
    [0x01] = { .name = "ldi", .type = INSTRUCTION_TYPE_1_REGISTER_1_IMMEDIATE, .operand_count = 2 },
    [0x02] = { .name = "ldr", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2 },
    [0x03] = { .name = "ldm8", .type = INSTRUCTION_TYPE_1_REGISTER_1_IMMEDIATE, .operand_count = 2 },
    [0x04] = { .name = "ldm16", .type = INSTRUCTION_TYPE_1_REGISTER_1_IMMEDIATE, .operand_count = 2 },
    [0x05] = { .name = "ldm32", .type = INSTRUCTION_TYPE_1_REGISTER_1_IMMEDIATE, .operand_count = 2 },
    [0x06] = { .name = "str8", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2 },
    [0x07] = { .name = "str16", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2 },
    [0x08] = { .name = "str32", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2 },
    [0x09] = { .name = "add", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3 },
    [0x0A] = { .name = "sub", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3 },
    [0x0B] = { .name = "mul", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3 },
    [0x0C] = { .name = "div", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3 },
    [0x0D] = { .name = "rem", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3 },
    [0x0E] = { .name = "shr", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3 },
    [0x0F] = { .name = "shl", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3 },
    [0x10] = { .name = "and", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3 },
    [0x11] = { .name = "or", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3 },
    [0x12] = { .name = "not", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2 },
    [0x13] = { .name = "xor", .type = INSTRUCTION_TYPE_3_REGISTER, .operand_count = 3 },
    [0x14] = { .name = "jnz", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2 },
    [0x15] = { .name = "jz", .type = INSTRUCTION_TYPE_2_REGISTER, .operand_count = 2 },
    [0x16] = { .name = "jmp", .type = INSTRUCTION_TYPE_1_REGISTER, .operand_count = 1 },
    [0x17] = { .name = "link", .type = INSTRUCTION_TYPE_1_REGISTER, .operand_count = 1 },
    [0x18] = { .name = "ret", .type = INSTRUCTION_TYPE_NO_OPERAND, .operand_count = 0 },
    [0x19] = { .name = "push", .type = INSTRUCTION_TYPE_1_REGISTER, .operand_count = 1 },
    [0x1A] = { .name = "pop", .type = INSTRUCTION_TYPE_1_REGISTER, .operand_count = 1 },
    [0x60] = { .name = "hlt", .type = INSTRUCTION_TYPE_NO_OPERAND, .operand_count = 0 },
    [0x80] = { .name = "sys", .type = INSTRUCTION_TYPE_SYSTEM, .operand_count = 1 },
    [0xF0] = { .name = "int", .type = INSTRUCTION_TYPE_SYSTEM, .operand_count = 1 },
};

typedef struct {
    char* start;
    usize length;
} word_t;

typedef struct {
    instruction_t* instruction;
    word_t operands[3];
} line_t;

typedef struct {
    line_t* lines;
    usize count;
    usize current;
} assembler_t;

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
    for (usize i = 0; i < word->length; i++) {
        hash = (hash * 31) + word->start[i];
    }

    return hash;
}

s32 retrieve_word(char* buffer, usize size, word_t* word) {
    if (size == 0) {
        return 0;
    }

    usize end = 0;
    while (end < size && buffer[end] != '\0' && buffer[end] != ' ' && buffer[end] != '\t' && buffer[end] != '\n') {
        end++;
    }

    word->start = buffer;
    word->length = end;
    return 1;
}

s32 process_word(assembler_t* asm, word_t* word) {
    line_t line = { .instruction = NULL, .operands = { { .start = NULL, .length = 0 }, { .start = NULL, .length = 0 }, { .start = NULL, .length = 0 } } };
    if (asm->current < asm->count) {
        line = asm->lines[asm->current];
    }

    usize hash = hash_word(word);
    for (u16 i = 0; i < 256; i++) {
        if (instructions[i].hash == hash) {
            if (line.instruction != NULL) {
                printf("Error: unexpected instruction '%s' already found '%s'\n", instructions[i].name, line.instruction->name);
                return 0;
            }

            line.instruction = &instructions[i];
            break;
        }
    }

    return 0;
}

s32 main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }

    char* buffer = NULL;
    usize size = 0;
    {
        FILE* file = fopen(argv[1], "rb");
        if (file == NULL) {
            printf("Failed to open file: %s\n", argv[1]);
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
    for (u16 i = 0; i < 256; i++) {
        if (instructions[i].name != NULL) {
            instructions[i].hash = hash_string(instructions[i].name);
        }
    }

    assembler_t asm = { .lines = NULL, .count = 0, .current = 0 };

    word_t word;
    usize index = 0;
    usize s = size;
    while (retrieve_word(&buffer[index], s, &word)) {
        printf("word (%zu): '%s'\n", word.length, word.start);
        printf("buffer: %p '%c', size: %zu\n", &buffer[index], buffer[index], s);
        if (index + word.length >= size) {
            break;
        }

        index += word.length + 1;
        s -= word.length + 1;

        if (!process_word(&asm, &word)) {
            break;
        }

        if (s == 0) {
            break;
        }
    }

    free(buffer);
}