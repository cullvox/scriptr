#pragma once

class UniqueID {
public:
    static uint32_t RegisterID(uint32_t id);
    static uint32_t GrabID();
};