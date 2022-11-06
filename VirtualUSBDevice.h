#pragma once
#include <thread>
#include <cerrno>
#include <functional>
#include <optional>
#include <cassert>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <set>
#include <chrono>
#include <sys/socket.h>
#include "USBIP.h"
#include "USBIPLib.h"
#include "LIB/Toastbox/Endian.h"
#include "LIB/Toastbox/USB.h"
#include "LIB/Toastbox/RuntimeError.h"
using namespace std::chrono_literals;

// Macros until C++ supports class-scoped namespace aliases / `using namespace` in class scope
#define USB             Toastbox::USB
#define RuntimeError    Toastbox::RuntimeError
#define Endian          Toastbox::Endian

class VirtualUSBDevice
{

public:
    struct Info
    {
        const USB::DeviceDescriptor* deviceDesc = nullptr;
        const USB::DeviceQualifierDescriptor* deviceQualifierDesc = nullptr;
        const USB::ConfigurationDescriptor*const* configDescs = nullptr;
        size_t configDescsCount = 0;
        const USB::StringDescriptor*const* stringDescs = nullptr;
        size_t stringDescsCount = 0;
        bool throwOnErr = false;
    };
    
    using Err = std::exception_ptr;
    static const inline Err ErrStopped = std::make_exception_ptr(std::runtime_error("VirtualUSBDevice stopped"));
    
    struct Xfer
    {
        uint8_t ep = 0;
        USB::SetupRequest setupReq = {};
        std::unique_ptr<uint8_t[]> data;
        size_t len = 0;
    };
    
    struct _Cmd
    {
        USBIP::HEADER header = {};
        std::unique_ptr<uint8_t[]> payload = {};
        size_t payloadLen = 0;
    };
    
    using _Rep = _Cmd;
    
    static const std::exception& ErrExtract(Err err);

    VirtualUSBDevice(const Info& info);
    
    ~VirtualUSBDevice();
    
    void start();
    
    void stop();
    
    std::optional<Xfer> read(std::chrono::milliseconds timeout=std::chrono::milliseconds::max());
    
    void write(uint8_t ep, const void* data, size_t len);
    
    Err err();
    
private:
    static constexpr uint8_t _DeviceID = 1;
    
    USB::SetupRequest _GetSetupRequest(const _Cmd& cmd) const;
    
    uint8_t _GetEndpointAddr(const _Cmd& cmd);
    
    static void _Read(int socket, void* data, size_t len);
    
    static void _Write(int socket, const void* data, size_t len);
    
    static _Cmd _ReadCmd(int socket);
    
    static uint32_t _SpeedFromBCDUSB(uint16_t bcdUSB);
    
    static size_t _DescLen(const USB::DeviceDescriptor& d);
    
    static size_t _DescLen(const USB::ConfigurationDescriptor& d);
    
    static size_t _DescLen(const USB::DeviceQualifierDescriptor& d);
    
    static size_t _DescLen(const USB::StringDescriptor& d);
    
    static uint8_t _ConfigVal(const USB::ConfigurationDescriptor& d);
    
    static bool _SelfPowered(const USB::ConfigurationDescriptor& d);
    
    void _readThread();
    
    void _writeThread();
    
    void _reply(const _Cmd& cmd, const void* data, size_t len, int32_t status);
    
    std::optional<Xfer> _handleCmd(_Cmd& cmd);
    
    std::optional<Xfer> _handleCmdSubmitEP0(_Cmd& cmd);
    
    std::optional<Xfer> _handleCmdSubmitEPX(_Cmd& cmd);
    
    Xfer _handleCmdSubmitEPXOut(_Cmd& cmd);
    
    void _handleCmdSubmitEPXIn(_Cmd& cmd);
    
    void _sendDataForInEndpoint(uint8_t epIdx);
    
    void _handleCmdUnlink(const _Cmd& cmd);
    
    void _handleCmdSubmitEP0StandardRequest(const _Cmd& cmd, const USB::SetupRequest& req);
    
    void _reset(std::unique_lock<std::mutex>& lock, Err err);
    
    const Info _info = {};
    


#undef USB
#undef RuntimeError
#undef Endian
};