.text

start:
    ldi sp, 0x1000
    ldi sys0, handler
    sys 0x03

    ldi r8, 0xFF
    ldi r9, 119
    ldi r10, 45
    ldi r11, 1
    ldi r12, 7
    ldi r14, 320
    ldi r15, 200

    ldi r0, putpixel

    loop:
        add r9, r9, r12
        add r10, r10, r11
        add r8, r8, r11

        rem r9, r9, r14
        add r9, r9, r10
        rem r10, r10, r15
        
        link r0
        jmpi loop

    exit:
        hlt

/* r8 color, r9 x, r10 y */
putpixel:
    push r15
    push r14
    push r10

    /* setup framebuffer address */
    ldi r15, 0xF0000000
    add r15, r15, r9

    /* get width */
    ldi r14, 0xF0050000
    ldm16 r13, r14
    mul r10, r10, r13

    add r15, r15, r10
    str8 r15, r8

    pop r10
    pop r14
    pop r15
    ret

handler:
    ret
