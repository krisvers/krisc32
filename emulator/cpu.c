#include <cpu.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

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

	return cpu;
}
