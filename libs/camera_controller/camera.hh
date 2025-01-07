#pragma once
#include "cameras_fwd.hh"
#include "camera_settings.hh"
#include "cameras_context.hh"
#include "image.hh"
#include <vector>
#include <optional>
#include <iosfwd>
#include <stdint.h>
#include <memory>
#include <functional>
#include <stop_token>

namespace camera {

// This mode of camera is when we are not capturing images yet.
struct IdleCamera;

// This mode is when we are starting the capture device, and reading the images from the camera
struct CapturingCamera;

// Operate the camera when we are NOT in capture mode.
// Create the camera based on the id, you can get the ID from the context
[[nodiscard]] auto create(const Context& ctx, const DeviceInfo& dev_id) -> std::shared_ptr<IdleCamera>;

// Set the format of the captured images, this is content of the frame that we are reading from the camera. 
[[nodiscard]] auto set_capture_type(IdleCamera& camera, PixelFormat pixel_format) -> bool;

// Enable software trigger - this means that the device is control by doing software trigger, that
// is, we are controlling the FPS by triggering the device each time we are ready to read the next image.
[[nodiscard]] auto set_software_trigger(IdleCamera& camera) -> bool;

// Enable hardware trigger. This means that we the camera is control by some external signal that will fire the device.
// This is the mode of operation that we will be using in the final product.
[[nodiscard]] auto set_hardware_trigger(IdleCamera& camera, HardWareTriggerSource src, ActivationMode am) -> bool;

// Set how the exposure is done - see ExposureMode
[[nodiscard]] auto set_exposure_mode(IdleCamera& camera, ExposureMode mode) -> bool;

// This will read the size of the payload that we are reading from the device. You must using this information in
// order to allocate buffers that will hold the data from the device. Note that we will have a mode (for debug),
// where the buffers will be allocated automatically, but even then, in order to allocate our own buffers,
// we need to have this information.
[[nodiscard]] auto get_frame_size(IdleCamera& camera) -> std::optional<int64_t>;    // if failed then return nullopt

// Whether we are working in single frame mode, a burst of images, or continues mode.
[[nodiscard]] auto set_acquisition_mode(IdleCamera& camera, AcquisitionMode mode) -> bool;

// Sets the device to work in auto exposure mode - i.e. Sets the automatic exposure mode when ExposureMode is Timed
// if once_flag is true, Exposure duration is adapted once by the device. Once it has converged, it returns to the Off state.
[[nodiscard]] auto auto_exposure(IdleCamera& camera, bool once_flag) -> bool;

// Exposure duration is user controlled using ExposureTime.
[[nodiscard]] auto manual_exposure(IdleCamera& camera, double time) -> bool;

// Controls the mode for automatic white balancing between the color channels. The white balancing ratios are
// automatically adjusted when `on` is true. if `continues` then this mode is done while doing the recording
[[nodiscard]] auto set_auto_whitebalance(IdleCamera& camera, bool on, bool continues) -> bool;

// Set the camera to use trigger by software, recording in RAW RGGB auto exposure and continue mode.
[[nodiscard]] auto set_default_software_mode(IdleCamera& camera, ActivationMode am) -> bool;

// Set the camera to use trigger by hardware, recording in RAW RGGB auto exposure and continue mode.
[[nodiscard]] auto set_default_hardware_mode(IdleCamera& camera, HardWareTriggerSource source, ActivationMode am) -> bool;

///////////////////////////////////////////////////////////////////////////////
// In capture mode
///////////////////////////////////////////////////////////////////////////////

// For a stream of images (capturing more than a single image), we would like to have a context where we can store a state
// to make the image memory allocation better. Use this when you are about to read more than single image.
// In order to crate the context we need to know how many buffers will be allocated between the device and the host.
// These buffers are the buffers that the camera will fill with the recorded image, and will be pull back to the host.
auto make_capture_context() -> capture_context_t;


// Capture a single image from the device, in case we failed to read after the timeout value (in milliseconds),
// we will return a nullopt.
// Note that this is a function that you should be using only when I single image is required,
// we have other functions to read a stream of images.
[[nodiscard]] auto capture_once(CapturingCamera& camera, uint32_t timeout) -> std::optional<Image>;

// This function is different in that we are reading a single image from the device, but we assuming that it would be
// one of many other images that we will be reading. We need to have a context to read with (CaptureContext).
// Note that you are not the owner of the memory!!. If you need to own the image, convert the iamge type from 
// ImageView into Image!
// Note that you would block to no more than the timeout value in milliseconds.
// In case if an error, the return value is nullopt.
// A simple use case for this is:
// try {
//  auto ctx = make_capture_context(camera);
//  while (..) {
//      if (auto i = capture_one(camera, 1000, ctx); i) {
//          // process the image inside the loop
//      }
//  }
// } catch (const std::exception& e) {
//      // handle the error case, we cannot capture!
// }
[[nodiscard]] auto capture_one(CapturingCamera& camera, uint32_t timeout, CaptureContext& context) -> std::optional<ImageView>;

// This code is to support working in "sync" mode. This mode allow the camera to call a callback function whenever we have a new frame.
// In this mode, the buffers for storing the results are allocated in advance, then whenever the camera has a ready frame,
// The user "callback" is called.
// For this we need to have a context where the user will register a "callback" and the framework will call it
// The normal use case for this is something like:
// std::stop_source stop_source;
// auto ctx = make_async_context([](ImageView image) { std::cout << "got image: " << image << std::endl; return image.number < 1000;}, stop_source.get_token());
// if (!ctx) { std::cerr << "failed to create async context\n"; exit(1);}
//
// if (!async_capture(ctx.get(), camera, 10)) {
//      std::cerr << "failed to start the async capture\n"; exit(1);
// }
// while (..) {
//     // do your stuff here..
//     // note this will run for as long as we are not stopping it - in this case while the frame number is less then 1000
// }
// stop_source.request_stop(); // abort here, we have some issue that we would like to exit
// 

// The function-like accept the next frame (not as an owner), and should return true if we need to continue, false will trigger stop of capture.
// note that we also have a stop token that allow cancellation from the outside by the user

[[nodiscard]] auto make_async_context(CapturingCamera& camera, frame_processing_f&& process_f, std::stop_token cancellation) -> async_context_t;

// The function will return true if successful.
[[nodiscard]] auto async_capture(AsyncCaptureContxt& context, CapturingCamera& camera, int queue_size) -> bool;

// We would support software trigger mode with capture. There is an issue here with it:
// This would only work with async mode, otherwise the trigger will not work.
// So we will allocate internal buffers, then run this with its own context where we collect the data.
// This is mostly the same from the application POV to the normal async mode.

// The function-like accept the next frame (not as an owner), and should return true if we need to continue, false will trigger stop of capture.
// note that we also have a stop token that allow cancellation from the outside by the user
[[nodiscard]] auto make_software_context(IdleCamera& camera, frame_processing_f&& process_f, std::stop_token cancellation, int queue_size) -> software_context_t;

// The function will return true if successful.
// In this mode we are working in continues mode, this means that we are only starting once. Then this function will return to the user.
// It is up to the user to stop this. The trigger in software is done once.
[[nodiscard]] auto async_software_capture(SoftwareCaptureContxt& context, CapturingCamera& camera) -> bool;

// This function will start the acquisition, trigger the device and stop the acquisition.
// This means that we will only get a single image per call to this function.
// Please note that still this is async operation, i.e. the processing of the image is done on the context of the
// process_f "function" that is registered at the context.
[[nodiscard]] auto async_software_capture_one(SoftwareCaptureContxt& context, CapturingCamera& camera) -> bool;

// You can call this function when you would like to stop, note that this is mostly required only for async_software_capture function.
[[nodiscard]] auto stop_acquisition(SoftwareCaptureContxt& context, CapturingCamera& camera) -> bool;

///////////////////////////////////////////////////////////////////////////////
// We can move into capture mode, and back to idle mode, but we cannot be in both.
auto From(std::shared_ptr<IdleCamera>&& cam) -> std::shared_ptr<CapturingCamera>;
auto From(IdleCamera&& cam) -> std::shared_ptr<CapturingCamera>;
auto Back(std::shared_ptr<CapturingCamera>&& cam) -> std::shared_ptr<IdleCamera>;



}   // end of namespace camera