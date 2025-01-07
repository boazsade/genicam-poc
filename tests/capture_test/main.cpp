#include "Bitmap.h"
#include "save2file.h"
#include "VimbaCPP/Include/VimbaCPP.h"
#include <iostream>
#include <string>
#include <memory>
#include <optional>
#include <chrono>

using namespace std::string_literals;
namespace vba = AVT::VmbAPI;

auto produce_delta_t = [last = std::chrono::high_resolution_clock::now()] () mutable {
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - last).count();
    last = t2;
    return ms_int;
};

struct Image : ImageBase {

    using Self = Image;

    VmbPixelFormatType pixel_format{VmbPixelFormatMono8};

    static auto TryInto(vba::FramePtr from) -> std::optional<Self> {
        Image image;
        from->GetPixelFormat(image.pixel_format);
        from->GetFrameID(image.number);
        
        if (!(from->GetImageSize(image.size) == VmbErrorSuccess &&
            from->GetWidth(image.width) == VmbErrorSuccess &&
            from->GetHeight(image.height) == VmbErrorSuccess
            )) {
            std::cerr << "error reading the image size\n";
            return {};
        }
        
        if (from->GetImage(image.data) != VmbErrorSuccess) {
            std::cerr << "failed to read the image data\n";
            return {};
        }
        return image;
    }
};

auto operator << (std::ostream& os, const Image& image) -> std::ostream& {
    return os << image.size << " bytes, [" << image.width << " x " << image.height << "] of type " << (int)image.pixel_format;
}

using device_contorl_t = std::shared_ptr<vba::VimbaSystem>;

// Set the GeV packet size to the highest possible value
        // (In this example we do not test whether this cam actually is a GigE cam)
auto set_comm_speed(vba::CameraPtr& camera) -> bool {
    vba::FeaturePtr features;
    if (camera->GetFeatureByName("GVSPAdjustPacketSize", features) != VmbErrorSuccess) {
        std::cerr << "failed to get packet size feature from the camera\n";
        return false;
    }
    if (features->RunCommand() != VmbErrorSuccess) {
        std::cerr << "failed to run feature command\n";
        return false;
    }
    bool done{false};
    while (features->IsCommandDone(done) == VmbErrorSuccess && !done) {
    }
    std::cout << "we have " << (done ? "successfully" : "failed") << " to set the feature" << std::endl;
    return done;
}

auto set_pixel_type(vba::CameraPtr& camera, VmbPixelFormatType type) -> bool {
    vba::FeaturePtr features;
    if (camera->GetFeatureByName("PixelFormat", features) != VmbErrorSuccess) {
        std::cerr << "failed to get pixel format feature from the camera\n";
        return false;
    }
    if (features->SetValue(type) != VmbErrorSuccess) {
        std::cerr << "failed to set the pixel type to " << (int)type << "\n";
        return false;
    }
    return true;
}

auto set_frame_rate(vba::CameraPtr& camera, float target) -> bool {
    vba::FeaturePtr features;
    if (camera->GetFeatureByName("ExposureAuto", features) != VmbErrorSuccess) {
        std::cerr << "failed to get exposure mode feature from the camera\n";
        return false;
    }
    if (features->SetValue("Off") != VmbErrorSuccess) {
        std::cerr << "failed to set auto exposure off\n";
        return false;
    }
    if (camera->GetFeatureByName("AcquisitionFrameRateEnable", features) != VmbErrorSuccess) {
        std::cerr << "failed to read the frame rate feature\n";
        return false;
    }
    if (features->SetValue(true) != VmbErrorSuccess) {
        std::cerr << "failed to set acquisition frame rate\n";
        return false;
    }
    if (camera->GetFeatureByName("AcquisitionFrameRate", features) != VmbErrorSuccess) {
        std::cerr << "failed to read the frame rate value feature\n";
        return false;
    }
    if (features->SetValue(target) != VmbErrorSuccess) {
        std::cerr << "failed to set acquisition frame rate\n";
        return false;
    }
    if (camera->GetFeatureByName("ExposureTime", features) != VmbErrorSuccess) {
        std::cerr << "failed to read exposure time feature\n";
        return false;
    }

    // if (features->SetValue(20'000) != VmbErrorSuccess) {
    //     std::cerr << "failed to set exposure time\n";
    //     return false;
    // }
    return true;
}

auto save_bitmap(Image image, const std::string& output_file) -> int {
    AVTBitmap bitmap;
    bitmap.bufferSize = image.size;
    bitmap.width = image.width;
    bitmap.height = image.height;
    bitmap.colorCode = ColorCodeRGB24;
    if (AVTCreateBitmap(&bitmap, image.data) == 0) {
        std::cerr << "failed to create the bitmap image from the input\n";
        return -1;
    }
    if (AVTWriteBitmapToFile( &bitmap, output_file.c_str()) == 0) {
        std::cerr << "failed to save bitmap image to the file " << output_file << "\n";
        return -1;
    }
    return 0;
}

