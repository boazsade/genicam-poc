#include "camera_controller/camera.hh"
#include "camera_controller/cameras_context.hh"
#include <thread>
#include <chrono>
#include <iostream>
#include <iterator>

using namespace std::chrono_literals;

auto open_device(const std::vector<camera::DeviceInfo>& devices, const camera::Context& ctx) -> std::shared_ptr<camera::IdleCamera> {
    for (auto&& di : devices) {
        if (auto camera{camera::create(ctx, di)}; camera) {
            std::cout << "Successfully opened " << di << " for working" << std::endl;
            return camera;
        }
        std::cerr << "failed to open " << di << " for working\n";
    }
    std::cerr << "failed to open any of the cameras out of " << devices.size() << "\n";
    return {};
}

auto capture_images_processing(camera::ImageView image) -> bool {
    std::cout << "successfully got new image " << image << " from the device as the result of the software trigger" << std::endl;
    return true;        // dont do stop from this context
}

auto main(int argc, char** argv) -> int {
    auto devices_ctx{camera::make_context()};
    if (std::holds_alternative<camera::error_type>(devices_ctx)) {
        std::cerr << "failed to create device context\n";
        return -1;
    }

    auto& ctx{std::get<camera::context_type>(devices_ctx)};  // this is safe now

    const auto devices{camera::enumerate(*ctx.get())};
    if (devices.empty()) {
        std::cerr << "no device was detected, make sure that you connected and turned on the power\n";
        return -1;
    }
    std::size_t cix{0};
    std::copy(devices.begin(), devices.end(), std::ostream_iterator<camera::DeviceInfo>(std::cout, "\n"));
    auto camera{open_device(devices, *ctx)};
    if (!camera) {
        return -1;
    }
    std::cout << "successfully opened " << devices.at(cix) << std::endl;
    if (!camera::set_capture_type(*camera, camera::PixelFormat::RawRGGB8)) {
        std::cerr << "failed to set camera to RAW RGGB0 format\n";
        return -1;
    }
    if (!camera::set_auto_whitebalance(*camera, true, true)) {
        std::cerr << "failed to set auto white balance\n";
        return -1;
    }
    if (!camera::auto_exposure(*camera, true)) {
        std::cerr << "failed to enable auto exposure mode!\n";
        return -1;
    }
    if (!camera::set_acquisition_mode(*camera, camera::AcquisitionMode::Single)) {
        std::cerr << "failed to set the single image mode\n";
        return false;
    }
    std::cout << "Starting to capture images using software trigger" << std::endl;
    std::stop_source stop_source;
    auto software_ctx{camera::make_software_context(*camera, capture_images_processing, stop_source.get_token(), 10)};
    if (!software_ctx) {
        std::cerr << "failed to start the software context for image acquisition" << std::endl;
        return -1;
    }
    auto cc{camera::From(std::move(camera))};
    for (auto i = 0; i < 20 && camera::async_software_capture_one(*software_ctx, *cc); i++) {
        std::cout << "Triggered for the " << i << " time successfully " << std::endl;
    }
    std::cout << "finish doing the software trigger test" << std::endl;
    
}