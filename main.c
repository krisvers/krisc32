#include <common.h>
#include <cpu.h>

#include <stdio.h>

cpu_t * cpu;

int main(void) {
	cpu = cpu_new(0xFFFFFF);
	printf("%u\n", cpu_run(cpu));

	return 0;
}
