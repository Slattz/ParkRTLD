
.global BindEntry_Trampoline
.type BindEntry_Trampoline, %function

BindEntry_Trampoline:
LDR     X17, [SP]
STR     X29, [SP]

STP     XZR,X8, [SP,#-0x10]!
STP     X7, X6, [SP,#-0x10]!
STP     X5, X4, [SP,#-0x10]!
STP     X3, X2, [SP,#-0x10]!
STP     X1, X0, [SP,#-0x10]!

STP     Q7, Q6, [SP,#-0x20]!
STP     Q5, Q4, [SP,#-0x20]!
STP     Q3, Q2, [SP,#-0x20]!
STP     Q1, Q0, [SP,#-0x20]!

MOV     X29, SP
SUB     X1, X17, X16
SUB     X1, X1, #8
LSR     X1, X1, #3
LDUR    X0, [X16,#-8]
BL      RoModule_CallBind //X0: RoModule Object ; X1: rela_plt index
MOV     X16, X0

LDP     Q1, Q0, [SP],#0x20
LDP     Q3, Q2, [SP],#0x20
LDP     Q5, Q4, [SP],#0x20
LDP     Q7, Q6, [SP],#0x20

LDP     X1, X0, [SP],#0x10
LDP     X3, X2, [SP],#0x10
LDP     X5, X4, [SP],#0x10
LDP     X7, X6, [SP],#0x10
LDP     XZR,X8, [SP],#0x10

LDP     X29, X30, [SP],#0x10
BR      X16