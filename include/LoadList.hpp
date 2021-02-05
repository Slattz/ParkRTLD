#pragma once
#include "RoModule.hpp"

struct LoadList {
    RoModule* front;
    RoModule* last;
};


static_assert(sizeof(LoadList) == 0x10, "LoadList incorrect size!");