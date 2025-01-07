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

auto simple_capture(camera::CapturingCamera& camera, int max) -> bool {
    auto cc{camera::make_capture_context()};
    if (!cc) {
        std::cerr << "failed to create capture context\n";
        return false;
    }
    uint32_t timeout{1000};
    auto success{0};
    for (auto i = 0; i < max; i++) {
        if (auto image = camera::capture_one(camera, timeout, *cc); image) {
            std::cout << "successfully read image: " << image.value() << std::endl;
            ++success;
        } else {
            std::cerr << "failed to read image number " << i << std::endl;
            timeout += 200;
        }
    }
    return success == max;
}

auto async_test(std::shared_ptr<camera::CapturingCamera> camera) -> bool {
    unsigned long long total{0};
    const auto process_image = [&total](camera::ImageView image) {
        std::cout << "successfully got image: " << image << std::endl;
        total = image.number;
        
        return image.number < 20;
    };

    std::stop_source stop_source;
    auto ctx{camera::make_async_context(*camera, [process_image](camera::ImageView image) -> bool {
        return process_image(image);
    }, stop_source.get_token())};
    if (!ctx) {
        std::cerr << "failed to create the async context for the device" << std::endl;
        return false;
    }
    if (!camera::async_capture(*ctx, *camera, 10)) {
        std::cerr << "failed to initiate the async capture!\n";
        return false;
    }
    while (total < 20) {
        std::this_thread::sleep_for(1s); // ~30 FPS
    }
    stop_source.request_stop();
    return true;
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
    // if (!camera::set_default_software_mode(*camera, camera::ActivationMode::AnyEdge)) {
    //     std::cerr << "failed to set to default software mode for " << devices.at(cix) << "\n";
    //     return -1;
    // }
    if (async_test(camera::From(std::move(camera)))) {
        std::cout << "successfully finish capturing in software mode" << std::endl;
    } else {
        std::cerr << "did go so well the software capture mode\n";
    }
    return 0;

    std::cout << "starting to capture from the device" << std::endl;
    auto capture_source{camera::From(std::move(camera))};
    if (simple_capture(*capture_source, 20)) {
        std::cout << "Successfully finish capturing" << std::endl;
    } else {
        std::cerr << "failed to capture all/some of the images\n";
    }
}