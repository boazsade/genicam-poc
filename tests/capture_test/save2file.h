#pragma once
#include <stdint.h>

struct ImageBase {
    uint32_t size{0};
    uint32_t width{0};
    uint32_t height{0};
    unsigned long long number{0};
    uint8_t* data{nullptr};
};

auto do_save(ImageBase image) -> void;