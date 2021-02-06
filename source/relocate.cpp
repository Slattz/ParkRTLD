#include "types.hpp"
#include "utils.hpp"
#include "RoModule.hpp"
#include "LoadList.hpp"
#include "svc.hpp"
#include <cstring>

#define READU32(addr) *(u32 *)(addr)

__attribute__((section(".bss"))) RoModule g_runtime_module;
LoadList g_pManualLoadList; //name from nnsdk binary
LoadList g_pAutoLoadList; //name from nnsdk binary
bool g_pRoDebugFlag;
__attribute__((section(".data"))) bool g_ExceptionHandlerEnabled;

__attribute__((section(".bss"))) Elf64_Addr (*g_pLookupGlobalManualFunctionPointer)(RoModule* module, const char *str);

Elf64_Addr g_pLookupGlobalAutoFunctionPointer(const char* str) {
    if (g_pAutoLoadList.last != (RoModule *)&g_pAutoLoadList) {
        RoModule* modObj = g_pAutoLoadList.last;

        while (modObj != (RoModule *)&g_pAutoLoadList) {
            Elf64_Sym* symbol = modObj->Lookup(str);
            if (symbol && ELF64_ST_BIND(symbol->st_info)) {
               return reinterpret_cast<Elf64_Addr>(modObj->moduleBaseAddr + symbol->st_value);
            }

            modObj = modObj->prev;
        }
    }

    return 0;
}


constexpr u32 RX_Perm = 5;
constexpr u32 CodeStatic_Type = 3;

constexpr u32 MOD0_Magic = 0x30444F4D;
struct MODHeader { //https://switchbrew.org/wiki/NSO#MOD
    u32 magic;
    u32 off_dynamic;
    u32 off_bss_start;
    u32 off_bss_end;
    u32 off_ehFrameHdr_start;
    u32 off_ehFrameHdr_end;
    u32 off_module_object;
};

/* Oracle Solaris 11.4 Linkers and Libraries Guide
    https://docs.oracle.com/cd/E37838_01/html/E36783/chapter6-42444.html
*/

extern "C" void __rtld_enable_exception_handler(void) {
    g_ExceptionHandlerEnabled = true;
}

extern "C" void __rtld_relocate_self(u8* rtldBaseAddr, Elf64_Dyn* dynamic) {
    Elf64_Xword rel_ent_count = 0; //X8
    Elf64_Xword rela_ent_count = 0; //X9
    Elf64_Xword rela_ent_size = 0; //X10
    Elf64_Addr rela_ent = 0; //X11

    for (const Elf64_Dyn* entry = dynamic; entry->d_tag != DT_NULL; entry++) {
        if (entry->d_tag == DT_RELCOUNT) {
            rel_ent_count = entry->d_un.d_val;
        }

        else if (entry->d_tag == DT_RELACOUNT) {
            rela_ent_count = entry->d_un.d_val;
        }

        else if (entry->d_tag == DT_RELA) {
            rela_ent = reinterpret_cast<Elf64_Addr>(rtldBaseAddr) + entry->d_un.d_ptr;
        }

        else if (entry->d_tag == DT_RELAENT) {
            rela_ent_size = entry->d_un.d_val;
        }
    }

    if (rela_ent_count) {
        u64 offset = 0;
        for (Elf64_Xword i = 0; i < rela_ent_count; i++) {
            Elf64_Rela* entry = reinterpret_cast<Elf64_Rela *>(rela_ent + offset);

            if (ELF64_R_TYPE(entry->r_info) == R_AARCH64_RELATIVE) {
                Elf64_Addr* addr = reinterpret_cast<Elf64_Addr*>(rtldBaseAddr + entry->r_offset);
                *addr = reinterpret_cast<Elf64_Xword>(rtldBaseAddr + entry->r_addend);
            }
            #ifndef EASE_RESTRICTIONS
                else { while(1); } //official rtld only wants aarch64 relative relocations (R_AARCH64_RELATIVE), hangs if anything else found
            #endif

            offset += rela_ent_size;
        }
    }

    #ifndef EASE_RESTRICTIONS
    if (rel_ent_count) { //official rtld only wants RELA types, hangs if REL is found
        while(1);
    }
    #endif
}

