#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define GET_U16(buffer, offset) (buffer[offset]) | (buffer[offset + 1] << 8)
#define GET_U32(buffer, offset) (buffer[offset]) | (buffer[offset + 1] << 8) | (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24)

/* little-endian */
s32 parse_elf(u8* buffer, usize size, elf_t* elf) {
	elf->header.ident.magic[0] = buffer[0x00];
	elf->header.ident.magic[1] = buffer[0x01];
	elf->header.ident.magic[2] = buffer[0x02];
	elf->header.ident.magic[3] = buffer[0x03];

	if (elf->header.ident.magic[0] != 0x7F || elf->header.ident.magic[1] != 'E' || elf->header.ident.magic[2] != 'L' || elf->header.ident.magic[3] != 'F') {
		printf("Invalid ELF magic\n");
		return 0;
	}

	elf->header.ident.class = buffer[0x04];
	elf->header.ident.data = buffer[0x05];
	elf->header.ident.version = buffer[0x06];
	elf->header.ident.abi = buffer[0x07];
	elf->header.ident.abiversion = buffer[0x08];

	if (elf->header.ident.class != 0x01) {
		printf("Linker supports only 32-bit format\n");
		return 0;
	}

	if (elf->header.ident.data != 0x01) {
		printf("Linker supports only little-endian format\n");
		return 0;
	}

	if (elf->header.ident.version != 0x01) {
		printf("Invalid ELF version\n");
		return 0;
	}

	if (elf->header.ident.abi != 0x6B) {
		printf("Linker supports only kr32 ABI (abi=0x6B)\n");
		return 0;
	}

	for (usize i = 9; i < 16; ++i) {
		elf->header.ident.reserved[i - 9] = buffer[i];
	}

	elf->header.type = GET_U16(buffer, 0x10);
	elf->header.machine = GET_U16(buffer, 0x12);
	elf->header.version = GET_U32(buffer, 0x14);
	elf->header.entry = GET_U32(buffer, 0x18);
	elf->header.phoffset = GET_U32(buffer, 0x1C);
	elf->header.shoffset = GET_U32(buffer, 0x20);
	elf->header.flags = GET_U32(buffer, 0x24);
	elf->header.size = GET_U16(buffer, 0x28);
	elf->header.phentry_size = GET_U16(buffer, 0x2A);
	elf->header.phcount = GET_U16(buffer, 0x2C);
	elf->header.shentry_size = GET_U16(buffer, 0x2E);
	elf->header.shcount = GET_U16(buffer, 0x30);
	elf->header.shname_index = GET_U16(buffer, 0x32);

	if (elf->header.machine != 0x726B) {
		printf("Linker supports only kr32 machine (machine=0x726B) found 0x%x\n", elf->header.machine);
		return 0;
	}

	if (elf->header.phoffset != 0x34) {
		printf("Invalid program header offset (expected 0x34, found 0x%x)\n", elf->header.phoffset);
		return 0;
	}

	if (elf->header.flags != 0x00) {
		printf("Warning: ignoring ELF header flags\n");
	}

	if (elf->header.size != 0x34) {
		printf("Invalid header size (expected 0x52, found 0x%x)\n", elf->header.size);
		return 0;
	}

	if (elf->header.phentry_size != 0x20) {
		printf("Invalid program header size\n");
		return 0;
	}

	if (elf->header.phcount != 0x01) {
		printf("Invalid program header count\n");
		return 0;
	}

	if (elf->header.shentry_size != 0x28) {
		printf("Invalid section header size\n");
		return 0;
	}

	if (elf->header.shcount == 0) {
		printf("No section headers found\n");
		return 0;
	}

	if (elf->header.shname_index >= elf->header.shcount) {
		printf("Invalid section header name index\n");
		return 0;
	}

	usize ph_offset = elf->header.phoffset;
	elf->program_header.type = GET_U32(buffer, ph_offset);
	elf->program_header.offset = GET_U32(buffer, ph_offset + 0x04);
	elf->program_header.vaddress = GET_U32(buffer, ph_offset + 0x08);
	elf->program_header.paddress = GET_U32(buffer, ph_offset + 0x0C);
	elf->program_header.file_size = GET_U32(buffer, ph_offset + 0x10);
	elf->program_header.memory_size = GET_U32(buffer, ph_offset + 0x14);
	elf->program_header.flags = GET_U32(buffer, ph_offset + 0x18);
	elf->program_header.align = GET_U32(buffer, ph_offset + 0x1C);

	if (elf->program_header.type != 0x01) {
		printf("Invalid program header type\n");
		return 0;
	}

	if (elf->program_header.offset != ph_offset + 0x20) {
		printf("Invalid program header offset\n");
		return 0;
	}

	if (elf->program_header.vaddress != 0x00000000) {
		printf("Invalid program header virtual address\n");
		return 0;
	}

	if (elf->program_header.flags != 0x00) {
		printf("Warning: ignoring program header flags\n");
	}

	if (elf->program_header.align != 0x00) {
		printf("Warning: ignoring program header alignment\n");
	}

	return 1;
}

