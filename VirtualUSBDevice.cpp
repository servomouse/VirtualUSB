

#include "VirtualUSBDevice.h"


void VirtualUSBDevice::stop()
{
    auto lock = std::unique_lock(_s.lock);
    _reset(lock, ErrStopped);
}