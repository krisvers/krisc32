.text

start:
    ldi sp, 0x1000
    ldi sys0, handler
    sys 0x03

    loop:
        jmpi loop

    exit:
        hlt

/* r8 color, r9 x, r10 y */
putpixel:
    push r15
    push r14
    push r10

    ldi r15, 0xF0000000
    add r15, r15, r9

    ldi r14, 120
    mul r10, r10, r14

    add r15, r15, r10
    str8 r15, r8

    pop r10
    pop r14
    pop r15
    ret

handler:
    ldi r0, putpixel

    ldi r15, color
    ldm8 r8, r15

    ldi r15, pos_x
    ldm8 r9, r15

    ldi r15, pos_y
    ldm8 r10, r15

    ldi r15, 0xFF000000
    ldm8 r1, r15

    ldi r3, 0x01
    add r15, r15, r3
    ldm8 r2, r15

    jnzi r2, keydown
    ret

    keydown:

    ldi r4, 0x89
    sub r4, r1, r4
    jnzi r4, handler_skip_up

        sub r10, r10, r3

    handler_skip_up:

    ldi r4, 0x88
    sub r4, r1, r4
    jnzi r4, handler_skip_down

        add r10, r10, r3

    handler_skip_down:

    ldi r4, 0x87
    sub r4, r1, r4
    jnzi r4, handler_skip_left

        sub r9, r9, r3

    handler_skip_left:

    ldi r4, 0x86
    sub r4, r1, r4
    jnzi r4, handler_done

        add r9, r9, r3

    handler_done:
        ldi r15, color
        str8 r15, r8

        ldi r15, pos_y
        str8 r15, r10
        
        ldi r15, pos_x
        str8 r15, r9

        link r0
        ret

pos_x:
    =0x00
pos_y:
    =0x00
color:
    =0xE3
