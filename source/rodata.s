.section ".rodata.module_name"	
.word 0
.word 8 //Length of module name
.asciz "ParkRTLD" //module name

.section ".rodata.mod0"
.global __nx_mod0
__nx_mod0:
//    .word 0
//    .word 8
    .ascii "MOD0"
    .word  __dynamic_start__    - __nx_mod0
    .word  __bss_start__        - __nx_mod0
    .word  __bss_end__          - __nx_mod0
    .word  0
    .word  0
    .word  g_runtime_module  - __nx_mod0 /* .word _ZN4rtld19__nx_module_runtimeE - __nx_mod0 */