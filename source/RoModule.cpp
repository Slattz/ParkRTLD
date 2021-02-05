#include "RoModule.hpp"
#include "utils.hpp"
#include "svc.hpp"
#include <cstring>

#ifdef DEBUG_EXECUTION
ALWAYS_INLINE void OutputAndBreak(const char* str, u32 breakReason, uintptr_t address, uintptr_t size) {
    svcOutputDebugString((str), strlen((str)));
    svcBreak(breakReason, address, size);
}
#else
ALWAYS_INLINE void OutputAndBreak(const char* str, u32 breakReason, uintptr_t address, uintptr_t size) {
    (void)str;
    svcBreak(breakReason, address, size);
}
#endif

extern "C" void BindEntry_Trampoline(void);

void RoModule::Init(u8* moduleBaseAddr, u64 arg1_0, Elf64_Dyn *dynamic, bool arg4_0) {
    this->unk_xB8 = arg1_0;
    this->is_rela = false;
    this->unk_xC0 = arg4_0;

    this->moduleBaseAddr = moduleBaseAddr;
    this->dynamic = dynamic;

    this->rela_plt = 0;
    this->rel_rela = nullptr;
    memset(&this->plt_size, 0, (u8*)&this->unk_xB8 - (u8*)&this->plt_size); //Gets optimised into STP instructions

    Elf64_Rela* val_rela_plt = nullptr;
    for (const Elf64_Dyn* entry = dynamic; entry->d_tag != DT_NULL; entry++) {
        switch (entry->d_tag) {
            case DT_RELACOUNT:
                this->relaEnt_count = entry->d_un.d_val; //0x98
                break;

            case DT_RELCOUNT:
                this->relEnt_count = entry->d_un.d_val; //0x90
                break;

            case DT_RELAENT: //same as DT_SYMENT for aarch64
                if (entry->d_un.d_val != sizeof(Elf64_Rela)) {
                    OutputAndBreak("DT_RELAENT != sizeof(Elf64_Rela)\n", 0, 0, 0);
                }
                break;

            case DT_SYMENT: //same as DT_RELAENT for aarch64
                if (entry->d_un.d_val != sizeof(Elf64_Sym)) {
                    OutputAndBreak("DT_SYMENT != sizeof(Elf64_Sym)\n", 0, 0, 0);
                }
                break;

            case DT_RELASZ:
                this->rela_dyn_size = entry->d_un.d_val; //0x80
                break;

            case DT_PLTREL:
                this->is_rela = (entry->d_un.d_val == DT_RELA); //0x30
                if (entry->d_un.d_val != DT_RELA && entry->d_un.d_val != DT_REL) {
                    OutputAndBreak("DT_PLTREL != DT_RELA || DT_REL\n", 0, 0, 0);
                }
                break;

            case DT_PLTRELSZ:
                this->plt_size = entry->d_un.d_val; //0x38
                break;

            case DT_PLTGOT:
                this->got_plt = reinterpret_cast<void**>(moduleBaseAddr + entry->d_un.d_ptr); //0x78
                break;

            case DT_RELENT:
                if (entry->d_un.d_val != sizeof(Elf64_Rel)) {
                    OutputAndBreak("DT_RELENT != sizeof(Elf64_Rel)\n", 0, 0, 0);
                }
                break;

            case DT_REL:
                this->rel = reinterpret_cast<Elf64_Rel*>(moduleBaseAddr + entry->d_un.d_ptr); //0x18
                break;

            case DT_RELA:
                this->rela = reinterpret_cast<Elf64_Rela*>(moduleBaseAddr + entry->d_un.d_ptr); //0x18
                break;

            case DT_HASH: {
                Elf64_Word* hashTable = reinterpret_cast<Elf64_Word*>(moduleBaseAddr + entry->d_un.d_ptr);

                Elf64_Word nbucket = hashTable[0];
                this->nbucket = nbucket; //0xA8
                this->nchain = hashTable[1]; //0xA0

                this->hashBucket = &hashTable[2]; //0x50
                this->hashChain = &hashTable[2 + nbucket]; //0x58
            }
                break;

            case DT_STRTAB:
                this->dynstrTable = reinterpret_cast<char*>(moduleBaseAddr + entry->d_un.d_ptr); //0x60
                break;

            case DT_SYMTAB:
                this->dynsymTable = reinterpret_cast<Elf64_Sym*>(moduleBaseAddr + entry->d_un.d_ptr); //0x68
                break;

            case DT_STRSZ:
                this->dynstrTable_size = entry->d_un.d_val; //0x70
                break;

            case DT_INIT:
                this->initFunc = reinterpret_cast<void (*)()>(moduleBaseAddr + entry->d_un.d_ptr); //0x40
                break;

            case DT_FINI:
                this->finiFunc = reinterpret_cast<void (*)()>(moduleBaseAddr + entry->d_un.d_ptr); //0x48
                break;

            case DT_RELSZ:
                this->rel_dyn_size = entry->d_un.d_val; //0x88
                break;

            case DT_JMPREL:
                val_rela_plt = reinterpret_cast<Elf64_Rela*>(moduleBaseAddr + entry->d_un.d_ptr); //0x10
                break;

            case DT_SONAME:
                this->off_soname = entry->d_un.d_val; //0xB0
                break;

            case DT_NEEDED:
            case DT_RPATH:
            case DT_SYMBOLIC:
            case DT_DEBUG:
            case DT_TEXTREL:
                break;
        }
    }

    //case DT_NULL
    this->rela_plt = val_rela_plt;
    this->ArchitectureData = 0;
}

