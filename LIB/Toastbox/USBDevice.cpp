#include "USBDevice.h"
#include "RuntimeError.h"
#include <libusb-1.0/libusb.h>
#include "USB.h"

// using namespace Toastbox;

struct _Interface {
    uint8_t bInterfaceNumber = 0;
    bool claimed = false;
};

std::vector<_Interface> _interfaces = {};



uint8_t OffsetForEndpointAddr(uint8_t epAddr)
{
    return ((epAddr&DIRECTION_MASK)>>3) | (epAddr&INDEX_MASK);
}

struct _EndpointInfo
{
    bool valid = false;
    uint8_t epAddr = 0;
    uint8_t ifaceIdx = 0;
    uint16_t maxPacketSize = 0;
};

const _EndpointInfo& _epInfo(uint8_t epAddr);
_EndpointInfo _epInfos[MAX_COUNT] = {};

const _EndpointInfo& _epInfo(uint8_t epAddr)
{
    const _EndpointInfo& epInfo = _epInfos[OffsetForEndpointAddr(epAddr)];
    if (!epInfo.valid) throw RUNTIME_ERROR("invalid endpoint address: 0x%02x", epAddr);
    return epInfo;
}



void check_err(int ir, const char* err_msg)
{
    if (ir < 0) throw RUNTIME_ERROR("%s: %s", err_msg, libusb_error_name(ir));
}

libusb_context* USB_ctx(void)
{
    static std::once_flag Once;
    static libusb_context* Ctx = nullptr;
    std::call_once(Once, [](){
        int ir = libusb_init(&Ctx);
        check_err(ir, "libusb_init failed");
    });
    return Ctx;
}

std::vector<Toastbox::USBDevice> get_free_devs(void)
{
    libusb_device** devs = nullptr;
    ssize_t devsCount = libusb_get_device_list(USB_ctx(), &devs);
    check_err((int)devsCount, "libusb_get_device_list failed");
    Defer( if (devs) libusb_free_device_list(devs, true); );
    
    std::vector<Toastbox::USBDevice> r;
    for (size_t i=0; i<(size_t)devsCount; i++)
    {
        r.push_back(devs[i]);
    }
    return r;
}

void close_libusb(libusb_device_handle* x)
{
    libusb_close(x);
}

using _LibusbHandle = Uniqued<libusb_device_handle*, close_libusb>;

_LibusbHandle _handle = {};

Toastbox::USBDevice::USBDevice(libusb_device* dev) : _dev(_LibusbDev::Retain, std::move(dev))
{
    assert(dev);
    
    // Populate _interfaces and _epInfos
    {
        struct libusb_config_descriptor* configDesc = nullptr;
        int ir = libusb_get_config_descriptor(_dev, 0, &configDesc);
        check_err(ir, "libusb_get_config_descriptor failed");
        
        for (uint8_t ifaceIdx=0; ifaceIdx<configDesc->bNumInterfaces; ifaceIdx++) {
            const struct libusb_interface& iface = configDesc->interface[ifaceIdx];
            
            // For now we're only looking at altsetting 0
            if (iface.num_altsetting < 1) throw RUNTIME_ERROR("interface has no altsettings");
            _interfaces.push_back({
                .bInterfaceNumber = iface.altsetting[0].bInterfaceNumber,
            });
            
            const struct libusb_interface_descriptor& ifaceDesc = iface.altsetting[0];
            for (uint8_t epIdx=0; epIdx<ifaceDesc.bNumEndpoints; epIdx++) {
                const struct libusb_endpoint_descriptor& endpointDesc = ifaceDesc.endpoint[epIdx];
                const uint8_t epAddr = endpointDesc.bEndpointAddress;
                _epInfos[OffsetForEndpointAddr(epAddr)] = _EndpointInfo{
                    .valid          = true,
                    .epAddr         = epAddr,
                    .ifaceIdx       = ifaceIdx,
                    .maxPacketSize  = endpointDesc.wMaxPacketSize,
                };
            }
        }
    }
}

