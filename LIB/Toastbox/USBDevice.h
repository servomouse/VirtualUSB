#pragma once

#if __APPLE__
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>
#include "SendRight.h"
#elif __linux__
#include <libusb-1.0/libusb.h>
#endif

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

namespace Toastbox {

class USBDevice {
public:
    using Milliseconds = std::chrono::milliseconds;
    static constexpr inline Milliseconds Forever = Milliseconds::max();
    
    struct _Interface {
        uint8_t bInterfaceNumber = 0;
        bool claimed = false;
    };
    
public:
    
    // Copy: illegal
    USBDevice(const USBDevice& x) = delete;
    USBDevice& operator=(const USBDevice& x) = delete;
    // Move: OK
    USBDevice(USBDevice&& x) = default;
    USBDevice& operator=(USBDevice&& x) = default;
    
    static std::vector<USBDevice> GetDevices() {
        libusb_device** devs = nullptr;
        ssize_t devsCount = libusb_get_device_list(_USBCtx(), &devs);
        _CheckErr((int)devsCount, "libusb_get_device_list failed");
        Defer( if (devs) libusb_free_device_list(devs, true); );
        
        std::vector<USBDevice> r;
        for (size_t i=0; i<(size_t)devsCount; i++) {
            r.push_back(devs[i]);
        }
        return r;
    }
    
    USBDevice(libusb_device* dev) : _dev(_LibusbDev::Retain, std::move(dev)) {
        assert(dev);
        
        // Populate _interfaces and _epInfos
        {
            struct libusb_config_descriptor* configDesc = nullptr;
            int ir = libusb_get_config_descriptor(_dev, 0, &configDesc);
            _CheckErr(ir, "libusb_get_config_descriptor failed");
            
            for (uint8_t ifaceIdx=0; ifaceIdx<configDesc->bNumInterfaces; ifaceIdx++) {
                const struct libusb_interface& iface = configDesc->interface[ifaceIdx];
                
                // For now we're only looking at altsetting 0
                if (iface.num_altsetting < 1) throw RuntimeError("interface has no altsettings");
                _interfaces.push_back({
                    .bInterfaceNumber = iface.altsetting[0].bInterfaceNumber,
                });
                
                const struct libusb_interface_descriptor& ifaceDesc = iface.altsetting[0];
                for (uint8_t epIdx=0; epIdx<ifaceDesc.bNumEndpoints; epIdx++) {
                    const struct libusb_endpoint_descriptor& endpointDesc = ifaceDesc.endpoint[epIdx];
                    const uint8_t epAddr = endpointDesc.bEndpointAddress;
                    _epInfos[_OffsetForEndpointAddr(epAddr)] = _EndpointInfo{
                        .valid          = true,
                        .epAddr         = epAddr,
                        .ifaceIdx       = ifaceIdx,
                        .maxPacketSize  = endpointDesc.wMaxPacketSize,
                    };
                }
            }
        }
    }
    
    bool operator==(const USBDevice& x) const {
        return _dev == x._dev;
    }
    
    USB::DeviceDescriptor deviceDescriptor() const {
        struct libusb_device_descriptor desc;
        int ir = libusb_get_device_descriptor(_dev, &desc);
        _CheckErr(ir, "libusb_get_device_descriptor failed");
        
        return USB::DeviceDescriptor{
            .bLength                = desc.bLength,
            .bDescriptorType        = desc.bDescriptorType,
            .bcdUSB                 = desc.bcdUSB,
            .bDeviceClass           = desc.bDeviceClass,
            .bDeviceSubClass        = desc.bDeviceSubClass,
            .bDeviceProtocol        = desc.bDeviceProtocol,
            .bMaxPacketSize0        = desc.bMaxPacketSize0,
            .idVendor               = desc.idVendor,
            .idProduct              = desc.idProduct,
            .bcdDevice              = desc.bcdDevice,
            .iManufacturer          = desc.iManufacturer,
            .iProduct               = desc.iProduct,
            .iSerialNumber          = desc.iSerialNumber,
            .bNumConfigurations     = desc.bNumConfigurations,
        };
    }
    
