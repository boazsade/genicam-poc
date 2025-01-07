#pragma once
#include "image.hh"
#include <memory>
#include <functional>

namespace camera {
constexpr std::size_t DEFAULT_NUMBER_OF_BUFFERS = 15;
struct CaptureContext;
using capture_context_t = std::shared_ptr<CaptureContext>;

struct AsyncCaptureContxt;
using async_context_t = std::shared_ptr<AsyncCaptureContxt>;
using frame_processing_f = std::function<bool(ImageView)>;
struct SoftwareCaptureContxt;
using software_context_t = std::shared_ptr<SoftwareCaptureContxt>;

}       // end of namespace camera