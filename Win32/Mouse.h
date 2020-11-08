#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "MathLib.h"

struct Mouse {
    LPDIRECTINPUTDEVICE8 device;

    Mouse(IDirectInput8*);
    Vec2i getDelta();
};