    USB::ConfigurationDescriptor configurationDescriptor(uint8_t idx) const {
        struct libusb_config_descriptor* desc;
        int ir = libusb_get_config_descriptor(_dev, idx, &desc);
        _CheckErr(ir, "libusb_get_config_descriptor failed");
        return USB::ConfigurationDescriptor{
            .bLength                 = desc->bLength,
            .bDescriptorType         = desc->bDescriptorType,
            .wTotalLength            = desc->wTotalLength,
            .bNumInterfaces          = desc->bNumInterfaces,
            .bConfigurationValue     = desc->bConfigurationValue,
            .iConfiguration          = desc->iConfiguration,
            .bmAttributes            = desc->bmAttributes,
            .bMaxPower               = desc->MaxPower, // `MaxPower`: typo or intentional indication of unit change?
        };
    }
    
    USB::StringDescriptorMax stringDescriptor(uint8_t idx, uint16_t lang=USB::Language::English) {
        _openIfNeeded();
        
        USB::StringDescriptorMax desc;
        int ir = libusb_get_descriptor(_handle, USB::DescriptorType::String, idx, (uint8_t*)&desc, sizeof(desc));
        _CheckErr(ir, "libusb_get_string_descriptor failed");
        desc.bLength = ir;
        return desc;
    }
    
    template <typename T>
    void read(uint8_t epAddr, T& t, Milliseconds timeout=Forever) {
        const size_t len = read(epAddr, (void*)&t, sizeof(t), timeout);
        if (len != sizeof(t)) throw RuntimeError("read() didn't read enough data (needed %ju bytes, got %ju bytes)",
            (uintmax_t)sizeof(t), (uintmax_t)len);
    }
    
    #warning TODO: loop if ior==interrupted
    size_t read(uint8_t epAddr, void* buf, size_t len, Milliseconds timeout=Forever) {
        _claimInterfaceForEndpointAddr(epAddr);
        int xferLen = 0;
        int ir = libusb_bulk_transfer(_handle, epAddr, (uint8_t*)buf, (int)len, &xferLen,
            _LibUSBTimeoutFromMs(timeout));
        _CheckErr(ir, "libusb_bulk_transfer failed");
        return xferLen;
    }
    
    template <typename T>
    void write(uint8_t epAddr, T& x, Milliseconds timeout=Forever) {
        write(epAddr, (void*)&x, sizeof(x), timeout);
    }
    
    void write(uint8_t epAddr, const void* buf, size_t len, Milliseconds timeout=Forever) {
        _claimInterfaceForEndpointAddr(epAddr);
        
        int xferLen = 0;
        int ir = libusb_bulk_transfer(_handle, epAddr, (uint8_t*)buf, (int)len, &xferLen,
            _LibUSBTimeoutFromMs(timeout));
        _CheckErr(ir, "libusb_bulk_transfer failed");
        if ((size_t)xferLen != len)
            throw RuntimeError("libusb_bulk_transfer short write (tried: %zu, got: %zu)", len, (size_t)xferLen);
    }
    
    void reset(uint8_t epAddr) {
        _claimInterfaceForEndpointAddr(epAddr);
        int ir = libusb_clear_halt(_handle, epAddr);
        _CheckErr(ir, "libusb_clear_halt failed");
    }
    
    template <typename T>
    void vendorRequestOut(uint8_t req, const T& x, Milliseconds timeout=Forever) {
        vendorRequestOut(req, (void*)&x, sizeof(x), timeout);
    }
    
    void vendorRequestOut(uint8_t req, const void* data, size_t len, Milliseconds timeout=Forever) {
        _openIfNeeded();
        
        const uint8_t bmRequestType =
            USB::RequestType::DirectionOut      |
            USB::RequestType::TypeVendor        |
            USB::RequestType::RecipientDevice   ;
        const uint8_t bRequest = req;
        const uint8_t wValue = 0;
        const uint8_t wIndex = 0;
        int ir = libusb_control_transfer(_handle, bmRequestType, bRequest, wValue, wIndex,
            (uint8_t*)data, len, _LibUSBTimeoutFromMs(timeout));
        _CheckErr(ir, "libusb_control_transfer failed");
    }
    
