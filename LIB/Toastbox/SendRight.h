#pragma once
// #include <mach/mach.h>
#include <cassert>
#include "RefCounted.h"

namespace Toastbox {

void _SendRightRetain(mach_port_t x);

void _SendRightRelease(mach_port_t x);

class SendRight : public RefCounted<mach_port_t, _SendRightRetain, _SendRightRelease>
{
public:
    using RefCounted::RefCounted;
    bool valid() const;
};

} // namespace Toastbox