void RoModule::Relocate() {
#ifndef EASE_RESTRICTIONS
    if (this->relEnt_count) {
        for (Elf64_Xword i = 0; i < this->relEnt_count; i++) {
            OutputAndBreak("RoModule::Relocate: Has Rel!\n", 0, 0, 0); //official rtld only wants RELA types, hangs if REL is found
        }
    }
#endif

    if (this->relaEnt_count) {
        u64 offset = 0;
        for (Elf64_Xword i = 0; i < this->relaEnt_count; i++) {
            Elf64_Rela* entry = reinterpret_cast<Elf64_Rela *>((u8*)this->rela + offset);

            if (ELF64_R_TYPE(entry->r_info) == R_AARCH64_RELATIVE) {
                Elf64_Addr* addr = reinterpret_cast<Elf64_Addr*>(this->moduleBaseAddr + entry->r_offset);
                *addr = reinterpret_cast<Elf64_Xword>(this->moduleBaseAddr + entry->r_addend);
            }

            /* Elf64_Rela size already verified in DT_RELAENT case in ::Init, so it uses sizeof here instead of this->rela_dyn_size */
            offset += sizeof(Elf64_Rela);
        }
    }
}

ALWAYS_INLINE u32 elfHash(const u8* name) {
    u32 h = 0, g;
 
    while (*name) {
        h = (h << 4) + *name++;
        if ((g = (h & 0xf0000000)))
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

Elf64_Sym* RoModule::Lookup(const char* symbol) {
    u32 hash = elfHash((u8*)symbol);

    for (u32 i = this->hashBucket[hash % this->nbucket]; i; i = this->hashChain[i]) {
        Elf64_Sym* sym = &this->dynsymTable[i];

        if (ELF64_ST_TYPE(sym->st_info) != STT_FILE) {
            const bool skipIndex = (sym->st_shndx == SHN_COMMON || sym->st_shndx == SHN_UNDEF);
            if (!skipIndex && strcmp(symbol, &this->dynstrTable[sym->st_name]) == 0) {
                return sym;
            }
        }
    }

    return nullptr;
}

bool RoModule::ResolveSym(Elf64_Addr* symbolAddr, Elf64_Sym* symbol) {
    const char* str = &this->dynstrTable[symbol->st_name];
    #ifdef DEBUG_EXECUTION
    svcOutputDebugString(str, strlen(str)); //Print the symbol to be resolved
    #endif
    if (ELF64_ST_VISIBILITY(symbol->st_other) != STV_DEFAULT) {
        Elf64_Sym* sym = this->Lookup(str);
        if (sym) {
            Elf64_Addr value = reinterpret_cast<Elf64_Addr>(this->moduleBaseAddr + sym->st_value);
            if (value) {
                *symbolAddr = value;
                return true;
            }
        }
    }
    else {
        Elf64_Addr addr = g_pLookupGlobalAutoFunctionPointer(str);
        if (addr == 0 && g_pLookupGlobalManualFunctionPointer != NULL) { //set to null when clearing .bss
            addr = g_pLookupGlobalManualFunctionPointer(this, str);
        }

        if (addr) {
            *symbolAddr = addr;
            return true;
        }
    }

    if (ELF64_ST_BIND(symbol->st_info) == STB_WEAK) { //official code is ((st_info&0xF0) == 0x20), probably just compiler optimisation
            *symbolAddr = 0;
            return true;
    }

    return false;
}

ALWAYS_INLINE void OutputDebugString_Helper(const char* str) {
    size_t len = strlen(str);
    svcOutputDebugString(str, len);
}

ALWAYS_INLINE void rtld_OutputDebugString(const char* str) {
    OutputDebugString_Helper("[rtld] warning: unresolved symbol = '");
    OutputDebugString_Helper(str);
    OutputDebugString_Helper("'\n");
}

void RoModule::Relocation(bool lazyGotPlt) {
#ifndef EASE_RESTRICTIONS
    if (this->relEnt_count < this->rel_dyn_size/sizeof(Elf64_Rel)) {
        for (Elf64_Xword i = this->relEnt_count; i < this->rel_dyn_size/sizeof(Elf64_Rel); i++) {
            OutputAndBreak("RoModule::Relocation: Has Rel!\n", 0, 0, 0); //official rtld only wants RELA types, hangs if REL is found
        }
    }
#endif

    if (this->relaEnt_count < this->rela_dyn_size/sizeof(Elf64_Rela)) {
        for (Elf64_Xword i = this->relaEnt_count; i < this->rela_dyn_size/sizeof(Elf64_Rela); i++) {
            Elf64_Rela* entry = &this->rela[i];

            const u32 type = ELF64_R_TYPE(entry->r_info);
            const u32 sym = ELF64_R_SYM(entry->r_info);

            if (type == R_AARCH64_ABS64 || type == R_AARCH64_ABS32 || type == R_AARCH64_GLOB_DAT) {
                Elf64_Addr symbolAddr;
                Elf64_Sym* symb = &this->dynsymTable[sym];
                

                if (this->ResolveSym(&symbolAddr, symb)) {
                    Elf64_Addr* addr = reinterpret_cast<Elf64_Addr*>(this->moduleBaseAddr + entry->r_offset);
                    *addr = static_cast<Elf64_Xword>(symbolAddr + entry->r_addend);
                }

                else if (g_pRoDebugFlag) {
                    rtld_OutputDebugString(&this->dynstrTable[symb->st_name]);
                }
            }

        #ifndef EASE_RESTRICTIONS
            else if (type == R_AARCH64_COPY) {
                OutputAndBreak("RoModule::Relocation: type == R_AARCH64_COPY!\n", 0, 0, 0);
            }
        #endif
        }
    }

    if (this->is_rela && this->plt_size >= sizeof(Elf64_Rela)) {
        for (Elf64_Xword i = 0; i < this->plt_size/sizeof(Elf64_Rela); i++) {
            Elf64_Rela* entry = &this->rela_plt[i];

            const u32 type = ELF64_R_TYPE(entry->r_info);
            const u32 sym = ELF64_R_SYM(entry->r_info);

            if (type == R_AARCH64_JUMP_SLOT) {
                Elf64_Addr* offset = (Elf64_Addr *)(this->moduleBaseAddr + entry->r_offset);
                Elf64_Addr addr = reinterpret_cast<Elf64_Addr>(this->moduleBaseAddr + (*offset));

                if (this->ArchitectureData) {
                    if (this->ArchitectureData != addr) {
                        OutputAndBreak("RoModule::Relocation: this->ArchitectureData != addr!\n", 0, 0, 0);
                    }
                }
                else {
                    this->ArchitectureData = addr;
                }

                if (lazyGotPlt) {
                    *offset = addr;
                }

                else {
                    Elf64_Addr symbolAddr;
                    Elf64_Sym* symb = &this->dynsymTable[sym];

                    if (this->ResolveSym(&symbolAddr, symb)) {
                        *offset = static_cast<Elf64_Xword>(addr + entry->r_addend);
                    }

                    else if (g_pRoDebugFlag) {
                        rtld_OutputDebugString(&this->dynstrTable[symb->st_name]);
                        *offset = addr; //Does lazy init on debug anyways
                    }
                }
            }
        }
    }

#ifndef EASE_RESTRICTIONS
    else if (this->plt_size >= sizeof(Elf64_Rel)) {
        for (Elf64_Xword i = 0; i < this->plt_size/sizeof(Elf64_Rel); i++) {
            OutputAndBreak("RoModule::Relocation: Has Rel Plt!\n", 0, 0, 0); //official rtld only wants RELA types, hangs if REL is found
        }
    }
#endif

    if (this->got_plt) {
      this->got_plt[1] = this;
      this->got_plt[2] = (void*)BindEntry_Trampoline;
    }
}

Elf64_Addr RoModule::BindEntry(u64 index) {
    Elf64_Addr ret = 0;
    
    if (this->is_rela) {
        Elf64_Rela* entry = &this->rela_plt[index];

        Elf64_Addr symbolAddr;
        Elf64_Sym* symb = &this->dynsymTable[ELF64_R_SYM(entry->r_info)];

        if (this->ResolveSym(&symbolAddr, symb)) {
            if (symbolAddr) {
                ret = symbolAddr + entry->r_addend;
            }
        }

        else if (g_pRoDebugFlag) {
            rtld_OutputDebugString(&this->dynstrTable[symb->st_name]);
            ret = 0;
        }

        Elf64_Addr* addr = reinterpret_cast<Elf64_Addr*>(this->moduleBaseAddr + entry->r_offset);
        *addr = ret;
    }

    else {
        OutputAndBreak("RoModule::BindEntry: this->is_rela is false!\n", 0, 0, 0); //official rtld only wants RELA types, hangs if REL is found
        ret = 0;
    }

    return ret;
}

extern "C" Elf64_Addr RoModule_CallBind(RoModule* module, u64 index) {
    return module->BindEntry(index);
}

void RoModule::CallInit() {
    if (this->initFunc) {
        this->initFunc();
    }
}

void RoModule::CallFini() {
    if (this->finiFunc) {
        this->finiFunc();
    }
}