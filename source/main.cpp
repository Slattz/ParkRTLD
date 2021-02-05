#include "types.hpp"
#include "sdk_funcs.hpp"
#include "svc.hpp"

typedef void (*ACNH100_121)(uintptr_t, uintptr_t, void (*)(void), void (*)(void));
typedef void (*ACNH130_170)(uintptr_t, uintptr_t, void (*)(void), void (*)(void), void (*)(void));

extern "C" void __rtld_init(void) {
    //svcOutputDebugString("__rtld_init called\n", 20);
}
extern "C" void __rtld_fini(void) {
    //svcOutputDebugString("__rtld_fini called\n", 20);
}

extern "C" void __rtld_call_start(uintptr_t mainThreadHandle, uintptr_t argAddr,
                void (*call_enable_exception_handler)(),
                void (*call_init_modules)(),
                void (*call_fini_modules)()
            )
{
    if (static_cast<ACNH100_121>(nn::init::Start)) {
        #ifdef DEBUG_EXECUTION
        svcOutputDebugString("Using ACNH version 1.0.0 - 1.2.1!", 34);
        #endif
        nn::init::Start(mainThreadHandle, argAddr, call_enable_exception_handler, call_init_modules);
    }

    else if (static_cast<ACNH130_170>(nn::init::Start)) {
        #ifdef DEBUG_EXECUTION
        svcOutputDebugString("Using ACNH version 1.3.0 - 1.7.0!", 34);
        #endif
        nn::init::Start(mainThreadHandle, argAddr, call_enable_exception_handler, call_init_modules, call_fini_modules);
    }

    else {
        svcOutputDebugString("Unknown ACNH Version!!", 23);
        svcBreak(0,0,0);
    }
}