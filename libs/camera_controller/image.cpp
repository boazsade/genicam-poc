#include "image.hh"
#include <iostream>

namespace camera {

auto operator << (std::ostream& os, const ImageView& iv) -> std::ostream& {
    return os << iv.number << ", image size: " << iv.size << " bytes [" << iv.width << " X " << iv.height << "], empty " << (iv.data ? "no" : "yes");
}

auto operator << (std::ostream& os, const Image& iv) -> std::ostream& {
    return os << iv.number << ", image size: " << iv.size() << " bytes [" << iv.width << " X " << iv.height << "], empty " << (iv.empty() ? "no" : "yes");
}

auto to_string(PixelFormat ea) -> const char* {
    switch (ea) {
    case PixelFormat::Mono8:
        return "Mono8";
    case PixelFormat::Mono10:
        return "Mono10";
    case PixelFormat::Mono10P:
        return "Mono10p";
    case PixelFormat::Mono12:
        return "Mono12";
    case PixelFormat::Mono12Packet:
        return "Mono12Packed";
    case PixelFormat::Mono12P:
        return "Mono12p";
    case PixelFormat::Mono14:
        return "Mono14";
    case PixelFormat::Mono16:
        return "Mono16";
    case PixelFormat::RawRGGB8:   // This is the default raw format that we are using
        return "BayerRG8";
    case PixelFormat::RawGR8:
        return "BayerGR8";
    case PixelFormat::RawGB8:
        return "BayerGB8";
    case PixelFormat::RawBG8:
        return "BayerBG8";
    case PixelFormat::RGB8:
        return "Rgb8";
    case PixelFormat::BGR8:
        return "Bgr8";
    case PixelFormat::ARGB8:
        return "Argb8";
    case PixelFormat::RGBA8:
        return "Rgba8";
    case PixelFormat::BGRA8:
        return "Bgra8";
    case PixelFormat::YUV411:
        return "Yuv411";
    case PixelFormat::YUV422:
        return "Yuv422";
    case PixelFormat::YUV444:
        return "Yuv444";
    default:
        return "BayerRG8";
    }
}

auto operator << (std::ostream& os, PixelFormat ea) -> std::ostream& {
    return os << to_string(ea);
}

}       // end of namespace camera