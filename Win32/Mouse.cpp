#include "Mouse.h"

#include <exception>

Mouse::Mouse(IDirectInput8* directInput) {
    auto result = directInput->CreateDevice(GUID_SysMouse, &device, NULL);
    DI_CHECK(result, "could not create mouse");

    DIPROPDWORD properties;
    properties.diph.dwSize = sizeof(DIPROPDWORD);
    properties.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    properties.diph.dwObj = 0;
    properties.diph.dwHow = DIPH_DEVICE;
    properties.dwData = DIPROPAXISMODE_REL;
    result = device->SetProperty(DIPROP_AXISMODE, &properties.diph);
    DI_CHECK(result, "could not set mouse properties");

    result = device->SetDataFormat(&c_dfDIMouse);
    DI_CHECK(result, "could not set mouse data format");

    result = device->Acquire();
    DI_CHECK(result, "could not acquire mouse");
}

Vec2i Mouse::getDelta() {
    Vec2i result = {};
    DIMOUSESTATE state;
    device->GetDeviceState(sizeof(state), &state);
    result.x = state.lX;
    result.y = state.lY;
    return result;
}
