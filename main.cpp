#include <climits>
#include "VirtualUSBDevice.h"
#include "Descriptor.h"

/* sudo apt-get install libudev-dev */
/* sudo modprobe vhci-hcd */
/* specify VID and PID in the Descriptor.h */

static Toastbox::USB::CDC::LineCoding _LineCoding = {};

static struct
{
    std::mutex lock;
    std::condition_variable signal;
    bool dtePresent = false;
} _State;

static void _handleXferEP0(VirtualUSBDevice& dev, VirtualUSBDevice::Xfer&& xfer)
{
    const Toastbox::USB::SetupRequest req = xfer.setupReq;
    const uint8_t* payload = xfer.data.get();
    const size_t payloadLen = xfer.len;
    
    // Verify that this request is a `Class` request
    if ((req.bmRequestType&Toastbox::USB::RequestType::TypeMask) != Toastbox::USB::RequestType::TypeClass)
        throw Toastbox::RuntimeError("invalid request bmRequestType (TypeClass)");
    
    // Verify that the recipient is the `Interface`
    if ((req.bmRequestType&Toastbox::USB::RequestType::RecipientMask) != Toastbox::USB::RequestType::RecipientInterface)
        throw Toastbox::RuntimeError("invalid request bmRequestType (RecipientInterface)");
    
    switch (req.bmRequestType&Toastbox::USB::RequestType::DirectionMask)
    {
    case Toastbox::USB::RequestType::DirectionOut:
        switch (req.bRequest)
        {
            case Toastbox::USB::CDC::Request::SET_LINE_CODING:
            {
                if (payloadLen != sizeof(_LineCoding))
                    throw Toastbox::RuntimeError("SET_LINE_CODING: payloadLen doesn't match sizeof(USB::CDC::LineCoding)");
                
                memcpy(&_LineCoding, payload, sizeof(_LineCoding));
                _LineCoding = {
                    .dwDTERate      = Toastbox::Endian::HFL_U32(_LineCoding.dwDTERate),
                    .bCharFormat    = Toastbox::Endian::HFL_U8(_LineCoding.bCharFormat),
                    .bParityType    = Toastbox::Endian::HFL_U8(_LineCoding.bParityType),
                    .bDataBits      = Toastbox::Endian::HFL_U8(_LineCoding.bDataBits),
                };
                
                printf("SET_LINE_CODING:\n");
                printf("  dwDTERate: %08x\n", _LineCoding.dwDTERate);
                printf("  bCharFormat: %08x\n", _LineCoding.bCharFormat);
                printf("  bParityType: %08x\n", _LineCoding.bParityType);
                printf("  bDataBits: %08x\n", _LineCoding.bDataBits);
                return;
            }
        
            case Toastbox::USB::CDC::Request::SET_CONTROL_LINE_STATE:
            {
                const bool dtePresent = req.wValue&1;
                printf("SET_CONTROL_LINE_STATE:\n");
                printf("  dtePresent=%d\n", dtePresent);
                auto lock = std::unique_lock(_State.lock);
                _State.dtePresent = dtePresent;
                _State.signal.notify_all();
                return;
            }
        
            case Toastbox::USB::CDC::Request::SEND_BREAK:
            {
                printf("SEND_BREAK:\n");
                return;
            }
        
            default:
                throw Toastbox::RuntimeError("invalid request (DirectionOut): %x", req.bRequest);
        }
    
    case Toastbox::USB::RequestType::DirectionIn:
        switch (req.bRequest)
        {
            case Toastbox::USB::CDC::Request::GET_LINE_CODING:
            {
                printf("GET_LINE_CODING\n");
                if (payloadLen != sizeof(_LineCoding))
                    throw Toastbox::RuntimeError("SET_LINE_CODING: payloadLen doesn't match sizeof(USB::CDC::LineCoding)");
                dev.write(Toastbox::USB::Endpoint::DefaultIn, &_LineCoding, sizeof(_LineCoding));
                return;
            }
            
            default:
                throw Toastbox::RuntimeError("invalid request (DirectionIn): %x", req.bRequest);
        }
    
    default:
        throw Toastbox::RuntimeError("invalid request direction");
    }
}

static void _handleXferEPX(VirtualUSBDevice& dev, VirtualUSBDevice::Xfer&& xfer)
{
    switch (xfer.ep)
    {
        case Endpoint::Out2:
        {
            printf("Endpoint::Out2: <");
            for (size_t i=0; i<xfer.len; i++)
            {
                printf(" %02x", xfer.data[i]);
            }
            printf(" >\n\n");
            break;
        }
    
    default:
        throw Toastbox::RuntimeError("invalid endpoint: 0x%02x", xfer.ep);
    }
}

static void _handleXfer(VirtualUSBDevice& dev, VirtualUSBDevice::Xfer&& xfer)
{
    if (xfer.ep == 0)    // Endpoint 0
        _handleXferEP0(dev, std::move(xfer));
    else    // Other endpoints
        _handleXferEPX(dev, std::move(xfer));
}

static void _threadResponse(VirtualUSBDevice& dev) {
    for (;;) {
        auto lock = std::unique_lock(_State.lock);
        while (!_State.dtePresent) {
            _State.signal.wait(lock);
        }
        lock.unlock();
        
        const char text[1024] = "Testing 123\r\n";
        dev.write(Endpoint::In2, text, sizeof(text));
        usleep(500000);
    }
}

int main(int argc, const char* argv[])
{
    const VirtualUSBDevice::Info deviceInfo = {
        .deviceDesc             = &Descriptor::Device,
        .deviceQualifierDesc    = &Descriptor::DeviceQualifier,
        .configDescs            = Descriptor::Configurations,
        .configDescsCount       = std::size(Descriptor::Configurations),
        .stringDescs            = Descriptor::Strings,
        .stringDescsCount       = std::size(Descriptor::Strings),
        .throwOnErr             = true,
    };
    
    VirtualUSBDevice dev(deviceInfo);
    try
    {
        try
        {
            dev.start();
        }
        catch (std::exception& e)
        {
            throw Toastbox::RuntimeError(
                "Failed to start VirtualUSBDevice: %s"                                  "\n"
                "Make sure:"                                                            "\n"
                "  - you're root"                                                       "\n"
                "  - the vhci-hcd kernel module is loaded: `sudo modprobe vhci-hcd`"    "\n",
                e.what()
            );
        }
        
        printf("Started\n");
        
        std::thread([&]{_threadResponse(dev);}).detach();
        
        for (;;)
        {
            VirtualUSBDevice::Xfer data = *dev.read();
            _handleXfer(dev, std::move(data));
        }
        
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "Error: %s\n", e.what());
        // Using _exit to avoid calling destructors for static vars, since that hangs
        // in __pthread_cond_destroy, because our thread is sitting in _State.signal.wait()
        _exit(1);
    }
    
    return 0;
}
