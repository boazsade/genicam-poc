#pragma once
#include <memory>
#include <tuple>
#include <string>
#include <variant>
#include <vector>
#include <iosfwd>

namespace camera {

struct Context;
using context_type = std::shared_ptr<Context>;
using error_type = std::string;

struct DeviceInfo {
    std::string id;
    std::string interface_id;
    std::string mode;
};

auto operator << (std::ostream& os, const DeviceInfo& dev) -> std::ostream&;

[[nodiscard]] auto make_context() -> std::variant<context_type, error_type>;
[[nodiscard]] auto stop(Context& ctx) -> bool;
[[nodiscard]] auto enumerate(Context& ctx) -> std::vector<DeviceInfo>;

}   // end of namespace camera