Toastbox::USB::DeviceDescriptor Toastbox::USBDevice::deviceDescriptor() const
{
    struct libusb_device_descriptor desc;
    int ir = libusb_get_device_descriptor(_dev, &desc);
    check_err(ir, "libusb_get_device_descriptor failed");
    
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

Toastbox::USB::ConfigurationDescriptor Toastbox::USBDevice::configurationDescriptor(uint8_t idx) const
{
    struct libusb_config_descriptor* desc;
    int ir = libusb_get_config_descriptor(_dev, idx, &desc);
    check_err(ir, "libusb_get_config_descriptor failed");
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

Toastbox::USB::StringDescriptorMax Toastbox::USBDevice::stringDescriptor(uint8_t idx, uint16_t lang)
{
    _openIfNeeded();
    
    USB::StringDescriptorMax desc;
    int ir = libusb_get_descriptor(_handle, USB::DescriptorType::String, idx, (uint8_t*)&desc, sizeof(desc));
    check_err(ir, "libusb_get_string_descriptor failed");
    desc.bLength = ir;
    return desc;
}

template <typename T>
void Toastbox::USBDevice::read(uint8_t epAddr, T& t, Milliseconds timeout)
{
    const size_t len = read(epAddr, (void*)&t, sizeof(t), timeout);
    if (len != sizeof(t))
        throw RUNTIME_ERROR("read() didn't read enough data (needed %ju bytes, got %ju bytes)", (uintmax_t)sizeof(t), (uintmax_t)len);
}

// #warning TODO: loop if ior==interrupted
size_t Toastbox::USBDevice::read(uint8_t epAddr, void* buf, size_t len, Milliseconds timeout)
{
    _claimInterfaceForEndpointAddr(epAddr);
    int xferLen = 0;
    int ir = libusb_bulk_transfer(_handle, epAddr, (uint8_t*)buf, (int)len, &xferLen, _LibUSBTimeoutFromMs(timeout));
    check_err(ir, "libusb_bulk_transfer failed");
    return xferLen;
}

template <typename T>
void Toastbox::USBDevice::write(uint8_t epAddr, T& x, Milliseconds timeout)
{
    write(epAddr, (void*)&x, sizeof(x), timeout);
}

void Toastbox::USBDevice::write(uint8_t epAddr, const void* buf, size_t len, Milliseconds timeout)
{
    _claimInterfaceForEndpointAddr(epAddr);
    
    int xferLen = 0;
    int ir = libusb_bulk_transfer(_handle, epAddr, (uint8_t*)buf, (int)len, &xferLen,
        _LibUSBTimeoutFromMs(timeout));
    check_err(ir, "libusb_bulk_transfer failed");
    if ((size_t)xferLen != len)
        throw RUNTIME_ERROR("libusb_bulk_transfer short write (tried: %zu, got: %zu)", len, (size_t)xferLen);
}

void Toastbox::USBDevice::reset(uint8_t epAddr)
{
    _claimInterfaceForEndpointAddr(epAddr);
    int ir = libusb_clear_halt(_handle, epAddr);
    check_err(ir, "libusb_clear_halt failed");
}

template <typename T>
void Toastbox::USBDevice::vendorRequestOut(uint8_t req, const T& x, Milliseconds timeout)
{
    vendorRequestOut(req, (void*)&x, sizeof(x), timeout);
}

void Toastbox::USBDevice::vendorRequestOut(uint8_t req, const void* data, size_t len, Milliseconds timeout)
{
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
    check_err(ir, "libusb_control_transfer failed");
}

bool Toastbox::USBDevice::operator==(const USBDevice& x) const
{
    return _dev == x._dev;
}

// uint8_t Toastbox::USBDevice::_OffsetForEndpointAddr(uint8_t epAddr)
// {
//     return ((epAddr&USB::Endpoint::DirectionMask)>>3) | (epAddr&USB::Endpoint::IndexMask);
// }

unsigned int Toastbox::USBDevice::_LibUSBTimeoutFromMs(Milliseconds timeout)
{
    if (timeout == Forever) return 0;
    else if (timeout == Milliseconds::zero()) return 1;
    else return timeout.count();
}

void Toastbox::USBDevice::_openIfNeeded()
{
    if (_handle.hasValue()) return;
    libusb_device_handle* handle = nullptr;
    int ir = libusb_open(_dev, &handle);
    check_err(ir, "libusb_open failed");
    _handle = handle;
}

void Toastbox::USBDevice::_claimInterfaceForEndpointAddr(uint8_t epAddr)
{
    _openIfNeeded();
    const _EndpointInfo& epInfo = _epInfo(epAddr);
    _Interface& iface = _interfaces.at(epInfo.ifaceIdx);
    if (!iface.claimed) {
        int ir = libusb_claim_interface(_handle, iface.bInterfaceNumber);
        check_err(ir, "libusb_claim_interface failed");
        iface.claimed = true;
    }
}

void Retain(libusb_device* x)
{
    libusb_ref_device(x);
}

void Release(libusb_device* x)
{
    libusb_unref_device(x);
}

uint16_t Toastbox::USBDevice::maxPacketSize(uint8_t epAddr) const
{
    const _EndpointInfo& epInfo = _epInfo(epAddr);
    return epInfo.maxPacketSize;
}

std::string Toastbox::USBDevice::manufacturer()
{
    return stringDescriptor(deviceDescriptor().iManufacturer).asciiString();
}

std::string Toastbox::USBDevice::product()
{
    return stringDescriptor(deviceDescriptor().iProduct).asciiString();
}

std::string Toastbox::USBDevice::serialNumber()
{
    return stringDescriptor(deviceDescriptor().iSerialNumber).asciiString();
}

std::vector<uint8_t> Toastbox::USBDevice::endpoints()
{
    std::vector<uint8_t> eps;
    for (const _EndpointInfo& epInfo : _epInfos) {
        if (epInfo.valid) {
            eps.push_back(epInfo.epAddr);
        }
    }
    return eps;
}