void hexdump(u8* buffer, usize size) {
	for (usize i = 0; i < size; ++i) {
		if (i % 16 == 0) {
			printf("0x%08X: ", (u32) i);
		}

		printf("%02X", buffer[i]);

		if (i % 2 == 1) {
			printf(" ");
		}

		if (i % 16 == 15) {
			for (usize j = 0; j < 16; ++j) {
				if (buffer[i - 15 + j] >= 0x20 && buffer[i - 15 + j] <= 0x7E) {
					printf("%c", buffer[i - 15 + j]);
				} else {
					printf(".");
				}
			}
			printf("\n");
		}
	}
}

int main(int argc, char** argv) {
	s32 base_set = 0;
	u32 base_address = 0;

	const char* elf_file = NULL;
	const char* out_file = NULL;

	s32 ofile_malloced = 0;

	#ifdef DEBUG_FIXED_FILES
	elf_file = "../../../../assembler/test.elf";
	out_file = "../../../test.bin";
	#else
	if (argc < 2) {
		printf("Usage: %s <source file> [options]\n", argv[0]);
		printf("Flags:\n  [-o], [/Fo]\n    <output file>  Output file\n  [--base], [/B]\n    <32-bit address> Base address to relocate to\n");
		return 1;
	}
	#endif

	for (usize i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "/Fo") == 0) {
			if (i + 1 >= argc) {
				printf("Expected output file after -o\n");
				return 1;
			}

			out_file = argv[i + 1];
			++i;
		} else if (strcmp(argv[i], "--base") == 0 || strcmp(argv[i], "/B") == 0) {
			if (i + 1 >= argc) {
				printf("Expected base address after --base\n");
				return 1;
			}

			char* base_str = argv[i + 1];
			usize len = strlen(base_str);

			u32(*char_to_u32)(char) = decchar_to_u32;
			u32 radix = 10;
			u32 start = 0;
			if (len > 2 && base_str[0] == '0' && base_str[1] == 'x') {
				char_to_u32 = hexchar_to_u32;
				radix = 16;
				start = 2;
			}

			u64 value = 0;
			for (usize i = start; i < len; ++i) {
				value = (value * radix) + char_to_u32(base_str[i]);
			}

			if (value > 0xFFFFFFFF) {
				printf("Warning: Base address is larger than 32-bits, truncating value\n");
			}
			base_address = (u32) value;
			base_set = 1;
			++i;
		} else if (elf_file == NULL) {
			elf_file = argv[i];
		} else {
			printf("Unknown argument (%u): '%s'\n", (u32) i, argv[i]);
			return 1;
		}
	}

	if (!base_set) {
		printf("Warning: Base address not set, defaulting to 0x00000000\n");
		base_address = 0;
	}

	if (elf_file == NULL) {
		printf("No source file specified\n");
		return 1;
	}

	if (out_file == NULL) {
		usize len = strlen(elf_file);
		usize base_len = len;
		char* ext = strrchr(elf_file, '.');
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

		strncpy((char*) out_file, elf_file, base_len);
		strncpy((char*) &out_file[base_len], ".bin", 4);
		((char*) out_file)[len] = '\0';
		ofile_malloced = 1;
	}

	FILE* elffp = fopen(elf_file, "rb");
	if (elffp == NULL) {
		printf("Failed to open file: %s\n", elf_file);
		return 1;
	}

	fseek(elffp, 0, SEEK_END);
	usize elf_size = ftell(elffp);
	fseek(elffp, 0, SEEK_SET);

	if (elf_size < sizeof(elf_header_t)) {
		printf("File is too small to be a valid ELF file\n");
		fclose(elffp);
		return 1;
	}

	u8* elf_buffer = (u8*) malloc(elf_size);
	if (elf_buffer == NULL) {
		printf("Failed to allocate memory\n");
		fclose(elffp);
		return 1;
	}

	if (fread(elf_buffer, 1, elf_size, elffp) != elf_size) {
		printf("Failed to read file\n");
		fclose(elffp);
		free(elf_buffer);
		return 1;
	}

	fclose(elffp);

	elf_t elf;
	if (!parse_elf(elf_buffer, elf_size, &elf)) {
		printf("Failed to parse ELF file\n");
		free(elf_buffer);
		return 1;
	}

	// .text first, all other sections after (aside from NULL and .shstrtab)
	usize out_buffer_size = 0;
	u8* out_buffer = NULL;

	usize sh_offset = elf.header.shoffset;
	elf_section_header_t* shstrstab_section = (elf_section_header_t*) &elf_buffer[sh_offset + (elf.header.shname_index * elf.header.shentry_size)];
	usize shstrtab_offset = shstrstab_section->offset;
	usize shstrtab_size = shstrstab_section->size;
	usize shstrtab_index = 0;

	// find .text and put first in buffer
	usize text_offset = 0;
	usize text_size = 0;
	for (usize i = 0; i < elf.header.shcount; ++i) {
		elf_section_header_t* sh = (elf_section_header_t*) &elf_buffer[sh_offset + (i * elf.header.shentry_size)];
		if (sh->name_offset == 0) {
			continue;
		}

		char* name = (char*) &elf_buffer[shstrtab_offset + sh->name_offset];
		if (strcmp(name, ".text") == 0) {
			text_offset = sh->offset;
			text_size = sh->size;
			break;
		}
	}

	if (text_size != 0) {
		out_buffer_size = text_size;
		out_buffer = (u8*) malloc(out_buffer_size);
		if (out_buffer == NULL) {
			printf("Failed to allocate memory\n");
			return 0;
		}

		memcpy(out_buffer, &elf_buffer[text_offset], text_size);
	}

	// find all other sections and put them in buffer
	for (usize i = 0; i < elf.header.shcount; ++i) {
		elf_section_header_t* sh = (elf_section_header_t*) &elf_buffer[sh_offset + (i * elf.header.shentry_size)];
		if (sh->name_offset == 0) {
			continue;
		}

		char* name = (char*) &elf_buffer[shstrtab_offset + sh->name_offset];
		if (strcmp(name, ".text") == 0 || strcmp(name, ".shstrtab") == 0 || strcmp(name, ".symtab") == 0 || strcmp(name, ".strtab") == 0 || strncmp(name, ".rel", 4) == 0 || strncmp(name, ".debug", 6) == 0 || strncmp(name, ".note", 5) == 0 || strncmp(name, ".comment", 8) == 0) {
			continue;
		}

		if (sh->size == 0) {
			continue;
		}

		if (out_buffer == NULL) {
			out_buffer_size = sh->size;
			out_buffer = (u8*) malloc(out_buffer_size);
			if (out_buffer == NULL) {
				printf("Failed to allocate memory\n");
				return 0;
			}
		} else {
			u8* p = (u8*) realloc(out_buffer, out_buffer_size + sh->size);
			if (p == NULL) {
				printf("Failed to allocate memory\n");
				return 0;
			}

			out_buffer = p;
			memcpy(&out_buffer[out_buffer_size], &elf_buffer[sh->offset], sh->size);
			out_buffer_size += sh->size;
		}
	}

	FILE* outfp = fopen(out_file, "wb");
	if (outfp == NULL) {
		printf("Failed to open file: %s\n", out_file);
		free(out_buffer);
		return 1;
	}

	if (fwrite(out_buffer, 1, out_buffer_size, outfp) != out_buffer_size) {
		printf("Failed to write file\n");
		fclose(outfp);
		free(out_buffer);
		return 1;
	}

	fclose(outfp);
	free(out_buffer);

	if (ofile_malloced) {
		free((void*) out_file);
	}

	free(elf_buffer);
	return 0;
}