// NOTE: This file "autogenerated" (by Logan)

#include "moon/server.h"
#include "moon/codec.h"

// Supported services
#include "service_bootloader.h"

moon_ret_t moon_services_handler( moon_msg_t * message )
{
	switch ( message->header.service ) {
		case kBootloader_service_id:
			return service_bootloader_handler( message );
		default:
			message->header.protocol = MOON_PROT_E_NO_SERVICE;
			return MOON_RET_E_NO_SERVICE;
	}

	// -- Not reached -- //
}
