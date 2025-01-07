#include "camera.hh"
#ifdef BUILD_WITH_VIMBA_SDK
#   include "vimba/internal_settings.hpp"
#   include "vimba/cameras_impl.hpp"
#endif
#include "log/logging.h"
#include <iostream>

namespace camera {
namespace {


}       // end of local namespace

///////////////////////////////////////////////////////////////////////////////
#ifdef BUILD_WITH_VIMBA_SDK
//using IdleCamera =  vimba_sdk::IdleModeCamera;
//using CapturingCamera = vimba_sdk::CaptureModeCamera;
using vimba_sdk::do_capture_once;
using vimba_sdk::async_capture_impl;
using vimba_sdk::map_pixel_type;
using vimba_sdk::start_acquisition;
using vimba_sdk::do_software_trigger;
struct IdleCamera : vimba_sdk::IdleModeCamera {
    using vimba_sdk::IdleModeCamera::IdleModeCamera;
};

struct CapturingCamera : vimba_sdk::CaptureModeCamera {
    using vimba_sdk::CaptureModeCamera::CaptureModeCamera;
};

#endif  // BUILD_WITH_VIMBA_SDK
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// API implementation
///////////////////////////////////////////////////////////////////////////////

auto From(std::shared_ptr<IdleCamera>&& cam) -> std::shared_ptr<CapturingCamera> {
    auto ret{std::make_shared<CapturingCamera>(std::move(*cam))};
    cam.reset();
    return ret;
}

auto From(IdleCamera&& cam) -> std::shared_ptr<CapturingCamera> {
    auto ret{std::make_shared<CapturingCamera>(std::move(cam))};
    return ret;
}

auto Back(std::shared_ptr<CapturingCamera>&& cam) -> std::shared_ptr<IdleCamera> {
    auto ret{std::make_shared<IdleCamera>(std::move(cam->camera))};
    cam.reset();
    return ret;
}

auto create(const Context& ctx, const DeviceInfo& dev_id) -> std::shared_ptr<IdleCamera> {
    //return IdleCamera::TryNew(ctx, dev_id);
    auto obj{IdleCamera::TryNew(ctx, dev_id)};
    if (obj) {
        return std::shared_ptr<IdleCamera>(static_cast<IdleCamera*>(obj.release()));
    }
    return {};
}


auto set_capture_type(IdleCamera& camera, PixelFormat pixel_format) -> bool {
    return camera.set_value("PixelFormat", map_pixel_type(pixel_format));
}


auto set_software_trigger(IdleCamera& camera) -> bool {
    return camera.set_value("TriggerMode", "On") &&
           camera.set_value("TriggerSource", "Software");
}

auto set_hardware_trigger(IdleCamera& camera, HardWareTriggerSource src, ActivationMode am) -> bool {
    

    return camera.set_value( "TriggerMode", "On") &&
            camera.set_value("TriggerActivation", to_string(am)) &&
            camera.set_value("TriggerSource", to_string(src));

}


auto get_frame_size(IdleCamera& camera) -> std::optional<int64_t> {
    if (auto v = camera.get_value<VmbInt64_t>("PayloadSize"); v) {
        return int64_t{v.value()};
    }
    return {};
}


auto set_acquisition_mode(IdleCamera& camera, AcquisitionMode mode) -> bool {
    return camera.set_value("AcquisitionMode", to_string(mode));
}


auto set_exposure_mode(IdleCamera& camera, ExposureMode mode) -> bool {
    return camera.set_value( "ExposureMode", to_string(mode));
}

auto auto_exposure(IdleCamera& camera, bool once_flag) -> bool {
    return set_exposure_mode(camera, ExposureMode::Timed) &&
        camera.set_value( "ExposureAuto", (once_flag ? "Once" : "Continuous"));
}


auto manual_exposure(IdleCamera& camera, double time) -> bool {
    return set_exposure_mode(camera, ExposureMode::Off) &&
        camera.set_value( "ExposureAuto", "Off") && 
        camera.set_value( "ExposureTime", time);
}


auto set_auto_whitebalance(IdleCamera& camera, bool on, bool continues) -> bool {
    if (on) {
        if (continues) {
            return camera.set_value( "BalanceWhiteAuto", "Continuous");
        }
        return camera.set_value( "BalanceWhiteAuto", "Once");
    }
    return camera.set_value( "BalanceWhiteAuto", "Off");
}


auto set_default_software_mode(IdleCamera& camera, ActivationMode am) -> bool {
    LOG(INFO) << "Setting the camera to use trigger by software" << ENDL;

    return set_capture_type(camera, PixelFormat::RawRGGB8) &&
        set_software_trigger(camera) &&
        set_acquisition_mode(camera, AcquisitionMode::Continuous) &&
        auto_exposure(camera, false) && camera.set_value("TriggerActivation", to_string(am)) &&
        set_auto_whitebalance(camera, true, false);
}


auto set_default_hardware_mode(IdleCamera& camera, HardWareTriggerSource source, ActivationMode am) -> bool {
    LOG(INFO) << "Setting the camera to be triggered by hardware" << ENDL;
    
    return set_capture_type(camera, PixelFormat::RawRGGB8) &&
        set_hardware_trigger(camera, source, am) &&
        set_acquisition_mode(camera, AcquisitionMode::Continuous) &&
        auto_exposure(camera, false) &&
        set_auto_whitebalance(camera, true, false);
}

///////////////////////////////////////////////////////////////////////////////
// Implement the capture mode

auto capture_once(CapturingCamera& camera, uint32_t timeout) -> std::optional<Image> {
    return do_capture_once(camera, timeout);
}

auto capture_one(CapturingCamera& camera, uint32_t timeout, CaptureContext& context) -> std::optional<ImageView> {
    return context.read(camera, timeout);
}

auto make_capture_context() -> capture_context_t {
    return make_capture_context_impl();
}

auto async_capture(AsyncCaptureContxt& context, CapturingCamera& camera, int queue_size) -> bool {
    return async_capture_impl(context, camera, queue_size);
}

auto make_async_context(CapturingCamera& camera, frame_processing_f&& process_f, std::stop_token cancellation) -> async_context_t {
    return std::make_shared<AsyncCaptureContxt>(camera.camera, std::move(process_f), std::move(cancellation));
}

auto make_software_context(IdleCamera& camera, frame_processing_f&& process_f, std::stop_token cancellation, int queue_size) -> software_context_t {
    // set the device so that we can trigger with source trigger before we are creating this context.
    // Note that if this is single mode and not Continuous you would need to trigger for each frame.
    if (auto image_size = get_frame_size(camera); image_size && set_software_trigger(camera)) {
        try {
            return std::make_shared<SoftwareCaptureContxt>(camera.camera, std::move(process_f), std::move(cancellation), queue_size, image_size.value());
        } catch (const std::exception& e) {
            LOG(ERROR) << "Failed to create the context: " << e.what() << ENDL;
            return {};
        }
    }
    LOG(ERROR) << "failed to generate software context - trigger mode or trigger source setting failed!" << ENDL;
    return {};
}

auto async_software_capture(SoftwareCaptureContxt&, CapturingCamera& camera) -> bool {
    LOG(INFO) << "starting software trigger" << ENDL;
    return camera.start_acquisition() && camera.trigger();
}

auto async_software_capture_one(SoftwareCaptureContxt& context, CapturingCamera& camera) -> bool {
    return camera.trigger_once();
}

auto stop_acquisition(SoftwareCaptureContxt& context, CapturingCamera& camera) -> bool {
    if (camera.stop_acquisition()) {
        context.stop();
        LOG(INFO) << "successfully stopped the software acquisition" << ENDL;
        return true;
    }
    LOG(ERROR) << "failed to stop the acquisition" << ENDL;
    return false;
}


}       // end of namespace camera