extern "C" void __rtld_relocate_others(u8* rtldBaseAddr, Elf64_Dyn* dynamic) {
    g_pManualLoadList.front = (RoModule *)&g_pManualLoadList;
    g_pManualLoadList.last = (RoModule *)&g_pManualLoadList;

    g_pAutoLoadList.front = (RoModule *)&g_pAutoLoadList;
    g_pAutoLoadList.last = (RoModule *)&g_pAutoLoadList;
    
    g_runtime_module.prev = &g_runtime_module;
    g_runtime_module.next = &g_runtime_module;
    g_runtime_module.Init(rtldBaseAddr, 0, dynamic, false);

    RoModule* next_module = g_runtime_module.next;
    g_runtime_module.next = g_pAutoLoadList.front;
    next_module->prev = (RoModule *)&g_pAutoLoadList;

    g_pAutoLoadList.front->prev = &g_runtime_module;
    g_pAutoLoadList.front = next_module;

    u64 address = 0;
    u32 pageInfo;
    MemoryInfo memInfo;

    bool pastLastModule = true;
    while (pastLastModule) {
        Result res = svcQueryMemory(&memInfo, &pageInfo, address);
        if (res) { while(1); }

        if (memInfo.perm == RX_Perm && memInfo.type == CodeStatic_Type) { //Perms must be RX and Memory type must be CodeStatic
            if (memInfo.addr != reinterpret_cast<u64>(rtldBaseAddr)) //skip over rtld module
            {
                u32 off_Magic = READU32(memInfo.addr+4);
                MODHeader* modHeader = reinterpret_cast<MODHeader *>(memInfo.addr + off_Magic);

                if (modHeader->magic != MOD0_Magic) { while(1); }

                u8* off_bssStart = reinterpret_cast<u8*>(modHeader) + modHeader->off_bss_start;
                if (off_bssStart != (u8*)modHeader + modHeader->off_bss_end) { //if there is a bss section
                    memset(off_bssStart, 0, modHeader->off_bss_end - modHeader->off_bss_start);
                }

                RoModule* moduleObj = (RoModule *)((u8*)modHeader + modHeader->off_module_object);
                moduleObj->prev = moduleObj;
                moduleObj->next = moduleObj;

                Elf64_Dyn* dynamicSect = reinterpret_cast<Elf64_Dyn *>((u8*)modHeader + modHeader->off_dynamic);
                moduleObj->Init((u8*)memInfo.addr, 0, dynamicSect, 0);
                moduleObj->Relocate();

                RoModule* nextModObj = moduleObj->next;
                moduleObj->next = g_pAutoLoadList.front;
                nextModObj->prev = (RoModule *)&g_pAutoLoadList;

                g_pAutoLoadList.front->prev = moduleObj;
                g_pAutoLoadList.front = nextModObj;
            }
            #ifdef DEBUG_EXECUTION
            else svcOutputDebugString("Skipped rtld!!", 15);
            #endif
        }

        const u64 nextAddress = memInfo.size + memInfo.addr;
        pastLastModule = nextAddress > address;
        address = nextAddress;
    }

    if (g_pAutoLoadList.last != (RoModule *)&g_pAutoLoadList) {
        RoModule* modObj = g_pAutoLoadList.last;
        while (modObj != (RoModule *)&g_pAutoLoadList) {
            Elf64_Sym* symbol = modObj->Lookup("_ZN2nn2ro6detail15g_pAutoLoadListE");
            if (symbol && ELF64_ST_BIND(symbol->st_info)) {
               Elf64_Addr* addr = reinterpret_cast<Elf64_Addr*>(modObj->moduleBaseAddr + symbol->st_value);
               *addr = reinterpret_cast<Elf64_Xword>(&g_pAutoLoadList);
            }

            symbol = modObj->Lookup("_ZN2nn2ro6detail17g_pManualLoadListE");
            if (symbol && ELF64_ST_BIND(symbol->st_info)) {
               Elf64_Addr* addr = reinterpret_cast<Elf64_Addr*>(modObj->moduleBaseAddr + symbol->st_value);
               *addr = reinterpret_cast<Elf64_Xword>(&g_pManualLoadList);
            }

            symbol = modObj->Lookup("_ZN2nn2ro6detail14g_pRoDebugFlagE");
            if (symbol && ELF64_ST_BIND(symbol->st_info)) {
               Elf64_Addr* addr = reinterpret_cast<Elf64_Addr*>(modObj->moduleBaseAddr + symbol->st_value);
               *addr = reinterpret_cast<Elf64_Xword>(&g_pRoDebugFlag);
            }

            symbol = modObj->Lookup("_ZN2nn2ro6detail34g_pLookupGlobalAutoFunctionPointerE");
            if (symbol && ELF64_ST_BIND(symbol->st_info)) {
               Elf64_Addr* addr = reinterpret_cast<Elf64_Addr*>(modObj->moduleBaseAddr + symbol->st_value);
               *addr = reinterpret_cast<Elf64_Xword>(&g_pLookupGlobalAutoFunctionPointer);
            }

            symbol = modObj->Lookup("_ZN2nn2ro6detail36g_pLookupGlobalManualFunctionPointerE");
            if (symbol && ELF64_ST_BIND(symbol->st_info)) {
               Elf64_Addr* addr = reinterpret_cast<Elf64_Addr*>(modObj->moduleBaseAddr + symbol->st_value);
               *addr = reinterpret_cast<Elf64_Xword>(&g_pLookupGlobalManualFunctionPointer);
            }

            modObj = modObj->prev;
        }

        for (RoModule* modObj = g_pAutoLoadList.last; modObj != (RoModule*)&g_pAutoLoadList; modObj = modObj->prev) {
            modObj->Relocation(true);
        }
        #ifdef DEBUG_EXECUTION
        svcOutputDebugString("\n\nDone Relocation!\n\n", 18);
        #endif
    }
}

extern "C" void __rtld_init_modules(void) {
    if (g_pAutoLoadList.last != (RoModule *)&g_pAutoLoadList) {
        RoModule* modObj = g_pAutoLoadList.front;

        while (modObj != g_pAutoLoadList.last) {
            modObj->CallInit();
            modObj = modObj->next;
        }
    }
}

extern "C" void __rtld_fini_modules(void) {
    if (g_pAutoLoadList.last != (RoModule *)&g_pAutoLoadList) {
        RoModule* modObj = g_pAutoLoadList.last;

        while (modObj != g_pAutoLoadList.front) {
            modObj->CallFini();
            modObj = modObj->prev;
        }
    }
}
