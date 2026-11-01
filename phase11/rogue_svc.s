@ Phase 11 modified SVC wrapper from Checkpoint 3ds/source/csvc.s;
@ Copyright 2017-2026 Bernardo Giordano, FlagBrew (permissive license).
@ Only the Luma service client session operation is used; NO live writes.
.arm
.balign 4
.section .text.rogueSvcControlService, "ax", %progbits
.global rogueSvcControlService
.type rogueSvcControlService, %function
.align 2
rogueSvcControlService:
    svc 0xB0
    bx lr
