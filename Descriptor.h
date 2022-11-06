#pragma once
#include "LIB/Toastbox/Endian.h"
#include "LIB/Toastbox/USB.h"

#define VID     0x1234  // Must be uint16_t
#define PID     0x5678  // Must be uint16_t
#if !defined(VID) || !defined(PID)
#error Define VID and PID first!
#endif

namespace Descriptor {

using namespace Toastbox::Endian;

constexpr Toastbox::USB::DeviceDescriptor Device = {
    .bLength                = LFH_U8(sizeof(Device)),
    .bDescriptorType        = LFH_U8(0x01),
    .bcdUSB                 = LFH_U16(0x0200),
    .bDeviceClass           = LFH_U8(0x02),
    .bDeviceSubClass        = LFH_U8(0x00),
    .bDeviceProtocol        = LFH_U8(0x00),
    .bMaxPacketSize0        = LFH_U8(0x10),
    .idVendor               = LFH_U16(VID),
    .idProduct              = LFH_U16(PID),
    .bcdDevice              = LFH_U16(0x0100),
    .iManufacturer          = LFH_U8(0x01), // String1
    .iProduct               = LFH_U8(0x02), // String2
    .iSerialNumber          = LFH_U8(0x00),
    .bNumConfigurations     = LFH_U8(0x01),
};

constexpr Toastbox::USB::DeviceQualifierDescriptor DeviceQualifier = {
    .bLength                = LFH_U8(sizeof(DeviceQualifier)),
    .bType                  = LFH_U8(0x06),
    .bcdUSB                 = LFH_U16(0x0200),
    .bDeviceClass           = LFH_U8(0x02),
    .bDeviceSubClass        = LFH_U8(0x00),
    .bDeviceProtocol        = LFH_U8(0x00),
    .bMaxPacketSize0        = LFH_U8(0x10),
    .bNumConfigurations     = LFH_U8(0x01),
    .bReserved              = LFH_U8(0x00),
};

struct Configuration
{
    Toastbox::USB::ConfigurationDescriptor configDesc;
    Toastbox::USB::InterfaceDescriptor iface0Desc;
    Toastbox::USB::CDC::HeaderFunctionalDescriptor headerFuncDesc;
    Toastbox::USB::CDC::CallManagementFunctionalDescriptor callManagementFuncDesc;
    Toastbox::USB::CDC::AbstractControlManagementFunctionalDescriptor acmFuncDesc;
    Toastbox::USB::CDC::UnionFunctionalDescriptor unionFuncDesc;
    Toastbox::USB::EndpointDescriptor epIn1Desc;
    
