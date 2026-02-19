.data
fmt_dbg:    .asciz "|%s|\n"
fmt_sum:    .asciz "%ld\n"
fmt_bad:    .asciz "expected: <int> <int>\n"
fmt_num:    .asciz "bad number format (use optional +/- and digits only)\n"
fmt_long:   .asciz "input too long (buffer overflow)\n"

.equ BUFSZ, 100
.equ SPC,   32
.equ LF,    10
.equ CR,    13
.equ MINUS, 45
.equ PLUS,  43

.bss
.align 4
buf: .space BUFSZ

.text
.global main

main:
    addi sp, sp, -32
    sd   ra, 24(sp)

    la   a0, buf
    li   a1, BUFSZ
    call gets

    la   t0, buf
strip_loop:
    lbu  t1, 0(t0)
    beqz t1, strip_done
    li   t2, LF
    beq  t1, t2, strip_zero
    li   t2, CR
    beq  t1, t2, strip_zero
    addi t0, t0, 1
    j    strip_loop
strip_zero:
    sb   zero, 0(t0)

strip_done:
    la   a0, fmt_dbg
    la   a1, buf
    call printf

    la   t0, buf
skip_leading:
    lbu  t1, 0(t0)
    beqz t1, bad_fmt
    li   t2, SPC
    bne  t1, t2, first_ok
    addi t0, t0, 1
    j    skip_leading

first_ok:
    sd   t0, 0(sp)

find_space:
    lbu  t1, 0(t0)
    beqz t1, bad_fmt
    li   t2, SPC
    beq  t1, t2, split
    addi t0, t0, 1
    j    find_space

split:
    sb   zero, 0(t0)
    addi t0, t0, 1

skip_between:
    lbu  t1, 0(t0)
    beqz t1, bad_fmt
    li   t2, SPC
    bne  t1, t2, second_ok
    addi t0, t0, 1
    j    skip_between

second_ok:
    sd   t0, 8(sp)

    mv   t3, t0
scan_second_for_space:
    lbu  t4, 0(t3)
    beqz t4, validate_first
    li   t5, SPC
    beq  t4, t5, bad_fmt
    addi t3, t3, 1
    j    scan_second_for_space

validate_first:
    ld   t3, 0(sp)
    lbu  t4, 0(t3)
    beqz t4, bad_num

    li   t5, MINUS
    beq  t4, t5, v1_skip
    li   t5, PLUS
    beq  t4, t5, v1_skip
    j    v1_first_digit

v1_skip:
    addi t3, t3, 1
    lbu  t4, 0(t3)
    beqz t4, bad_num

v1_first_digit:
    li   t5, 48
    bltu t4, t5, bad_num
    li   t5, 57
    bltu t5, t4, bad_num

v1_loop:
    addi t3, t3, 1
    lbu  t4, 0(t3)
    beqz t4, validate_second
    li   t5, 48
    bltu t4, t5, bad_num
    li   t5, 57
    bltu t5, t4, bad_num
    j    v1_loop

validate_second:
    ld   t3, 8(sp)
    lbu  t4, 0(t3)
    beqz t4, bad_num

    li   t5, MINUS
    beq  t4, t5, v2_skip
    li   t5, PLUS
    beq  t4, t5, v2_skip
    j    v2_first_digit

v2_skip:
    addi t3, t3, 1
    lbu  t4, 0(t3)
    beqz t4, bad_num

v2_first_digit:
    li   t5, 48
    bltu t4, t5, bad_num
    li   t5, 57
    bltu t5, t4, bad_num

v2_loop:
    addi t3, t3, 1
    lbu  t4, 0(t3)
    beqz t4, parse_numbers
    li   t5, 48
    bltu t4, t5, bad_num
    li   t5, 57
    bltu t5, t4, bad_num
    j    v2_loop

parse_numbers:
    ld   t3, 0(sp)
    lbu  t4, 0(t3)
    li   t5, MINUS
    bne  t4, t5, p_not_minus
    addi t3, t3, 1
    mv   a0, t3
    call atoi
    negw a0, a0
    j    p_done
p_not_minus:
    li   t5, PLUS
    bne  t4, t5, p_plain
    addi t3, t3, 1
p_plain:
    mv   a0, t3
    call atoi
p_done:
    sw   a0, 16(sp)

    ld   t3, 8(sp)
    lbu  t4, 0(t3)
    li   t5, MINUS
    bne  t4, t5, q_not_minus
    addi t3, t3, 1
    mv   a0, t3
    call atoi
    negw a0, a0
    j    q_done
q_not_minus:
    li   t5, PLUS
    bne  t4, t5, q_plain
    addi t3, t3, 1
q_plain:
    mv   a0, t3
    call atoi
q_done:
    mv   t1, a0

    lw   a0, 16(sp)
    mv   a1, t1
    call addab

    mv   a1, a0
    la   a0, fmt_sum
    call printf

    li   a0, 0
    j    done

bad_fmt:
    la   a0, fmt_bad
    call printf
    li   a0, 1
    j    done

bad_num:
    la   a0, fmt_num
    call printf
    li   a0, 1
    j    done

done:
    ld   ra, 24(sp)
    addi sp, sp, 32
    ret
