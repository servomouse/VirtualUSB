#pragma once
#include <cstddef>
#include <cstdint>

namespace Toastbox {

template <typename... Ts>
size_t HashInts(Ts... ts);

} // namespace Toastbox
