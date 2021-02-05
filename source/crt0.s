.extern __nx_mod0

.extern svcReturnFromException
.extern _ZN2nn2os6detail20UserExceptionHandlerEv

.extern __rtld_enable_exception_handler
.extern __rtld_init_modules
.extern __rtld_fini_modules


/* 
	* Relative func offset to absolute pointer, used to workaround ASLR
	* Code uses a BL to the function, then adding X30 (LR) to an offset 
*/
.macro FUNC_REL_ABS name, reg, address
	.word \address - .
\name:
	LDR W\reg, [X30] //X30 is LR, which is set by the BL
	SXTW X\reg, W\reg //signed extend word
	ADD X\reg, X\reg, X30
.endm

.macro GET_GOT_PTR symbol, reg
	ADRP X\reg, :got:\symbol
    LDR X\reg, [X\reg, :got_lo12:\symbol]
.endm

/*
	X0: u64 arg0 (must be 0)
	X1: u32 mainThread (handle to main thread)
*/
.section ".text.crt0","ax"
.global __rtld_entry
__rtld_entry:
    B #0x8
    .word __nx_mod0 - __rtld_entry //offset to MOD0 required to be at 0x4

	CMP X0, #0 //arg0 must be 0
	B.NE __rtld_entry_exception
	MOV W19, W1 //Save the main thread handle in a preserved register
	BL __rtld_entry_normal

FUNC_REL_ABS __rtld_entry_normal, 0, __bss_start__
	BL __rtld_clean_bss

FUNC_REL_ABS __rtld_clean_bss, 2, __bss_end__
	SUB X2, X2, X0
	MOV W1, #0
	BL memset
	BL __rtld_entry_start

FUNC_REL_ABS __rtld_entry_start, 0, __rtld_entry //Gets rtld base address, passes it to __rtld_start
	BL __rtld_start

FUNC_REL_ABS __rtld_start, 1, __dynamic_start__ 
	MOV X20, X0 //X20 is a preserved register, shouldn't get overwritten
	MOV X21, X1 //X21 is a preserved register, shouldn't get overwritten
	BL __rtld_relocate_self //X0: rtld base address (aslr base address), X1: Elf64_Dyn
	MOV X0, X20
	MOV X1, X21
	BL __rtld_relocate_others //X0: rtld base address (aslr base address), X1: Elf64_Dyn
	MOV W0, W19 //Restore main thread handle
	GET_GOT_PTR __argdata__, 1
	BL __rtld_prep_arg2


FUNC_REL_ABS __rtld_prep_arg2, 2, __rtld_enable_exception_handler
	BL __rtld_prep_arg3

FUNC_REL_ABS __rtld_prep_arg3, 3, __rtld_init_modules
	BL __rtld_prep_arg4

FUNC_REL_ABS __rtld_prep_arg4, 4, __rtld_fini_modules
	BL __rtld_call_start
	B .	//Infinite Loop


.section ".text.crt0","ax"
.global __rtld_entry_exception
__rtld_entry_exception:
	BL __rtld_handle_entry_exception

FUNC_REL_ABS __rtld_handle_entry_exception, 2, __bss_start__ //should be g_IsExceptionHandlerReady
	LDR W2, [X2]
	CBZ W2, _unhandled_exception

	GET_GOT_PTR _ZN2nn2os6detail20UserExceptionHandlerEv, 2
	CBZ X2, _unhandled_exception
	BR X2

_unhandled_exception:
	MOV X0, #0xF801
	BL svcReturnFromException
	B . //Infinite Loop