    operator libusb_device*() const { return _dev; }
    
private:
    struct _EndpointInfo {
        bool valid = false;
        uint8_t epAddr = 0;
        uint8_t ifaceIdx = 0;
        uint16_t maxPacketSize = 0;
    };
    
    static libusb_context* _USBCtx() {
        static std::once_flag Once;
        static libusb_context* Ctx = nullptr;
        std::call_once(Once, [](){
            int ir = libusb_init(&Ctx);
            _CheckErr(ir, "libusb_init failed");
        });
        return Ctx;
    }
    
    static uint8_t _OffsetForEndpointAddr(uint8_t epAddr) {
        return ((epAddr&USB::Endpoint::DirectionMask)>>3) | (epAddr&USB::Endpoint::IndexMask);
    }
    
    static unsigned int _LibUSBTimeoutFromMs(Milliseconds timeout) {
        if (timeout == Forever) return 0;
        else if (timeout == Milliseconds::zero()) return 1;
        else return timeout.count();
    }
    
    static void _CheckErr(int ir, const char* errMsg) {
        if (ir < 0) throw RuntimeError("%s: %s", errMsg, libusb_error_name(ir));
    }
    
    void _openIfNeeded() {
        if (_handle.hasValue()) return;
        libusb_device_handle* handle = nullptr;
        int ir = libusb_open(_dev, &handle);
        _CheckErr(ir, "libusb_open failed");
        _handle = handle;
    }
    
    void _claimInterfaceForEndpointAddr(uint8_t epAddr) {
        _openIfNeeded();
        const _EndpointInfo& epInfo = _epInfo(epAddr);
        _Interface& iface = _interfaces.at(epInfo.ifaceIdx);
        if (!iface.claimed) {
            int ir = libusb_claim_interface(_handle, iface.bInterfaceNumber);
            _CheckErr(ir, "libusb_claim_interface failed");
            iface.claimed = true;
        }
    }
    
    const _EndpointInfo& _epInfo(uint8_t epAddr) const {
        const _EndpointInfo& epInfo = _epInfos[_OffsetForEndpointAddr(epAddr)];
        if (!epInfo.valid) throw RuntimeError("invalid endpoint address: 0x%02x", epAddr);
        return epInfo;
    }
    
    static void _Retain(libusb_device* x) { libusb_ref_device(x); }
    static void _Release(libusb_device* x) { libusb_unref_device(x); }
    using _LibusbDev = RefCounted<libusb_device*, _Retain, _Release>;
    
    static void _Close(libusb_device_handle* x) { libusb_close(x); }
    using _LibusbHandle = Uniqued<libusb_device_handle*, _Close>;
    
    _LibusbDev _dev = {};
    _LibusbHandle _handle = {};
    std::vector<_Interface> _interfaces = {};
    _EndpointInfo _epInfos[USB::Endpoint::MaxCount] = {};
    
public:
    
    uint16_t maxPacketSize(uint8_t epAddr) const {
        const _EndpointInfo& epInfo = _epInfo(epAddr);
        return epInfo.maxPacketSize;
    }
    
    std::string manufacturer() {
        return stringDescriptor(deviceDescriptor().iManufacturer).asciiString();
    }
    
    std::string product() {
        return stringDescriptor(deviceDescriptor().iProduct).asciiString();
    }
    
    std::string serialNumber() {
        return stringDescriptor(deviceDescriptor().iSerialNumber).asciiString();
    }
    
    std::vector<uint8_t> endpoints() {
        std::vector<uint8_t> eps;
        for (const _EndpointInfo& epInfo : _epInfos) {
            if (epInfo.valid) {
                eps.push_back(epInfo.epAddr);
            }
        }
        return eps;
    }
};

} // namespace Toastbox
