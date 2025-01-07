#pragma once
#include "cameras_context.hh"
#include "cameras_fwd.hh"
#include "image.hh"
#include "vimba/internal_settings.hpp"
#include "log/logging.h"


namespace camera {

using AVT::VmbAPI::IFrameObserverPtr;
using AVT::VmbAPI::IFrameObserver;
using AVT::VmbAPI::FramePtr;
using AVT::VmbAPI::CameraPtr;

namespace vimba_sdk {
    struct CaptureModeCamera;
    auto register_buffers(CameraPtr& camera, std::vector<FramePtr>& frames, IFrameObserverPtr fop) -> bool;
    auto do_software_trigger(CameraPtr& camera) -> bool;
    auto start_acquisition(CameraPtr& camera) -> bool;
    auto stop_acquisition(CameraPtr& camera) -> bool;
    auto do_software_trigger_once(CameraPtr& camera) -> bool;
}

struct CaptureContext : std::enable_shared_from_this<CaptureContext> {
    auto read(vimba_sdk::CaptureModeCamera& camera, uint32_t timeout) -> std::optional<ImageView>;
private:
    FramePtr frame;
};

auto make_capture_context_impl() -> std::shared_ptr<CaptureContext>;

struct AsyncCaptureContxt : std::enable_shared_from_this<AsyncCaptureContxt> {
    AsyncCaptureContxt(CameraPtr cp, frame_processing_f&& pf, std::stop_token sp) : 
            source{new FrameGrabber(cp, this)},
            processing_op{std::move(pf)}, cancellation{std::move(sp)} {

    }

    ~AsyncCaptureContxt() {
        stop();
    }

    auto get_observer() -> IFrameObserverPtr {
        return source;
    }

    auto stop() -> void {
        dynamic_cast<FrameGrabber*>(source.get())->stop();  // note that this is safe, as we know what we allocated
    }

    auto process(const FramePtr f) {
        if (cancellation.stop_requested()) {
            LOG(INFO) << "got stop request from the application, will cancel capture" << ENDL;
            stop();
            return;
        }
        if (auto frame{vimba_sdk::TryInto(f)}; frame) {
            if (!processing_op(frame.value())) {    // we were told stop
                LOG(INFO) << "processing function notify to stop the processing for frame number " << frame->number << ENDL;
                stop();
            }
        } else {
            LOG(WARNING) << "something is wrong the frame, dropping" << ENDL;
        }
    }

private:
    struct FrameGrabber : IFrameObserver {
        FrameGrabber(CameraPtr cp, AsyncCaptureContxt* self) : IFrameObserver{cp}, patent{self} {

        }

        auto camera_ptr() -> CameraPtr& {
            return m_pCamera;
        }

        auto stop() -> void {
            std::string id;
            camera_ptr()->GetID(id);
            LOG(INFO) << "Stopping the frame processing for camera " << id << ENDL;
            m_pCamera->StopContinuousImageAcquisition();
        }

        void FrameReceived(const FramePtr f) override {
            patent->process(f);
            m_pCamera->QueueFrame(f);
        }

        AsyncCaptureContxt* patent{nullptr};
    };

    IFrameObserverPtr                   source;
    frame_processing_f                  processing_op;
    std::stop_token                     cancellation;
};

struct SoftwareCaptureContxt : AsyncCaptureContxt {
    SoftwareCaptureContxt(CameraPtr cp, frame_processing_f&& pf, std::stop_token sp, int queue_size, int64_t image_size) : 
                AsyncCaptureContxt(std::move(cp), std::move(pf), std::move(sp)), frames(queue_size) {
        for (auto&& f: frames) {
            f = FramePtr(new AVT::VmbAPI::Frame(image_size, AVT::VmbAPI::FrameAllocation_AllocAndAnnounceFrame));
        }
        if (!vimba_sdk::register_buffers(cp, frames, get_observer())) {
            throw std::runtime_error("failed to register the frames to the device, this will result in critical error");
        }

    }

private:
    std::vector<FramePtr> frames;
};

namespace vimba_sdk {

auto run_command(CameraPtr& camera, const char* name) -> bool {
    FeaturePtr feature;
    if (auto e = camera->GetFeatureByName(name, feature); e != VmbErrorSuccess) {
        LOG(WARNING) << "failed to get feature " << name << ": " << ErrorCodeToMessage(e) << ENDL;
        return false;
    }
    if (auto e = feature->RunCommand(); e != VmbErrorSuccess) {
        LOG(WARNING) << "failed to run " << name << ": " << ErrorCodeToMessage(e) << ENDL;
        return false;
    }
    return true;
}

auto do_software_trigger_once(CameraPtr& camera) -> bool {

    FeaturePtr feature;
    const auto run_one_command = [&camera, &feature] (const char* name) -> bool {
        if (auto e = camera->GetFeatureByName(name, feature); e != VmbErrorSuccess) {
            LOG(WARNING) << "failed to get feature " << name << ": " << ErrorCodeToMessage(e) << ENDL;
            return false;
        }
        if (auto e = feature->RunCommand(); e != VmbErrorSuccess) {
            LOG(WARNING) << "failed to run feature " << name << ": " << ErrorCodeToMessage(e) << ENDL;
            return false;
        }
        return true;
    };
    return run_one_command("AcquisitionStart") &&
        run_one_command("TriggerSoftware") &&
        run_one_command("AcquisitionStop");

}

auto do_software_trigger(CameraPtr& camera) -> bool {
    return run_command(camera, "TriggerSoftware");
}

auto start_acquisition(CameraPtr& camera) -> bool {
    return run_command(camera, "AcquisitionStart");
}

auto stop_acquisition(CameraPtr& camera) -> bool {
    return run_command(camera, "AcquisitionStop");
}

auto register_buffers(CameraPtr& camera, std::vector<FramePtr>& frames, IFrameObserverPtr fop) -> bool {

    for (auto&& f: frames) {
        if (auto e = f->RegisterObserver(fop); e != VmbErrorSuccess) {
            LOG(ERROR) <<  "failed to register observer to the frame" << std::endl;
            return -1;
        }
        if (auto e = camera->AnnounceFrame(f); e != VmbErrorSuccess) {
            LOG(ERROR) << "failed to connect frame of to camera queue: " << ErrorCodeToMessage(e) << ENDL;
            return -1;
        }
    }
    if (auto e = camera->StartCapture(); e != VmbErrorSuccess) {
        LOG(ERROR) << "failed to start capture " << ErrorCodeToMessage(e) << ENDL;
        return false;
    }
    for (auto&& f: frames) {
        if (auto e = camera->QueueFrame(f); e != VmbErrorSuccess) {
            LOG(ERROR) << "failed to queue frame to camera queue: " << ErrorCodeToMessage(e) << ENDL;
            return false;
        }
    }
    LOG(INFO) << "successfully registered " << frames.size() << " to the camera" << ENDL;
    return true;
}

struct IdleModeCamera : std::enable_shared_from_this<IdleModeCamera> {
    using Self = IdleModeCamera;

