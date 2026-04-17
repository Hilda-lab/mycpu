; Demo assembly program for myCPU
; Calculates: 5 + 10 = 15, then tests various instructions

; Basic MOV and ADD
    MOV EAX, 5
    MOV EBX, 10
    ADD EAX, EBX      ; EAX = 15
    
; Test NEG
    MOV ECX, 100
    NEG ECX           ; ECX = -100 (0xFFFFFF9C)
    
; Test NOP
    NOP
    
; Test CLC and STC
    CLC               ; CF = 0
    STC               ; CF = 1
    
; Test DEC and LOOP
    MOV ECX, 5
loop_start:
    DEC EAX           ; EAX: 15 -> 14 -> 13 -> 12 -> 11 -> 10
    LOOP loop_start   ; Loop 5 times

; Final result: EAX = 10
    HLT
