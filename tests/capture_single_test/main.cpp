#include <VimbaCPP/Include/VimbaCPP.h>
#include <iostream>
#include <thread> // For delay
#include <chrono> // For precise timing
#include <string_view>

using namespace AVT::VmbAPI;
using device_contorl_t = std::shared_ptr<VimbaSystem>;

auto error2str(VmbErrorType e) -> std::string_view {
     switch (e) {
        case VmbErrorSuccess: return "no error";
        case VmbErrorInternalFault: return "Unexpected fault in VimbaC or driver";
        case VmbErrorApiNotStarted: return "VmbStartup() was not called before the current command";
        case VmbErrorNotFound: return "The designated instance (camera, feature etc.) cannot be found";
        case VmbErrorBadHandle: return "The given handle is not valid";
        case VmbErrorDeviceNotOpen: return "VmbErrorDeviceNotOpen";
        case VmbErrorInvalidAccess: return "Operation is invalid with the current access mode";
        case VmbErrorBadParameter: return "ne of the parameters is invalid (usually an illegal pointer)";
        case VmbErrorStructSize: return "The given struct size is not valid for this version of the API";
        case VmbErrorMoreData: return "More data available in a string/list than space is provided";
        case VmbErrorWrongType: return "Wrong feature type for this access function";
        case VmbErrorInvalidValue: return "The value is not valid; either out of bounds or not an increment of the minimum";
        case VmbErrorTimeout: return "Timeout during wait";
        case VmbErrorOther: return "Other error";
        case VmbErrorResources: return "Resources not available (e.g. memory)";
        case VmbErrorInvalidCall: return "Call is invalid in the current context (e.g. callback)";
        case VmbErrorNoTL: return "No transport layers are found ";
        case VmbErrorNotImplemented: return "API feature is not implemented";
        case VmbErrorNotSupported: return "API feature is not supported";
        case VmbErrorIncomplete: return "The current operation was not completed (e.g. a multiple registers read or write)";
        case VmbErrorIO: return "Low level IO error in transport layer";
        default:
            return "unknown error";

    }
}

auto open_camera(std::string camera_id, device_contorl_t& control) -> CameraPtr {

    static const auto enumerate = [](const CameraPtrVector& cams) {
        for (auto&& camera : cams) {
            std::string id, iid, m;
            camera->GetID(id);
            camera->GetInterfaceID(iid);
            camera->GetModel(m);
            std::cout << "Successfully opened:\n\tdevice id: " << id 
                    << "\n\tinterface id: " << iid << "\n\tmodel: " << m << std::endl;
        }
    };

    const auto open_device = [&control](CameraPtr camera, const auto& id) {
        return control->OpenCameraByID(id.c_str(), VmbAccessModeFull, camera) == VmbErrorSuccess ?
                camera : CameraPtr{};
    };

    if (camera_id.empty()) {
        CameraPtrVector cameras;
        if (control->GetCameras(cameras) != VmbErrorSuccess || cameras.empty()) {
            std::cerr << "failed to get cameras list (maybe no camera is connected)\n";
            return {};
        }
        std::cout << "successfully found " << cameras.size() << " cameras on this host" << std::endl;
        enumerate(cameras);
        for (auto&& c : cameras) {
            if (auto e = c->GetID(camera_id); e != VmbErrorSuccess) {
                std::cerr << "failed to get camera id for first camera device: " << error2str(e) << "\n";
                return {};
            }
            auto cam{open_device(std::move(c), camera_id)};
            if (cam) {
                return cam;
            }
        }
        return {};
    }
    CameraPtr camera;
    if (auto e = control->GetCameraByID(camera_id.c_str(), camera); e != VmbErrorSuccess) {
        std::cerr << "failed to get camera '" << camera_id << "' make sure that this camera is connected " << error2str(e) << "\n";
        return {};
    }

    return open_device(std::move(camera), camera_id);
}


auto produce_delta_t = [last = std::chrono::high_resolution_clock::now()] () mutable {
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - last).count();
    last = t2;
    return ms_int;
};

class FrameObserver : public IFrameObserver {
public:
    explicit FrameObserver(CameraPtr c, uint64_t max) : IFrameObserver{c}, max_iteration{max} {

    }

    void FrameReceived(const FramePtr f) {
        VmbUint64_t id{0};
        VmbUint32_t size{0};
        f->GetFrameID(id);
        f->GetImageSize(size);
        std::cout << produce_delta_t() << ": capture frame: " << id << " of size " << size << std::endl;
        count = id;
        m_pCamera->QueueFrame(f);
    }

    constexpr auto done() const -> bool {
        return count >= max_iteration;
    }
private:
    uint64_t count{0};
    uint64_t max_iteration{0};
};

template<typename Value>
auto set_value(CameraPtr& camera, const char* key, Value val) -> bool {
    FeaturePtr features;
    if (auto e = camera->GetFeatureByName(key, features); e != VmbErrorSuccess) {
        std::cerr << "failed to get exposure mode feature from the camera: " << error2str(e) << "\n";
        return false;
    }
    if (auto e = features->SetValue(val); e != VmbErrorSuccess) {
        std::cerr << "failed to set value " << val << " for " << key << ": " << error2str(e) << "\n";
        return false;
    }
    return true;
}

