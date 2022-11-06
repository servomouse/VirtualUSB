#pragma once
#include <cstdint>
#include <libudev.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

namespace USBIPLib {

constexpr uint32_t USBIP_CMD_SUBMIT = 1;
constexpr uint32_t USBIP_CMD_UNLINK = 2;
constexpr uint32_t USBIP_RET_SUBMIT = 3;
constexpr uint32_t USBIP_RET_UNLINK = 4;

constexpr uint32_t USBIP_DIR_OUT    = 0;
constexpr uint32_t USBIP_DIR_IN     = 1;

constexpr size_t SYSFS_PATH_MAX = 256;
constexpr size_t SYSFS_BUS_ID_SIZE = 32;
constexpr size_t MAX_STATUS_NAME = 18;
constexpr char USBIP_VHCI_BUS_TYPE[] = "platform";
constexpr char USBIP_VHCI_DEVICE_NAME[] = "vhci_hcd.0";

enum hub_speed
{
    HUB_SPEED_HIGH = 0,
    HUB_SPEED_SUPER,
};

enum usbip_status
{
    /* sdev is available. */
    SDEV_ST_AVAILABLE = 0x01,
    /* sdev is now used. */
    SDEV_ST_USED,
    /* sdev is unusable because of a fatal error. */
    SDEV_ST_ERROR,

    /* vdev does not connect a remote device. */
    VDEV_ST_NULL,
    /* vdev is used, but the USB address is not assigned yet */
    VDEV_ST_NOTASSIGNED,
    VDEV_ST_USED,
    VDEV_ST_ERROR
};

typedef struct __attribute__((packed)) usbip_usb_device_t
{
    char path[SYSFS_PATH_MAX];
    char busid[SYSFS_BUS_ID_SIZE];

    uint32_t busnum;
    uint32_t devnum;
    uint32_t speed;

    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;

    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bConfigurationValue;
    uint8_t bNumConfigurations;
    uint8_t bNumInterfaces;
} usbip_usb_device;

typedef struct usbip_imported_device_t
{
    enum hub_speed hub;
    uint8_t port;
    uint32_t status;

    uint32_t devid;

    uint8_t busnum;
    uint8_t devnum;

    /* usbip_class_device list */
    usbip_usb_device udev;
}usbip_imported_device;

typedef struct usbip_vhci_driver_t
{
    /* /sys/devices/platform/vhci_hcd */
    struct udev_device *hc_device;

    int ncontrollers;
    int nports;
    usbip_imported_device idev[];
}usbip_vhci_driver;

enum usb_device_speed
{
    USB_SPEED_UNKNOWN = 0,            /* enumerating */
    USB_SPEED_LOW, USB_SPEED_FULL,        /* usb 1.1 */
    USB_SPEED_HIGH,                /* usb 2.0 */
    USB_SPEED_WIRELESS,            /* wireless (usb 2.5) */
    USB_SPEED_SUPER,            /* usb 3.0 */
    USB_SPEED_SUPER_PLUS,            /* usb 3.1 */
};

struct speed_string
{
    int num;
    const char *speed;
    const char *desc;
};

static const struct speed_string speed_strings[] =
{
    { USB_SPEED_UNKNOWN, "unknown", "Unknown Speed"},
    { USB_SPEED_LOW,  "1.5", "Low Speed(1.5Mbps)"  },
    { USB_SPEED_FULL, "12",  "Full Speed(12Mbps)" },
    { USB_SPEED_HIGH, "480", "High Speed(480Mbps)" },
    { USB_SPEED_WIRELESS, "53.3-480", "Wireless"},
    { USB_SPEED_SUPER, "5000", "Super Speed(5000Mbps)" },
    { 0, NULL, NULL }
};


int usbip_net_set_keepalive(int sockfd);

int write_sysfs_attribute(const char *attr_path, const char *new_value, size_t len);

int read_attr_speed(struct udev_device *dev);

int read_attr_value(struct udev_device *dev, const char *name, const char *format);

int read_usb_device(struct udev_device *sdev, usbip_usb_device *udev);

int usbip_vhci_driver_open(void);

int usbip_vhci_get_free_port(uint32_t speed);

int usbip_vhci_attach_device2(uint8_t port, int sockfd, uint32_t devid, uint32_t speed);

void usbip_vhci_driver_close(void);

} // namespace USBIPLib