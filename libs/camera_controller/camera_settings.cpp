#include "camera_settings.hh"
#include <iostream>

namespace camera {
auto operator << (std::ostream& os, HardWareTriggerSource ea) -> std::ostream& {
    return os << to_string(ea);
}

auto operator << (std::ostream& os, AcquisitionMode ea) -> std::ostream& {
    return os << to_string(ea);
}

auto operator << (std::ostream& os, ActivationMode ea) -> std::ostream& {
    return os << to_string(ea);
}

auto operator << (std::ostream& os, ExposureMode ea) -> std::ostream& {
    return os << to_string(ea);
}

auto operator << (std::ostream& os, ExposureAuto ea) -> std::ostream& {
    return os << (ea == ExposureAuto::Continuous ? 
                "Continuous" : ea == ExposureAuto::Once ? "Once" :  "Off");

}
}       // end of namespace camera