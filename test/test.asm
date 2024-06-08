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

/* 0x0000 */
start:
	ldi sp, 0x1000
	ldi r6, fib_return_first
	ldi r7, fib_return_second
	ldi r8, 2
	ldi r10, fib_one
	ldi r11, 1
	ldi r12, fib_zero
	ldi r13, halt
	ldi r14, fib
	ldi r15, 4

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

	push r2
	push r6
	push r2
	jmp r14 // fib(n - 1)

fib_return_first:
	pop r2 // n - 1

	sub r2, r2, r11 // n - 2
	push r0
	push r7
	push r2
	jmp r14 // fib(n - 2)

fib_return_second:
	pop r4
	add r0, r0, r4
	jmp r12

fib_one:
	ldr r0, r1

fib_zero:
	pop r9
	jmp r9