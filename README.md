# Recorder
This repository include source code to support recording application from 2 or more cameras.

## Overview
This include source code that contains code to:
- control the cameras.
- save the output from the cameras to disk.
- stream the captured images lives over UDP.
- enable remote control of the camera from some external application over TCP commands.

## Getting Started
Most of the code here is in C++.
The dependencies are managed using Conan.
To install these dependencies run:
```bash
./conan_install.sh
```
To build from the command line:
```bash
cmake --preset conan-debug
cmake --preset conan-release
cmake --build --preset conan-debug
cmake --build --preset conan-release
```
> Note that you don't need to run both `debug` and `release` - you can select either one.
> Note that you must run `./conan_install.sh` at least once. And then you can use IDE such as `vs-code` to build from inside it.

## Host Requirements
The host must contain the drivers for the camera - the SDK, the control the camera already installed.
The location of the SDK must be set before, either set the value inside the main [cmake file](https://github.com/boazsade/genicam-poc/blob/1119f098f331200ccce54fb7fcbdbddccb6bec11/CMakeLists.txt).
If the SDK support `find_package` cmake function, the we can use this to find the SDK settings.
### Hardware Setting
In order to work with the cameras, you must set the network to connect to the camera. Since this is a local - direct connection, the setting for the device must be for local networking as well.
#### Interface Setup
On Linux you can set this value to `Link Local Only` for the interface to which the camera is connected inside `Settings` -> `Network` -> click on the 'hamburger' icon next to the network interface that need to be set, then in the `IPv4` select the `Link Local Only`, the click `Apply`.
#### Interface Speed
In order to gain maximum presence over the ethernet connection, set the `MTU` value to `9000` for the interface to which the camera(s) is/are connected. On Linux you can set this up with the `Settings` -> `Network` -> click on the 'hamburger' icon next to the network interface that need to be set, then select `Identity` tab and change the value in the `MTU` to 9000, then click `Apply`.
To change this value from the command line follow this [this link](https://www.baeldung.com/linux/maximum-transmission-unit-change-size).

## Camera Control
The code here is controlling the cameras by using an external SDK. The SDK assumes to implement the [GenIcam](https://www.emva.org/wp-content/uploads/GenICam_SFNC_2_2.pdf) standard.
In general, the GenICam technology allows exposing arbitrary features of a camera through a
unified API and GUI. Each feature can be defined in an abstract manner by its name, interface
type, unit of measurement and behavior. The GenApi module of the GenICam standard defines
how to write a camera description file that describes the features of a device.
The usage of GenApi alone could be sufficient to make all the features of a camera or a device
accessible through the GenICam API. However if the user wants to write generic and portable
software for a whole class of cameras or devices and be interoperable, then GenApi alone is not
sufficient and the software and the device vendors have to agree on a common naming
convention for the standard feature.
### Starting The Device
When using `GenIcam` we the SDK is using a standard commands to control such features as source of the trigger to acquire the next image, setting the exposure time, acquiring the next image, stopping the acquiring, and so on.
The normal flow is:
1. Create the SDK environment.
2. Listing all connected cameras.
3. Connecting to the cameras and opening them.
4. Read the camera hardware configuration such as image resolution.
5. Setting various camera settings such as whether to use auto exposure, setting white balance mode, the source of the trigger to get the next image and so on.
### Acquisition Mode
Once this is done the application can start reading images from the device.
In general there are 2 mode doing so:
#### Async mode
In this mode, the application will register a list of buffers in a queue, and then it will be notified using callbacks about the next ready image.
This means that the application don't doing a wait operation as it will be notify about the ready image once it is ready, this mode creates a stream of images.
It allow the application to trigger and `start`, `stop` the acquisition from the main loop (outside of the callback context).
#### Synchronous mode
In this mode, the application will ask for the SDK to read the next image from the device, the SDK normally provides a timeout to be waited, so if the image is not ready the application will not be blocked forever.
Using this mode allow for more control about how and when to read the next image from the device, but in some SDKs it will use more memory to allocate more buffers and not just allocate them once.

## Basic Flow
First and foremost a GenICam SDK must be installed on the host.
The make sure that at least one camera is connected to the host, and the is visible from the host.
the basic code for using the camera is:
```cpp
#include "camera_controller/camera.hh"
#include "camera_controller/cameras_context.hh"

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

{
    auto devices_ctx{camera::make_context()};                           // create the SDK context
    assert(std::holds_alternative<camera::error_type>(devices_ctx));    // make sure we successfully created it
    auto& ctx{std::get<camera::context_type>(devices_ctx)};             // extract the context
    const auto devices{camera::enumerate(*ctx.get())};                  // list all available cameras
    assert(!devices.empty());
    auto camera{open_device(devices, *ctx)};                            // get one available camera from the list
    assert(camera);
    // now you can set some setting to the camera, but this is not required
    assert(camera::set_auto_whitebalance(*camera, true, true));
    // when ready to start the capture, convert to capture mode
    // Here we are showing how to use a sync mode, where we are waiting for the next image to be ready
    auto capture_source{camera::From(std::move(camera))};
    auto cc{camera::make_capture_context()};
    // now capture 20 images
    uint32_t timeout{1000};
    auto success{0};
    for (auto i = 0; i < 20; i++) {
        if (auto image = camera::capture_one(*camera, timeout, *cc); image) {
            std::cout << "successfully read image: " << image.value() << std::endl;
            ++success;
        } else {
            std::cerr << "failed to read image number " << i << std::endl;
            timeout += 200;
        }
    }
    // All the cleanup is done automatically.   
}
```

## Current SDK
- The code here is currently using under the hood the [VIMBA SDK](https://www.alliedvision.com/en/products/software/vimba-x-sdk/).
- There current release note for version 6.1 which is what this was developed with can be found [here](https://docs.alliedvision.com/Vimba_ReleaseNotes/ARM.html#summary).
- The installation guild for this SDK [is found here](https://www.alliedvision.com/fileadmin/content/documents/products/software/software/Vimba/appnote/Vimba_installation_under_Linux.pdf).
- Please note that the the cmake file you can control whether to use this SDK through `VIMBA_SDK` option that is set to `on` by default.
- You must also set the location where the SDK is installed, in the cmake variable  `SDK_INSTALL_DIR`. by default installation is done under `/opt/` directory.
- You can find the c++ developers guide in [this link](https://docs.alliedvision.com/Vimba_X/Vimba_X_DeveloperGuide/cppAPIManual.html#transforming-images).
- There are many code examples here such as:
1. [capture test](https://github.com/boazsade/genicam-poc/tree/main/tests/capture_test).
2. [capture single test](https://github.com/boazsade/genicam-poc/blob/1119f098f331200ccce54fb7fcbdbddccb6bec11/tests/capture_single_test/main.cpp).
3. [first test](https://github.com/boazsade/genicam-poc/blob/1119f098f331200ccce54fb7fcbdbddccb6bec11/tests/first_case/program.cpp).

The code itself is not writing as a vimba oriented code, i.e. the use of the vimba API is hidden and is not part of the external API here. most of the code that is using the SDK is located under [vimba sdk directory](http://192.168.200.3:7990/projects/DAA/repos/recorder4cameras/browse/libs/camera_controller/vimba).
The basic flow for using the vimba SDK is:
![this flow](https://docs.alliedvision.com/Vimba_X/Vimba_X_DeveloperGuide/_images/VmbCPP_asynchronous.png).
### Image Capture vs. Image Acquisition
mage capture and image acquisition are two independent operations: The API captures images, the camera acquires images. To obtain an image from your camera, setup the API to capture images before starting the acquisition on the camera as seen in the image above.
Note that the API prepares a list of buffers (one or more) on the device. Then the acquisition starts, images collected, and finally, the buffers are released and the after the acquisition is stopped.
