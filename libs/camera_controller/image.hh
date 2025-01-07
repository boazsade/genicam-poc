#pragma once
#include <stdint.h>
#include <iosfwd>
#include <vector>

namespace camera {

// For the full list of supported types you should looked at
// /opt/Vimba_6_0/VimbaC/Include/VmbCommonTypes.h
// This is just a partial list that we have now, but you can add more.
enum class PixelFormat : uint32_t {
    Mono8,
    Mono10,
    Mono10P,
    Mono12,
    Mono12Packet,
    Mono12P,
    Mono14,
    Mono16,
    RawRGGB8,   // This is the default raw format that we are using
    RawGR8, 
    RawGB8,
    RawBG8,
    RGB8,
    BGR8,
    ARGB8,
    RGBA8,
    BGRA8,
    YUV411,
    YUV422,
    YUV444,
};
auto operator << (std::ostream& os, PixelFormat ea) -> std::ostream&;
auto to_string(PixelFormat ea) -> const char*;

// none owning image
struct ImageView {
    uint32_t size{0};
    uint32_t width{0};
    uint32_t height{0};
    unsigned long long number{0};
    const uint8_t* data{nullptr};
    PixelFormat type{PixelFormat::RawRGGB8};

    constexpr ImageView() = default;
    constexpr ImageView(uint32_t s, uint32_t w, uint32_t h, unsigned long long n, const uint8_t* d, PixelFormat pf) :
            size{s}, width{w}, height{h}, number{n}, data{d}, type{pf} {

    }
};

auto operator << (std::ostream& os, const ImageView& iv) -> std::ostream&;

// Data owning image
struct Image {
    uint32_t width{0};
    uint32_t height{0};
    unsigned long long number{0};
    std::vector<uint8_t> data;
    PixelFormat type{PixelFormat::RawRGGB8};

    constexpr auto size() const -> std::size_t {
        return data.size();
    }

    constexpr auto empty() const -> bool {
        return data.empty();
    }

    Image() = default;
    Image(const ImageView& from) : 
        width{from.width}, height{from.height}, number{from.number},
        data(construct(from)), type{from.type} {

    }

private:
    static auto construct(const ImageView& from) -> std::vector<uint8_t> {
        return from.data ? 
            std::vector<uint8_t>(from.data, from.data + from.size) :
            std::vector<uint8_t>{};
    }
};
auto operator << (std::ostream& os, const Image& iv) -> std::ostream&;

}   // end of camera namespace