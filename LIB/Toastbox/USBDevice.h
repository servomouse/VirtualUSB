#pragma once

#include <libusb-1.0/libusb.h>

#include <vector>
#include <set>
#include <memory>
#include <mutex>
#include <cassert>
#include "USB.h"
#include "RefCounted.h"
#include "Uniqued.h"
#include "RuntimeError.h"
#include "Defer.h"


void Retain(libusb_device* x);
void Release(libusb_device* x);

namespace Toastbox {

class USBDevice
{
public:
    using Milliseconds = std::chrono::milliseconds;
    static constexpr inline Milliseconds Forever = Milliseconds::max();
    
    // struct _Interface {
    //     uint8_t bInterfaceNumber = 0;
    //     bool claimed = false;
    // };
    
public:
    
    // Copy: illegal
    USBDevice(const USBDevice& x) = delete;
    USBDevice& operator=(const USBDevice& x) = delete;
    // Move: OK
    USBDevice(USBDevice&& x) = default;
    USBDevice& operator=(USBDevice&& x) = default;
    
    // std::vector<USBDevice> GetDevices();
    
    USBDevice(libusb_device* dev);
    
    bool operator==(const USBDevice& x) const;
    
    USB::DeviceDescriptor deviceDescriptor() const;
    
    USB::ConfigurationDescriptor configurationDescriptor(uint8_t idx) const;
    
    USB::StringDescriptorMax stringDescriptor(uint8_t idx, uint16_t lang=USB::Language::English);
    
    template <typename T>
    void read(uint8_t epAddr, T& t, Milliseconds timeout=Forever);
    
    size_t read(uint8_t epAddr, void* buf, size_t len, Milliseconds timeout=Forever);
    
    template <typename T>
    void write(uint8_t epAddr, T& x, Milliseconds timeout=Forever);
    
    void write(uint8_t epAddr, const void* buf, size_t len, Milliseconds timeout=Forever);
    
    void reset(uint8_t epAddr);
    
    template <typename T>
    void vendorRequestOut(uint8_t req, const T& x, Milliseconds timeout=Forever);
    
    void vendorRequestOut(uint8_t req, const void* data, size_t len, Milliseconds timeout=Forever);
    
    operator libusb_device*() const { return _dev; }
    
private:


    static unsigned int _LibUSBTimeoutFromMs(Milliseconds timeout);


    void _openIfNeeded();

    void _claimInterfaceForEndpointAddr(uint8_t epAddr);


    // static void _Retain(libusb_device* x);

    // static void _Release(libusb_device* x);

    using _LibusbDev = RefCounted<libusb_device*, Retain, Release>;

    _LibusbDev _dev = {};
    
public:
    
    uint16_t maxPacketSize(uint8_t epAddr) const;

    std::string manufacturer();

    std::string product();

    std::string serialNumber();
    
    std::vector<uint8_t> endpoints();
};

} // namespace Toastbox
