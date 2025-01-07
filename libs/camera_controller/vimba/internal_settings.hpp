#pragma once
#include "log/logging.h"
#include "vmb_common/ErrorCodeToMessage.h"
#include <VimbaCPP/Include/VimbaCPP.h>

namespace camera {
namespace vimba_sdk {

using namespace AVT::VmbAPI;

template<typename Value>
auto set_value_impl(CameraPtr& camera, const char* key, Value val) -> bool {
    FeaturePtr features;
    if (auto e = camera->GetFeatureByName(key, features); e != VmbErrorSuccess) {
        LOG(ERROR) << "failed to get exposure mode feature from the camera: " << ErrorCodeToMessage(e) << ENDL;
        return false;
    }
    if (auto e = features->SetValue(val); e != VmbErrorSuccess) {
        LOG(ERROR) << "failed to set value " << val << " for " << key << ": " << ErrorCodeToMessage(e) << ENDL;
        return false;
    }
    return true;
}

template<typename Value>
auto get_value_impl(CameraPtr& camera, const char* key) -> std::optional<Value> {
    FeaturePtr feature;
    if (auto error = camera->GetFeatureByName(key, feature); error != VmbErrorSuccess) {
        LOG(WARNING) << "failed to read payload size: " << ErrorCodeToMessage(error) << ENDL;
        return {};
    }
    Value pl;
    if (auto error = feature->GetValue(pl); error != VmbErrorSuccess) {
        LOG(WARNING) << "failed to get payload size: " << ErrorCodeToMessage(error) << ENDL;
        return {};
    }
    return pl;
}

auto set_comm_speed(CameraPtr& camera) -> bool {
    FeaturePtr features;
    if (camera->GetFeatureByName("GVSPAdjustPacketSize", features) != VmbErrorSuccess) {
        LOG(ERROR) << "failed to get packet size feature from the camera" << ENDL;
        return false;
    }
    if (features->RunCommand() != VmbErrorSuccess) {
        LOG(ERROR) << "failed to run feature command" << ENDL;
        return false;
    }
    bool done{false};
    while (features->IsCommandDone(done) == VmbErrorSuccess && !done) {
    }
    LOG(INFO) << "we have " << (done ? "successfully" : "failed") << " set the feature comm speed" << ENDL;
    return done;
}
constexpr auto type_map(VmbPixelFormatType from) -> PixelFormat {
    switch (from) {
    case VmbPixelFormatMono8:
        return PixelFormat::Mono8;
    case VmbPixelFormatMono10:
        return PixelFormat::Mono10;
    case VmbPixelFormatMono10p:
        return PixelFormat::Mono10P;;
    case VmbPixelFormatMono12:
        return PixelFormat::Mono12;
    case VmbPixelFormatMono12Packed:
        return PixelFormat::Mono12Packet;
    case VmbPixelFormatMono12p:
        return PixelFormat::Mono12P;
    case VmbPixelFormatMono14:
        return PixelFormat::Mono14;
    case VmbPixelFormatMono16:
        return PixelFormat::Mono16;
    case VmbPixelFormatBayerRG8:   // This is the default raw format that we are using
        return PixelFormat::RawRGGB8;
    case VmbPixelFormatBayerGR8:
        return PixelFormat::RawGR8;
    case VmbPixelFormatBayerGB8:
        return PixelFormat::RawGB8;
    case VmbPixelFormatBayerBG8:
        return PixelFormat::RawBG8;
    case VmbPixelFormatRgb8:
        return PixelFormat::RGB8;
    case VmbPixelFormatBgr8:
        return PixelFormat::BGR8;
    case VmbPixelFormatArgb8:
        return PixelFormat::ARGB8;
    case VmbPixelFormatBgra8:
        return PixelFormat::BGRA8;
    case VmbPixelFormatYuv411:
        return PixelFormat::YUV411;
    case VmbPixelFormatYuv422:
        return PixelFormat::YUV422;
    case VmbPixelFormatYuv444:
        return PixelFormat::YUV444;
    default:
        return PixelFormat::RawRGGB8;
    }
}

constexpr auto map_pixel_type(PixelFormat from) -> VmbPixelFormatType {
    switch (from) {
    case PixelFormat::Mono8:
        return VmbPixelFormatMono8;
    case PixelFormat::Mono10:
        return VmbPixelFormatMono10;
    case PixelFormat::Mono10P:
        return VmbPixelFormatMono10p;
    case PixelFormat::Mono12:
        return VmbPixelFormatMono12;
    case PixelFormat::Mono12Packet:
        return VmbPixelFormatMono12Packed;
    case PixelFormat::Mono12P:
        return VmbPixelFormatMono12p;
    case PixelFormat::Mono14:
        return VmbPixelFormatMono14;
    case PixelFormat::Mono16:
        return VmbPixelFormatMono16;
    case PixelFormat::RawRGGB8:   // This is the default raw format that we are using
        return VmbPixelFormatBayerRG8;
    case PixelFormat::RawGR8:
        return VmbPixelFormatBayerGR8;
    case PixelFormat::RawGB8:
        return VmbPixelFormatBayerGB8;
    case PixelFormat::RawBG8:
        return VmbPixelFormatBayerBG8;
    case PixelFormat::RGB8:
        return VmbPixelFormatRgb8;
    case PixelFormat::BGR8:
        return VmbPixelFormatBgr8;
    case PixelFormat::ARGB8:
        return VmbPixelFormatArgb8;
    case PixelFormat::RGBA8:
        return VmbPixelFormatRgba8;
    case PixelFormat::BGRA8:
        return VmbPixelFormatBgra8;
    case PixelFormat::YUV411:
        return VmbPixelFormatYuv411;
    case PixelFormat::YUV422:
        return VmbPixelFormatYuv422;
    case PixelFormat::YUV444:
        return VmbPixelFormatYuv444;
    default:
        return VmbPixelFormatBayerRG8;
    }
}

auto TryInto(const FramePtr& from) -> std::optional<ImageView> {
    ImageView image;
    VmbPixelFormatType pixel_format;
    from->GetPixelFormat(pixel_format);
    from->GetFrameID(image.number);
    
    if (!(from->GetImageSize(image.size) == VmbErrorSuccess &&
        from->GetWidth(image.width) == VmbErrorSuccess &&
        from->GetHeight(image.height) == VmbErrorSuccess
        )) {
        LOG(WARNING) << "error reading the image size" << ENDL;
        return {};
    }
    
    if (from->GetImage(image.data) != VmbErrorSuccess) {
        LOG(WARNING) << "failed to read the image data" << ENDL;
        return {};
    }
    image.type = type_map(pixel_format);
    return image;
}

// auto set_activation_mode(CameraPtr& camera, ActivationMode am) -> bool {
//     return set_value(camera, "TriggerActivation", to_string(am));
// }

auto do_acquisition(CameraPtr& camera, uint32_t timeout, FramePtr& frame) -> std::optional<ImageView> {
    if (auto e = camera->AcquireSingleImage(frame, timeout); e != VmbErrorSuccess) {
        LOG(WARNING) << "failed to read image from device after " << timeout << " ms, error: " << ErrorCodeToMessage(e) << ENDL;
        return {};
    }
    VmbFrameStatusType status;
    frame->GetReceiveStatus(status);
    if (status != VmbFrameStatusComplete) {
        LOG(WARNING) << "we don't have the full image after " << timeout << ENDL;
        return {};
    }
    return TryInto(frame);
}

}   // vimba_sdk
}   // end of namespace camera