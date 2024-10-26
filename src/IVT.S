.balign 4
.globl IVT
IVT:
j int_internals
j int_timer
j int_invalid
j int_invalid
j int_invalid
j int_invalid
j int_invalid
j int_invalid
j int_invalid
j int_tty