    Toastbox::USB::InterfaceDescriptor iface1Desc;
    Toastbox::USB::EndpointDescriptor epOut2Desc;
    Toastbox::USB::EndpointDescriptor epIn2Desc;
} __attribute__((packed));

constexpr Configuration Configuration = {
    .configDesc = {
        .bLength                        = LFH_U8(sizeof(Toastbox::USB::ConfigurationDescriptor)),
        .bDescriptorType                = LFH_U8(Toastbox::USB::DescriptorType::Configuration),
        .wTotalLength                   = LFH_U16(sizeof(Configuration)),
        .bNumInterfaces                 = LFH_U8(0x02),
        .bConfigurationValue            = LFH_U8(0x01),
        .iConfiguration                 = LFH_U8(0x00),
        .bmAttributes                   = LFH_U8(0xC0),
        .bMaxPower                      = LFH_U8(0x32),
    },
    
        .iface0Desc = {
            .bLength                    = LFH_U8(sizeof(Toastbox::USB::InterfaceDescriptor)),
            .bDescriptorType            = LFH_U8(Toastbox::USB::DescriptorType::Interface),
            .bInterfaceNumber           = LFH_U8(0x00),
            .bAlternateSetting          = LFH_U8(0x00),
            .bNumEndpoints              = LFH_U8(0x01),
            .bInterfaceClass            = LFH_U8(0x02), // Communication Interface Class
            .bInterfaceSubClass         = LFH_U8(0x02), // Abstract Control Model
            .bInterfaceProtocol         = LFH_U8(0x01), // Common AT commands
            .iInterface                 = LFH_U8(0x00),
        },
        
            .headerFuncDesc = {
                .bFunctionLength        = LFH_U8(0x05),
                .bDescriptorType        = LFH_U8(0x24),
                .bDescriptorSubtype     = LFH_U8(0x00),
                .bcdCDC                 = LFH_U16(0x0110),
            },
            
            .callManagementFuncDesc = {
                .bFunctionLength        = LFH_U8(0x05),
                .bDescriptorType        = LFH_U8(0x24),
                .bDescriptorSubtype     = LFH_U8(0x01),
                .bmCapabilities         = LFH_U8(0x01),
                .bDataInterface         = LFH_U8(0x01),
            },
            
            .acmFuncDesc = {
                .bFunctionLength        = LFH_U8(0x04),
                .bDescriptorType        = LFH_U8(0x24),
                .bDescriptorSubtype     = LFH_U8(0x02),
                .bmCapabilities         = LFH_U8(0x02),
            },
            
            .unionFuncDesc = {
                .bFunctionLength        = LFH_U8(0x05),
                .bDescriptorType        = LFH_U8(0x24),
                .bDescriptorSubtype     = LFH_U8(0x06),
                .bMasterInterface       = LFH_U8(0x00),
                .bSlaveInterface0       = LFH_U8(0x01),
            },
            
            .epIn1Desc = {
                .bLength                = LFH_U8(sizeof(Toastbox::USB::EndpointDescriptor)),
                .bDescriptorType        = LFH_U8(Toastbox::USB::DescriptorType::Endpoint),
                .bEndpointAddress       = LFH_U8(0x81),
                .bmAttributes           = LFH_U8(0x03),
                .wMaxPacketSize         = LFH_U16(0x0008),
                .bInterval              = LFH_U8(0x0A),
            },
        
        .iface1Desc = {
            .bLength                    = LFH_U8(sizeof(Toastbox::USB::InterfaceDescriptor)),
            .bDescriptorType            = LFH_U8(Toastbox::USB::DescriptorType::Interface),
            .bInterfaceNumber           = LFH_U8(0x01),
            .bAlternateSetting          = LFH_U8(0x00),
            .bNumEndpoints              = LFH_U8(0x02),
            .bInterfaceClass            = LFH_U8(0x0A), // Data Interface Class
            .bInterfaceSubClass         = LFH_U8(0x00), // "should have a value of 00h"
            .bInterfaceProtocol         = LFH_U8(0x00), // "No class specific protocol required"
            .iInterface                 = LFH_U8(0x03), // String3
        },
        
            .epOut2Desc = {
                .bLength                = LFH_U8(sizeof(Toastbox::USB::EndpointDescriptor)),
                .bDescriptorType        = LFH_U8(Toastbox::USB::DescriptorType::Endpoint),
                .bEndpointAddress       = LFH_U8(0x02),
                .bmAttributes           = LFH_U8(0x02),
                .wMaxPacketSize         = LFH_U16(0x0020),
                .bInterval              = LFH_U8(0x00),
            },
            
            .epIn2Desc = {
                .bLength                = LFH_U8(sizeof(Toastbox::USB::EndpointDescriptor)),
                .bDescriptorType        = LFH_U8(Toastbox::USB::DescriptorType::Endpoint),
                .bEndpointAddress       = LFH_U8(0x82),
                .bmAttributes           = LFH_U8(0x02),
                .wMaxPacketSize         = LFH_U16(0x0020),
                .bInterval              = LFH_U8(0x00),
            },
};

const Toastbox::USB::ConfigurationDescriptor* Configurations[] = {
    (Toastbox::USB::ConfigurationDescriptor*)&Configuration,
};

constexpr auto String0 = Toastbox::USB::SupportedLanguagesDescriptorMake({0x0409});
constexpr auto String1 = Toastbox::USB::StringDescriptorMake("ManufacturerString");
constexpr auto String2 = Toastbox::USB::StringDescriptorMake("ProductString");
constexpr auto String3 = Toastbox::USB::StringDescriptorMake("InterfaceString");

const Toastbox::USB::StringDescriptor* Strings[] = {
    (Toastbox::USB::StringDescriptor*)&String0,
    (Toastbox::USB::StringDescriptor*)&String1,
    (Toastbox::USB::StringDescriptor*)&String2,
    (Toastbox::USB::StringDescriptor*)&String3,
};

} // namespace Descriptor

namespace Endpoint {
    constexpr uint8_t In1    = 0x81;
    constexpr uint8_t Out2   = 0x02;
    constexpr uint8_t In2    = 0x82;
} // namespace Endpoint