auto open_camera(std::string camera_id, device_contorl_t& control) -> vba::CameraPtr {

    static const auto enumerate = [](const vba::CameraPtrVector& cams) {
        for (auto&& camera : cams) {
            std::string id, iid, m;
            camera->GetID(id);
            camera->GetInterfaceID(iid);
            camera->GetModel(m);
            std::cout << "Successfully opened:\n\tdevice id: " << id 
                    << "\n\tinterface id: " << iid << "\n\tmodel: " << m << std::endl;
        }
    };

    const auto open_device = [&control](vba::CameraPtr camera, const auto& id) {
        return control->OpenCameraByID(id.c_str(), VmbAccessModeFull, camera) == VmbErrorSuccess ?
                camera : vba::CameraPtr{};
    };

    if (camera_id.empty()) {
        vba::CameraPtrVector cameras;
        if (control->GetCameras(cameras) != VmbErrorSuccess || cameras.empty()) {
            std::cerr << "failed to get cameras list (maybe no camera is connected)\n";
            return {};
        }
        std::cout << "successfully found " << cameras.size() << " cameras on this host" << std::endl;
        enumerate(cameras);
        auto camera{cameras[0]};
        if (camera->GetID(camera_id) != VmbErrorSuccess) {
            std::cerr << "failed to get camera id for first camera device\n";
            return {};
        }
        return open_device(std::move(camera), camera_id);
    }
    vba::CameraPtr camera;
    if (control->GetCameraByID(camera_id.c_str(), camera) != VmbErrorSuccess) {
        std::cerr << "failed to get camera '" << camera_id << "' make sure that this camera is connected\n";
        return {};
    }

    return open_device(std::move(camera), camera_id);
}

auto do_acquisition(vba::CameraPtr& camera, vba::FramePtr& frame) -> std::optional<Image> {
    if (camera->AcquireSingleImage(frame, 1000) != VmbErrorSuccess) {
        std::cerr << "failed to read image from camera after 5000 ms\n";
        return {};
    }
    VmbFrameStatusType status;
    frame->GetReceiveStatus(status);
    if (status != VmbFrameStatusComplete) {
        std::cerr << "we don't have the full image after 5000ms\n";
        return {};
    }
    const auto image{Image::TryInto(frame)};
    // if (image) {
    //     std::cout << "successfully read image" << *image << std::endl;
    // }
    return image;
}

auto capture_one(vba::CameraPtr& camera, int number, VmbPixelFormatType pt) -> int {
    std::cout << produce_delta_t() <<  ": running capture number " << number;
    vba::FramePtr frame;
    auto image{do_acquisition(camera, frame)};
    if (image) {
        if (image->pixel_format != pt) {
            std::cout << std::endl;
            std::cerr << "we got image format of " << (int)image->pixel_format << " and not VmbPixelFormatRgb8" << std::endl;
            return -1;
        }
    }
    if (pt == VmbPixelFormatRgb8) {
        std::cout << ", success RGB: " << image->height << " x " << image->width << ": " << image->number << std::endl;
        // output_file = output_file + "_" + std::to_string(number) + ".bmp";
        // auto i = save_bitmap(*image, output_file);
        // if (i < 0) {
        //     std::cout << "the image " << *image << " was not saved as bitmap to " << output_file << std::endl;
        //     return number;
        // }
    } else {
        if (pt == VmbPixelFormatBayerRG8) {
            image->number = number;
            std::cout << ", success RAW: " << image->height << " x " << image->width << ": " << image->number << std::endl;
            //do_save(*image);                    
        }
    }
    //std::cout << "saved the image number " << number <<"\n";
    return number;
}

auto do_capture(const std::string& camera_id, std::string output_file, VmbPixelFormatType pt) -> int {
    device_contorl_t handle(&vba::VimbaSystem::GetInstance(), [](vba::VimbaSystem* handle) {handle->Shutdown();});

    struct Clearup {
        Clearup(vba::CameraPtr& c) : cam{c} {

        }
        ~Clearup() {
            cam->Close();
        }

        vba::CameraPtr& cam;
    };

    if (handle->Startup() != VmbErrorSuccess) {
        std::cerr << "failed to start the device\n";
        return -1;
    }
    // now open the camera that we have, if the value is empty get the list of cameras attached to the host and use the first one
    auto camera{open_camera(camera_id, handle)};
    if (!camera) {
        std::cerr << "failed to set the camera!\n";
        return -1;
    }
    Clearup c{camera};
    // now we have a valid open camera, let retport this to the user:
    std::string id, iid, m;
    camera->GetID(id);
    camera->GetInterfaceID(iid);
    camera->GetModel(m);
    std::cout << "Successfully opened:\n\tdevice id: " << id 
            << "\n\tinterface id: " << iid << "\n\tmodel: " << m << std::endl;
    if (set_comm_speed(camera) && set_pixel_type(camera, pt)/* && set_frame_rate(camera, 25.0)*/) {
        std::cout << "Starting up the acquisition process\n";
        for (auto i = 0; i < 50; i++) {
            if (capture_one(camera, i, pt) < 0) {
                return -1;
            }
        }
        return 0;
    } else {
        std::cerr << "failed to set the camera features required to the image capture\n";
    }
    return -1;
}

auto main(int argc, char** argv) -> int {
    const auto camera_id = argc > 1 ? std::string{argv[1]} : std::string{};
    const auto output_file = argc > 2 ? std::string{argv[2]} : "SynchronousGrab"s;

    if (camera_id.empty()) {
        std::cout << "will select the first detected camera as the image source" << std::endl;
    } else {
        std::cout << "Will use camera id: " << camera_id << std::endl;
    }

    std::cout << "Will save the capture image to " << output_file << std::endl;
    return do_capture(camera_id, output_file, VmbPixelFormatBayerRG8);//VmbPixelFormatRgb8);
}
