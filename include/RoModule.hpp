#pragma once
#include "types.hpp"
#include <elf.h>

// nn::ro::detail::RoModule                               !!!!!  possibly inherits LoadList????  !!!!!!
class RoModule { //A Doubly Linked List with module info
public:
    RoModule* next;    //0x0, 8 bytes ptr, next module, set in func @ 0x4C0+4C
    RoModule* prev;    //0x8, 8 bytes ptr, prev module, set in func @ 0x4C0+4C
    Elf64_Rela* rela_plt; //0x10, 8 bytes ptr, X21 in func @ 0xA60+2AC
    union { //0x18, 8 bytes ptr, X21 in func @ 0xA60+C0
        Elf64_Rel* rel;
        Elf64_Rela* rela;
        void* rel_rela;
    };
    u8* moduleBaseAddr; //0x20, 8 bytes ptr, X1 arg in func @ 0xA60+50
    Elf64_Dyn* dynamic; //0x28, 8 bytes ptr, X3 arg in func @ 0xA60+50
    bool is_rela; //0x30, 1 byte, set to 0 in func @ 0xA60+38
    u8 pad_x31[7]; //0x31, 7 bytes, padding
    Elf64_Xword plt_size; //0x38, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    void (*initFunc)(void); //0x40, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    void (*finiFunc)(void); //0x48, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    Elf64_Word* hashBucket; //0x50, 8 bytes ptr, set to 0 in func @ 0xA60+54 to 0xA60+74 
    Elf64_Word* hashChain; //0x58, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    char* dynstrTable;      //0x60, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    Elf64_Sym* dynsymTable; //0x68, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    Elf64_Xword dynstrTable_size; //0x70, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    void** got_plt; //0x78, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    Elf64_Xword rela_dyn_size; //0x80, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    Elf64_Xword rel_dyn_size; //0x88, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    Elf64_Xword relEnt_count; //0x90, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    Elf64_Xword relaEnt_count; //0x98, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    Elf64_Xword nchain; //0xA0, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    Elf64_Xword nbucket; //0xA8, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    Elf64_Xword off_soname; //0xB0, 8 bytes, set to 0 in func @ 0xA60+54 to 0xA60+74 
    u64 unk_xB8;  //0xB8, 8 bytes, X2 arg in func @ 0xA60+2C
    bool unk_xC0; //0xC0, 1 byte , W4 arg in func @ 0xA60+44
    u8 pad_xC1[7]; //0xC1, 7 bytes, padding
    Elf64_Xword ArchitectureData; //0xC8, 8 bytes, set to 0 in func @ 0xA60+2B0, "ArchitectureData" according to nnsdk binary symbols

    void Init(u8* moduleBaseAddr, u64 arg1_0, Elf64_Dyn *dynamic, bool arg4_0);
    void Relocate();
    Elf64_Sym* Lookup(const char* symbol); //Lookup the symbol by name (nnsdk: nn::ro::detail::RoModule::Lookup)
    void Relocation(bool lazyGotPlt);

    Elf64_Addr BindEntry(u64 index);
    void CallInit();
    void CallFini();

private:
    bool ResolveSym(Elf64_Addr* symbolAddr, Elf64_Sym* symbol);
};

static_assert(sizeof(RoModule) == 0xD0, "RoModule incorrect size!");

extern "C" Elf64_Addr RoModule_CallBind(RoModule* module, u64 index);

extern bool g_pRoDebugFlag;
extern Elf64_Addr g_pLookupGlobalAutoFunctionPointer(const char* str);
extern Elf64_Addr (*g_pLookupGlobalManualFunctionPointer)(RoModule* module, const char *str);