#include "types.hpp"
#include "sdk_funcs.hpp"
#include "svc.hpp"

typedef void (*ACNH100_121)(uintptr_t, uintptr_t, void (*)(void), void (*)(void)); //ACNH 1.0.0 - 1.2.1
typedef void (*ACNH130_1XX)(uintptr_t, uintptr_t, void (*)(void), void (*)(void), void (*)(void)); //ACNH 1.3.0 - 1.7.0+

extern "C" void __rtld_init(void) {
#ifdef DEBUG_EXECUTION
    svcOutputDebugString("__rtld_init called\n", 20);
#endif
}
extern "C" void __rtld_fini(void) {
#ifdef DEBUG_EXECUTION
    svcOutputDebugString("__rtld_fini called\n", 20);
#endif
}

extern "C" void __rtld_call_start(uintptr_t mainThreadHandle, uintptr_t argAddr,
                void (*call_enable_exception_handler)(),
                void (*call_init_modules)(),
                void (*call_fini_modules)()
            )
{
    if (static_cast<ACNH130_1XX>(nn::init::Start)) { //Check latest version first rather than oldest version
        #if defined(DEBUG_EXECUTION) || defined(PRINT_VERSION)
        svcOutputDebugString("Using ACNH version 1.3.0 - 1.7.0!", 34);
        #endif
        nn::init::Start(mainThreadHandle, argAddr, call_enable_exception_handler, call_init_modules, call_fini_modules);
    }

    else if (static_cast<ACNH100_121>(nn::init::Start)) {
        #if defined(DEBUG_EXECUTION) || defined(PRINT_VERSION)
        svcOutputDebugString("Using ACNH version 1.0.0 - 1.2.1!", 34);
        #endif
        nn::init::Start(mainThreadHandle, argAddr, call_enable_exception_handler, call_init_modules);
    }

    else {
        svcOutputDebugString("Unknown ACNH Version!!", 23);
        svcBreak(0,0,0);
    }
}