#pragma once
#include "types.hpp"

/* Slimmed svc.h from libnx */

/// Memory information structure.
typedef struct {
    u64 addr;            ///< Base address.
    u64 size;            ///< Size.
    u32 type;            ///< Memory type (see lower 8 bits of \ref MemoryState).
    u32 attr;            ///< Memory attributes (see \ref MemoryAttribute).
    u32 perm;            ///< Memory permissions (see \ref Permission).
    u32 device_refcount; ///< Device reference count.
    u32 ipc_refcount;    ///< IPC reference count.
    u32 padding;         ///< Padding.
} MemoryInfo;

/**
 * @brief Query information about an address. Will always fetch the lowest page-aligned mapping that contains the provided address.
 * @param[out] meminfo_ptr \ref MemoryInfo structure which will be filled in.
 * @param[out] pageinfo Page information which will be filled in.
 * @param[in] addr Address to query.
 * @return Result code.
 * @note Syscall number 0x06.
 */
extern "C" Result svcQueryMemory(MemoryInfo* meminfo_ptr, u32 *pageinfo, u64 addr);

/**
 * @brief Breaks execution.
 * @param[in] breakReason Break reason (see \ref BreakReason).
 * @param[in] address Address of the buffer to pass to the debugger.
 * @param[in] size Size of the buffer to pass to the debugger.
 * @return Result code.
 * @note Syscall number 0x26.
 */
extern "C" Result svcBreak(u32 breakReason, uintptr_t address, uintptr_t size);

/**
 * @brief Outputs debug text, if used during debugging.
 * @param[in] str Text to output.
 * @param[in] size Size of the text in bytes.
 * @return Result code.
 * @note Syscall number 0x27.
 */
extern "C" Result svcOutputDebugString(const char *str, u64 size);

/**
 * @brief Returns from an exception.
 * @param[in] res Result code.
 * @note Syscall number 0x28.
 */
extern "C" void NORETURN svcReturnFromException(Result res);

extern "C" void UserExceptionHandler(void);