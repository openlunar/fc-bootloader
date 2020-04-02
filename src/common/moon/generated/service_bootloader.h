
#ifdef __cplusplus
	extern "C" {
#endif

#ifndef SERVICE_BOOTLOADER_H
#define SERVICE_BOOTLOADER_H

#include "moon/services/bootloader.h"
#include "moon/common.h"

#include <stdint.h>

int service_bootloader_handler( moon_msg_t * message );

// TODO: These could just be moved into the "service_bootloader.c" file (they're only used in one file, so reason to have them in this header; no one else should be calling them). The "service_bootloader_handler()" could be moved into a "services.h" file which "services.c" uses (also private). The only function that is "public facing" is "moon_services_handler" which is defined in the "moon/server.h" file.
// TODO: Write a weak definition for the "moon_services_handler" function (?) - more thought needed
int bl_ping_shim( moon_msg_t * message );

int bl_writePageBuffer_shim( moon_msg_t * message );

int bl_erasePageBuffer_shim( moon_msg_t * message );

int bl_eraseApp_shim( moon_msg_t * message );

int bl_writePage_shim( moon_msg_t * message );

int bl_setBootAction_shim( moon_msg_t * message );

int bl_boot_shim( moon_msg_t * message );

#endif // SERVICE_BOOTLOADER_H