    explicit IdleModeCamera(CameraPtr c) : camera{std::move(c)} {

    }

    static auto TryNew(const Context&, const DeviceInfo& dev_id) -> std::unique_ptr<Self> {
        auto& control{VimbaSystem::GetInstance()};
        CameraPtr camera;
        if (auto e = control.GetCameraByID(dev_id.id.c_str(), camera); e != VmbErrorSuccess) {
            LOG(ERROR) << "failed to get camera '" << dev_id << "' make sure that this camera is connected " << ErrorCodeToMessage(e) << ENDL;
            return {};
        }
        LOG(INFO) << "Trying to open by id " << dev_id.id << ENDL;
        if (auto e = control.OpenCameraByID(dev_id.id.c_str(), VmbAccessModeFull, camera); e != VmbErrorSuccess) {
            LOG(ERROR) << "Failed to open camera! " << ErrorCodeToMessage(e) << ENDL;
            return {};
        }
        if (set_comm_speed(camera)) {
            return std::make_unique<IdleModeCamera>(camera);
        }
        return {};
    }

    template<typename T>
    auto set_value(const char* name, T val) -> bool {
        return set_value_impl(camera, name, val);
    }

    template<typename T>
    auto get_value(const char* name) -> std::optional<T> {
        return get_value_impl<T>(camera, name);
    }

    CameraPtr camera;
};

struct CaptureModeCamera : std::enable_shared_from_this<CaptureModeCamera> {

    explicit CaptureModeCamera(IdleModeCamera&& from) : camera{std::move(from.camera)} {

    }

    template<typename T>
    auto set_value(const char* name, T val) -> bool {
        return set_value(camera, name, val);
    }

    auto start_acquisition() -> bool {
        return vimba_sdk::start_acquisition(camera);
    }

    auto stop_acquisition() -> bool {
        return vimba_sdk::stop_acquisition(camera);
    }

    auto trigger() -> bool {
        return vimba_sdk::do_software_trigger(camera);
    }

    auto trigger_once() -> bool {
        return vimba_sdk::do_software_trigger_once(camera);
    }

    CameraPtr camera;
};

auto Into(CaptureModeCamera from) -> IdleModeCamera {
    return IdleModeCamera{std::move(from.camera)};
}


auto do_capture_once(CaptureModeCamera& camera, uint32_t timeout) -> std::optional<Image> {
    auto cc{make_capture_context_impl()};
    if (auto res = cc->read(camera, timeout); res) {
        return Image{res.value()};
    }
    return {};
}

auto async_capture_impl(AsyncCaptureContxt& context, CaptureModeCamera& camera, int queue_size) -> bool {
    if (auto e = camera.camera->StartContinuousImageAcquisition(queue_size, context.get_observer()); e != VmbErrorSuccess) {
        LOG(ERROR) << "failed to register for capturing from the camera: " << ErrorCodeToMessage(e) << ENDL;
        context.stop();
        return false;
    }
    LOG(INFO) << "successfully registered for getting images from the camera" << ENDL;
    return true;
}

}       // end of namespace vimba_sdk

auto CaptureContext::read(vimba_sdk::CaptureModeCamera& camera, uint32_t timeout) -> std::optional<ImageView> {
    return vimba_sdk::do_acquisition(camera.camera, timeout, frame);
}

auto make_capture_context_impl() -> std::shared_ptr<CaptureContext> {
    return std::make_shared<CaptureContext>();
}


}       // end of namespace camera