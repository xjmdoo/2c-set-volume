#include <stdio.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/i2c/IOI2CInterface.h>

kern_return_t setDisplayVolume(io_service_t service, int volume) {
    kern_return_t kr;
    io_service_t interface;
    IOI2CRequest request;
    IOI2CConnectRef connect;
    UInt8 sendData[7];

    sendData[0] = 0x51;
    sendData[1] = 0x84;
    sendData[2] = 0x03;
    sendData[3] = 0x62;
    sendData[4] = (UInt8)volume;
    sendData[5] = (UInt8)volume & 255;
    sendData[6] = 0x6E ^ sendData[0] ^ sendData[1] ^ sendData[2] ^ sendData[3] ^ sendData[4] ^ sendData[5];

    IOFBCopyI2CInterfaceForBus(service, 0, &interface);

    kr = IOI2CInterfaceOpen(interface, 0, &connect);
    if (kr != KERN_SUCCESS) {
        printf("Failed to open I2C interface: %d\n", kr);
        return kr;
    }

    memset(&request, 0, sizeof(request));
    request.commFlags = 0;
    request.sendAddress = 0x6E;
    request.sendTransactionType = kIOI2CSimpleTransactionType;
    request.sendBuffer = (vm_address_t)sendData;
    request.sendBytes = sizeof(sendData);
    request.replyTransactionType = kIOI2CNoTransactionType;
    request.replyBytes = 0;

    kr = IOI2CSendRequest(connect, 0, &request);
    if (kr != KERN_SUCCESS) {
        printf("Failed to send I2C request: %d\n", kr);
    }

    IOI2CInterfaceClose(connect, 0);

    return kr;
}

int strToInt(const char* str) {
    int num = 0;
    int len = strlen(str);

    for (int i = 0; i < len; i++) {
        num *= 10;
        num += str[i] - '0';
    }

    return num;
}

int main(int argc, const char** argv) {
    if (argc < 4) {
        printf("[usage] i2c_set_volume <volume:[0-100]> <vendor_id> <product_id>\n");
        return 0;
    }

    io_iterator_t iter;
    io_service_t service;
    CFDictionaryRef displayInfo;
    CFNumberRef vendorIDRef;
    int volume = strToInt(argv[1]);
    int selectedVendorId = strToInt(argv[2]);
    int selectedProductId = strToInt(argv[3]);
    int vendorId;
    int productId;
    bool found = false;

    kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching("IOFramebuffer"), &iter);
    if (kr != KERN_SUCCESS) {
        printf("Failed to find displays: %d\n", kr);
        return -1;
    }

    while ((service = IOIteratorNext(iter)) != 0) {
        displayInfo = IODisplayCreateInfoDictionary(service, kIODisplayOnlyPreferredName);
        if (displayInfo) {
            vendorIDRef = CFDictionaryGetValue(displayInfo, CFSTR(kDisplayVendorID));
            if (vendorIDRef && CFNumberGetValue(vendorIDRef, kCFNumberIntType, &vendorId)) {

                CFNumberRef productIDRef = CFDictionaryGetValue(displayInfo, CFSTR(kDisplayProductID));
                if (productIDRef && CFNumberGetValue(productIDRef, kCFNumberIntType, &productId)) {
                    if (vendorId == selectedVendorId && productId == selectedProductId) {
                        found = true;
                        printf("Found display with vendor ID = %d and product ID = %d\n", vendorId, productId);
                        IOItemCount busCount; 
                        IOFBGetI2CInterfaceCount(service, &busCount);
                        // printf("Bus count %d\n", busCount);
                        printf("Setting volume to %d%%\n", volume);
                        kr = setDisplayVolume(service, volume);
                        if (kr != KERN_SUCCESS) {
                            printf("Failed to set volume: %d\n", kr);
                        }
                    }
                }

            }

            CFRelease(displayInfo);
        }

        IOObjectRelease(service);
        if (found) {
            break;
        }
    }

    IOObjectRelease(iter);
    return 0;
}
