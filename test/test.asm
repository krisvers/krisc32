/*
int fib(int n) {
	if (n == 0) {
		return 0;
	}

	if (n == 1) {
		return 1;
	}

	return fib(n - 1) + fib(n - 2);
}

int main() {
	int n = 10;
	int result = fib(n);
}
*/

start:
	ldi sp, 0x1000
	ldi r8, 2
	ldi r10, fib_one
	ldi r11, 1
	ldi r12, fib_zero
	ldi r13, halt
	ldi r14, fib
	ldi r15, 10
	push r13
	push r15
	jmp r14

halt:
	hlt

fib:
	pop r1 // n
	jz r1, r12

	sub r2, r1, r11 // n - 1
	jz r2, r10

	sub r1, r1, r8 // n - 2
	push r12
	push r2
	jmp r14 // fib(n - 1)
	ldr r4, r0 // r0 = fib(n - 1)

	push r12
	push r1
	jmp r14 // fib(n - 2)

	add r0, r0, r4
	jmp r12

fib_one:
	ldr r0, r1
fib_zero:
	pop r9
	jmp r9