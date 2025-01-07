#pragma once
#include <iosfwd>
#include <stdint.h>

namespace camera {

// Note that this is HW specific.
enum class HardWareTriggerSource : uint32_t {
    Line0, Line1, Line2,
    Line3, Line4, Line5,
    Line6, Line7, Line8,
    Line9, Line10, Line11,
    Line12, Line13, Line14,
    Line15, Line16, Line17,
    Line18, Line19, Line20
};
auto operator << (std::ostream& os, HardWareTriggerSource ea) -> std::ostream&;

// Whether we are getting a single frame, burst of frames, or stream of frames.
enum class AcquisitionMode : uint32_t {
    Single,
    Burst,
    Continuous
};
auto operator << (std::ostream& os, AcquisitionMode ea) -> std::ostream&;

// Specifies the activation mode of the trigger
enum class ActivationMode : uint32_t {
    RisingEdge,     // Specifies that the trigger is considered valid on the rising edge of the source signal.
    FallingEdge,    // Specifies that the trigger is considered valid on the falling edge of the source signal
    AnyEdge,        // Specifies that the trigger is considered valid on the falling or rising edge of the source signal
    LevelHigh,      // Specifies that the trigger is considered valid as long as the level of the source signal is high
    LevelLow        // Specifies that the trigger is considered valid as long as the level of the source signal is low
};
auto operator << (std::ostream& os, ActivationMode ea) -> std::ostream&;

// Sets the operation mode of the Exposure.
enum class ExposureMode : uint32_t {
    Off,            // Disables the Exposure and let the shutter open.
    Timed,          // Timed exposure. The exposure duration time is set using the ExposureTime or ExposureAuto features and the exposure starts with the FrameStart or LineStart.
    TriggerWidth,   // Uses the width of the current Frame or Line trigger signal(s) pulse to control the exposure duration.
                    // Note that if the Frame or Line TriggerActivation is RisingEdge or LevelHigh,
                    // the exposure duration will be the time the trigger stays High. If TriggerActivation is FallingEdge or
                    // LevelLow, the exposure time will last as long as the trigger stays Low.
    TriggerControlled   // Uses one or more trigger signal(s) to control the exposure duration
                        // independently from the current Frame or Line triggers. See ExposureStart, ExposureEnd and
                        // ExposureActive of the TriggerSelector feature.
};
auto operator << (std::ostream& os, ExposureMode ea) -> std::ostream&;

// Sets the automatic exposure mode when ExposureMode is Timed. The exact algorithm used to implement
// this control is device-specific.
// Some other device-specific features might be used to allow the selection of the algorithm.
enum class ExposureAuto : uint32_t {
    Off,
    Once,
    Continuous
};
auto operator << (std::ostream& os, ExposureAuto ea) -> std::ostream&;

inline constexpr auto to_string(ActivationMode src) -> const char* {
    switch (src) {            
        case ActivationMode::FallingEdge:
            return "FallingEdge";
        case ActivationMode::LevelHigh:
            return "LevelHigh";
        case ActivationMode::LevelLow:
            return "LevelLow";
        case ActivationMode::RisingEdge:
            return "RisingEdge";
        case ActivationMode::AnyEdge:
        default:
            return "AnyEdge";
    }
}

inline constexpr auto to_string(ExposureMode em) -> const char* {
    switch (em) {
        case ExposureMode::Off:
            return "Off";
        case ExposureMode::Timed:
            return "Timed";
        case ExposureMode::TriggerControlled:
            return "TriggerControlled";
        case ExposureMode::TriggerWidth:
            return "ExposureMode";
        default:
            return "Off";
    }
}



inline constexpr auto to_string(HardWareTriggerSource from) -> const char* {
    switch (from) {
        case HardWareTriggerSource::Line0:
            return "Line0";
        case HardWareTriggerSource::Line1:
            return "Line1";
        case HardWareTriggerSource::Line2:
            return "Line2";
        case HardWareTriggerSource::Line3: 
            return "Line3";
        case HardWareTriggerSource::Line4: 
            return "Line4";
        case HardWareTriggerSource::Line5:
            return "Line5";
        case HardWareTriggerSource::Line6: 
            return "Line6";
        case HardWareTriggerSource::Line7: 
            return "Line7";
        case HardWareTriggerSource::Line8:
            return "Line8";
        case HardWareTriggerSource::Line9: 
            return "Line9";
        case HardWareTriggerSource::Line10: 
            return "Line10";
        case HardWareTriggerSource::Line11:
            return "Line11";
        case HardWareTriggerSource::Line12: 
            return "Line12";
        case HardWareTriggerSource::Line13: 
            return "Line13";
        case HardWareTriggerSource::Line14:
            return "Line14";
        case HardWareTriggerSource::Line15:
            return "Line15";
        case HardWareTriggerSource::Line16:
            return "Line16";
        case HardWareTriggerSource::Line17:
            return "Line17";
        case HardWareTriggerSource::Line18:
            return "Line18";
        case HardWareTriggerSource::Line19:
            return "Line19";
        case HardWareTriggerSource::Line20:
            return "Line20";
        default:
            return "Line0";

    }
}

inline constexpr auto to_string(AcquisitionMode mode) {
    switch (mode) {
        case AcquisitionMode::Single: return "SingleFrame";
        case AcquisitionMode::Burst: return "Burst";
        case AcquisitionMode::Continuous:
        default:
            return "Continuous";
    }
}

}       // end of namespace camera