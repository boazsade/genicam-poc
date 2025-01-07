#include "camera_controller/camera.hh"
#include "camera_controller/cameras_context.hh"
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <thread>
#include <chrono>
#include <iostream>
#include <iterator>

using namespace std::chrono_literals;

inline auto colorize(cv::InputArray input, cv::OutputArray rgb) -> void {
    cv::cvtColor(input, rgb, cv::COLOR_BayerRG2RGB);
}

auto show_image(const std::uint8_t* data, int width, int height, int number, const char* name) -> void {
    using namespace std::string_literals;
    constexpr auto row_step = 30;       
    auto row = row_step;

    cv::Mat rgb(height, width, CV_8UC1, (void*)data);
    colorize(rgb, rgb);
    cv::resize(rgb, rgb, cv::Size(), 0.5, 0.5);
    std::string buf("FrameData:  #: "s + std::to_string(number));
    putText(rgb, buf.c_str(), cv::Point(row_step, row), cv::HersheyFonts::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
    row += row_step;
    auto title{"Image "s + name};
    cv::imshow(title, rgb);
    cv::waitKey(1);
}

auto show_images(const camera::ImageView& frames) -> void {
    if (frames.size) {
        show_image(frames.data, frames.width, frames.height, frames.number, "left");
    } else {
        std::cerr << "missing frame cannot display\n";
    }
    
}

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
    show_images(image);
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
        std::this_thread::sleep_for(500ms);
    }
    std::cout << "finish doing the software trigger test" << std::endl;
    
}
