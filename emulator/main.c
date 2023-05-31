#include <common.h>
#include <cpu.h>

#include <stdio.h>

#define ROM_SIZE 256

u8 rom[ROM_SIZE] = {
	0x00, 0x00, 0x69, 0x00, 0x00, 0x00,
	0x0C
};

cpu_t * cpu;
int main(void) {
	cpu = cpu_new(0xFFFFFF);
	cpu_load_rom(cpu, rom, ROM_SIZE);
	printf("cpu exited with r0: %08x\n", cpu_run(cpu));

	cpu_free(cpu);

	return 0;
}