auto set_frame_rate(CameraPtr& camera, float target) -> bool {
    return set_value(camera, "ExposureAuto", "Off") &&
           set_value(camera, "AcquisitionFrameRateEnable", false) &&
           set_value(camera, "ExposureTime", 120'000);
}

auto set_comm_speed(CameraPtr& camera) -> bool {
    FeaturePtr features;
    if (camera->GetFeatureByName("GVSPAdjustPacketSize", features) != VmbErrorSuccess) {
        std::cerr << "failed to get packet size feature from the camera\n";
        return false;
    }
    if (auto e = features->RunCommand(); e != VmbErrorSuccess) {
        std::cerr << "failed to run feature command " << e << "\n";
        return false;
    }
    bool done{false};
    while (features->IsCommandDone(done) == VmbErrorSuccess && !done) {
    }
    std::cout << "we have " << (done ? "successfully" : "failed") << " set the feature comm speed" << std::endl;
    return done;
}

auto set_pixel_type(CameraPtr& camera, VmbPixelFormatType type) -> bool {
    return set_value(camera, "PixelFormat", type);
}

using namespace std::chrono_literals;

int main(int argc, char** argv)
{
    uint64_t max_iters = argc > 1 ? std::stoll(argv[1]) : 10;

    std::cout << "Running for " << max_iters << " times\n";

    device_contorl_t handle(&VimbaSystem::GetInstance(), [](VimbaSystem* handle) {handle->Shutdown();});

    if (handle->Startup() != VmbErrorSuccess) {
        std::cerr << "failed to start the device\n";
        return -1;
    }

    auto camera{open_camera(std::string{}, handle)};
    if (!camera) {
        std::cerr << "failed to open camera\n";
        return -1;
    }

    // Open camera settings
    camera->Open(VmbAccessModeFull);
    if (!(set_comm_speed(camera) && set_pixel_type(camera, VmbPixelFormatBayerRG8))) {
        std::cerr << "failed to set the device settings\n";
        return -1;
    }
    FrameObserver* fo = new FrameObserver(camera, max_iters);
    IFrameObserverPtr fop{fo};
    // Set camera to software trigger mode
    std::vector<FramePtr> frames;
    FeaturePtr feature;
    if (auto error = camera->GetFeatureByName("PayloadSize", feature); error != VmbErrorSuccess) {
        std::cerr << "failed to read payload size: " << error2str(error) << "\n";
        return -1;
    }
    VmbInt64_t pl{-1};
    if (auto error = feature->GetValue(pl); error != VmbErrorSuccess) {
        std::cerr << "failed to get payload size: " << error2str(error) << "\n";
        return -1;
    }
        // set the use of trigger by software
    if (camera->GetFeatureByName("TriggerMode", feature) == VmbErrorSuccess) {
        if (auto e = feature->SetValue("On"); e != VmbErrorSuccess) {
            std::cerr << "failed to set mode to on: " << error2str(e) << std::endl;
        }
    }
    if (camera->GetFeatureByName("TriggerSource", feature) == VmbErrorSuccess) {
        if (auto e = feature->SetValue("Software"); e != VmbErrorSuccess) {
            std::cerr << "failed to set trigger to software mode: " << error2str(e) << std::endl;
        }
    }
    if (camera->GetFeatureByName("AcquisitionMode", feature) == VmbErrorSuccess) {
        if (auto e = feature->SetValue("SingleFrame"); e != VmbErrorSuccess) {
            std::cerr << "failed to set AcquisitionMode to single mode: " << error2str(e) << std::endl;
        }
    }

    frames.resize(15);
    for (auto&& f: frames) {
        f = FramePtr(new Frame(pl, AVT::VmbAPI::FrameAllocation_AllocAndAnnounceFrame));
        if (auto e = f->RegisterObserver(fop); e != VmbErrorSuccess) {
            std::cerr << "failed to register observer to the frame" << std::endl;
            return -1;
        }
        if (auto e = camera->AnnounceFrame(f); e != VmbErrorSuccess) {
            std::cerr << "failed to connect frame of size " << pl << " to camera queue: " << error2str(e) << "\n";
            return -1;
        }
    }

    if (auto e = camera->StartCapture(); e != VmbErrorSuccess) {
        std::cerr << "failed to start capture " << error2str(e) << "\n";
        return -1;
    }
    for (auto&& f: frames) {
        if (auto e = camera->QueueFrame(f); e != VmbErrorSuccess) {
            std::cerr << "failed to queue frame of size " << pl << " to camera queue: " << error2str(e) << "\n";
            return -1;
        }
    }

    // Start acquisition
    // Main loop: Software trigger and capture
    for (; !fo->done();) {
        if (camera->GetFeatureByName("AcquisitionStart", feature) == VmbErrorSuccess) {
            feature->RunCommand();
        } else {
            std::cerr << "failed to start acquisition" << std::endl;
            break;
        }
        // Send software trigger
        if (camera->GetFeatureByName("TriggerSoftware", feature) == VmbErrorSuccess) {
            feature->RunCommand();
        }
        else {
            std::cerr << "Failed to send software trigger." << std::endl;
            break;
        }
        // Optional: Control frame rate by introducing a delay
        std::this_thread::sleep_for(1s); // ~30 FPS
        if (camera->GetFeatureByName("AcquisitionStop", feature) == VmbErrorSuccess) {
            feature->RunCommand();
        } else {
            std::cerr << "failed to stop the Acquisition, aborting\n";
            break;
        }
    }
    if (camera->GetFeatureByName("AcquisitionStop", feature) == VmbErrorSuccess) {
        feature->RunCommand();
    } else {
        std::cerr << "failed to stop the Acquisition, aborting\n";
        exit(1);
    }
    camera->EndCapture();
    camera->FlushQueue();
    camera->RevokeAllFrames();
    // Close the camera
    camera->Close();

    return 0;
}