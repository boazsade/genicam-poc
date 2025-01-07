#include "cameras_context.hh"
#include "log/logging.h"
#include "vmb_common/ErrorCodeToMessage.h"
#include <VimbaCPP/Include/VimbaCPP.h>

namespace camera {

using namespace AVT::VmbAPI;

struct Context {

    ~Context() {
        stop();
    }

    static auto stop() -> bool {
        LOG(INFO) << "Closing the cameras context" << ENDL;
        if (auto e = VimbaSystem::GetInstance().Shutdown(); e != VmbErrorSuccess) {
            LOG(ERROR) << "error while trying to close cameras context: " << ErrorCodeToMessage(e) << ENDL;
            return false;
        }
        return true;
    }
};

auto make_context() -> std::variant<context_type, error_type> {
    if (auto e = VimbaSystem::GetInstance().Startup(); e != VmbErrorSuccess) {
        return {std::string{ErrorCodeToMessage(e)}};
    }
    LOG(INFO) << "successfully setup the cameras context\n";
    return {std::make_shared<Context>()};
}

auto stop(Context& ctx) -> bool {
    return ctx.stop();
}

auto enumerate(Context& ctx) -> std::vector<DeviceInfo> {
    CameraPtrVector cameras;
    if (auto e = VimbaSystem::GetInstance().GetCameras(cameras); e != VmbErrorSuccess || cameras.empty()) {
        LOG(ERROR) << "failed to get cameras list (maybe no camera is connected):" << ErrorCodeToMessage(e) << ENDL;
        return {};
    }
    LOG(INFO) << "successfully found " << cameras.size() << " cameras on this host" << ENDL;
    std::vector<DeviceInfo> output;
    for (auto&& camera : cameras) {
        DeviceInfo dev;
        camera->GetID(dev.id);
        camera->GetInterfaceID(dev.interface_id);
        camera->GetModel(dev.mode);
        output.push_back(dev);
    }
    return output;
}

auto operator << (std::ostream& os, const DeviceInfo& dev) -> std::ostream& {
    return os << "device id: " << dev.id 
        << "\n\tinterface id: " << dev.interface_id
        << "\n\tmodel: " << dev.mode;
}

}   // end of namespace camera