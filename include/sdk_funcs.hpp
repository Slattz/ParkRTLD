#pragma once
#include "types.hpp"

namespace nn {
    namespace os::detail {
            void WEAK __attribute__((section(".got"))) UserExceptionHandler();
    }  // namespace os::detail

    namespace init {
        void WEAK Start(uintptr_t mainThreadHandle, uintptr_t argAddr, //ACNH 1.0.0 - 1.2.1
                void (*call_enable_exception_handler)(),
                void (*call_init_modules)()
                );
        
        void WEAK Start(uintptr_t mainThreadHandle, uintptr_t argAddr, //ACNH 1.3.0 - 1.7.0+
                void (*call_enable_exception_handler)(),
                void (*call_init_modules)(),
                void (*call_fini_modules)()
                );
    } //namespace init
}  // namespace nn