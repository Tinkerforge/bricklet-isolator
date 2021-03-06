/* isolator-bricklet
 * Copyright (C) 2018-2019 Olaf Lüke <olaf@tinkerforge.com>
 *
 * communication.c: TFP protocol message handling
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "communication.h"

#include <string.h>

#include "bricklib2/utility/communication_callback.h"
#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/hal/system_timer/system_timer.h"

#include "spitfp_master.h"
#include "configs/config_spitfp_master.h"

#include "xmc_usic.h"

void communication_inspect_message_from_bricklet_to_brick(void *message) {
	const uint8_t fid = tfp_get_fid_from_message(message);

	char uid[9] = {'\0'};
	tfp_uid_uint32_to_base58(tfp_get_uid_from_message(message), uid);

	switch(fid) {
		case TFP_FID_ENUMERATE_CALLBACK:
		case TFP_FID_GET_IDENTITY: {
			TFPGetIdentityReturn *gir = message;

			// Insert correct position and connected UID into
			// enumerate and identity messages
			gir->position = 'Z';
			memset(gir->connected_uid, '\0', TFP_UID_STR_MAX_LENGTH);
			tfp_uid_uint32_to_base58(bootloader_get_uid(), gir->connected_uid);

			// Save UID and DID so user can get it through statistics API
			memcpy(spitfp_master.connected_bricklet_uid, gir->uid, TFP_UID_STR_MAX_LENGTH);
			spitfp_master.connected_bricklet_device_identifier = gir->device_identifier;
		}
	}
}

BootloaderHandleMessageResponse handle_message(const void *message, void *response) {
	const uint32_t message_uid = tfp_get_uid_from_message(message);
	// If message is for isolator we handle it here
	if(message_uid == bootloader_get_uid()) {
		switch(tfp_get_fid_from_message(message)) {
			case FID_GET_STATISTICS: return get_statistics(message, response);
			case FID_SET_SPITFP_BAUDRATE_CONFIG: return set_spitfp_baudrate_config(message);
			case FID_GET_SPITFP_BAUDRATE_CONFIG: return get_spitfp_baudrate_config(message, response);
			case FID_SET_SPITFP_BAUDRATE: return set_spitfp_baudrate(message);
			case FID_GET_SPITFP_BAUDRATE: return get_spitfp_baudrate(message, response);
			case FID_GET_ISOLATOR_SPITFP_ERROR_COUNT: return get_isolator_spitfp_error_count(message, response);
			case FID_SET_STATISTICS_CALLBACK_CONFIGURATION: return set_statistics_callback_configuration(message);
			case FID_GET_STATISTICS_CALLBACK_CONFIGURATION: return get_statistics_callback_configuration(message, response);
			default: return HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED;
		}
	}
	
	// If there is space for a new message from the Brick we store it
	while(spitfp_master.buffer_message_from_brick_length != 0) {
		spitfp_master_tick();
	}

	spitfp_master.messages_from_brick++;
	const uint8_t length = tfp_get_length_from_message(message);
	memcpy(spitfp_master.buffer_message_from_brick, message, length);
	spitfp_master.buffer_message_from_brick_length = length;

#if 0
	// If there is data available to be send to bricklet we can send it here
	if(spitfp_master.buffer_message_from_bricklet_length > 0) {
		memcpy(response, spitfp_master.buffer_message_from_bricklet, spitfp_master.buffer_message_from_bricklet_length);
		spitfp_master.buffer_message_from_bricklet_length = 0;

		communication_inspect_message_from_bricklet_to_brick(response);

		return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
	}
#endif

	return HANDLE_MESSAGE_RESPONSE_NONE;
}

bool communication_handle_message_from_bricklet(uint8_t *message, const uint8_t length) {
	if(bootloader_spitfp_is_send_possible(&bootloader_status.st)) {
		spitfp_master.messages_from_bricklet++;

		communication_inspect_message_from_bricklet_to_brick(message);
		bootloader_spitfp_send_ack_and_message(&bootloader_status, message, length);
		return true;
	}

	return false;
}



BootloaderHandleMessageResponse get_statistics(const GetStatistics *data, GetStatistics_Response *response) {
	response->header.length                        = sizeof(GetStatistics_Response);
	response->messages_from_brick                  = spitfp_master.messages_from_brick;
	response->messages_from_bricklet               = spitfp_master.messages_from_bricklet;
	response->connected_bricklet_device_identifier = spitfp_master.connected_bricklet_device_identifier;
	memcpy(response->connected_bricklet_uid, spitfp_master.connected_bricklet_uid, 8);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_spitfp_baudrate_config(const SetSPITFPBaudrateConfig *data) {
	spitfp_master.enable_dynamic_baudrate  = data->enable_dynamic_baudrate;
	spitfp_master.minimum_dynamic_baudrate = BETWEEN(400000, data->minimum_dynamic_baudrate, 2000000);

	if(!spitfp_master.enable_dynamic_baudrate) {
		spitfp_master.baudrate_new = true;
	}

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_spitfp_baudrate_config(const GetSPITFPBaudrateConfig *data, GetSPITFPBaudrateConfig_Response *response) {
	response->header.length            = sizeof(GetSPITFPBaudrateConfig_Response);
	response->enable_dynamic_baudrate  = spitfp_master.enable_dynamic_baudrate;
	response->minimum_dynamic_baudrate = spitfp_master.minimum_dynamic_baudrate;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_spitfp_baudrate(const SetSPITFPBaudrate *data) {
	spitfp_master.baudrate = BETWEEN(400000, data->baudrate, 2000000);

	if(!spitfp_master.enable_dynamic_baudrate) {
		spitfp_master.baudrate_new = true;
	}

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_spitfp_baudrate(const GetSPITFPBaudrate *data, GetSPITFPBaudrate_Response *response) {
	response->header.length = sizeof(GetSPITFPBaudrate_Response);
	response->baudrate      = spitfp_master.baudrate;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_isolator_spitfp_error_count(const GetIsolatorSPITFPErrorCount *data, GetIsolatorSPITFPErrorCount_Response *response) {
	response->header.length                = sizeof(GetIsolatorSPITFPErrorCount_Response);
	response->error_count_ack_checksum     = spitfp_master.error_count_ack_checksum;
	response->error_count_frame            = spitfp_master.error_count_frame;
	response->error_count_message_checksum = spitfp_master.error_count_message_checksum;
	response->error_count_overflow         = spitfp_master.error_count_overflow;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}


BootloaderHandleMessageResponse set_statistics_callback_configuration(const SetStatisticsCallbackConfiguration *data) {
	spitfp_master.period              = data->period;
	spitfp_master.value_has_to_change = data->value_has_to_change;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_statistics_callback_configuration(const GetStatisticsCallbackConfiguration *data, GetStatisticsCallbackConfiguration_Response *response) {
	response->header.length       = sizeof(GetStatisticsCallbackConfiguration_Response);
	response->period              = spitfp_master.period;
	response->value_has_to_change = spitfp_master.value_has_to_change;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}


bool handle_statistics_callback(void) {
	static bool is_buffered = false;
	static Statistics_Callback cb;

	static uint32_t last_time = 0;
	static uint32_t last_messages_from_brick;
	static uint32_t last_messages_from_bricklet;
	static uint16_t last_connected_bricklet_device_identifier;

	if(!is_buffered) {
		if(spitfp_master.period == 0 || !system_timer_is_time_elapsed_ms(last_time, spitfp_master.period)) {
			return false;
		}

		// We ignore UID here, since we don't support hotplug anyway
		if(spitfp_master.messages_from_brick == last_messages_from_brick &&
		   spitfp_master.messages_from_bricklet == last_messages_from_bricklet &&
		   spitfp_master.connected_bricklet_device_identifier == last_connected_bricklet_device_identifier) {
			return false;
		}

		tfp_make_default_header(&cb.header, bootloader_get_uid(), sizeof(Statistics_Callback), FID_CALLBACK_STATISTICS);
		cb.messages_from_brick                  = spitfp_master.messages_from_brick;
		cb.messages_from_bricklet               = spitfp_master.messages_from_bricklet;
		cb.connected_bricklet_device_identifier = spitfp_master.connected_bricklet_device_identifier;
		memcpy(cb.connected_bricklet_uid, spitfp_master.connected_bricklet_uid, 8);

		last_messages_from_brick                  = cb.messages_from_brick;
		last_messages_from_bricklet               = cb.messages_from_bricklet;
		last_connected_bricklet_device_identifier = cb.connected_bricklet_device_identifier;

		last_time = system_timer_get_ms();
	}

	if(bootloader_spitfp_is_send_possible(&bootloader_status.st)) {
		bootloader_spitfp_send_ack_and_message(&bootloader_status, (uint8_t*)&cb, sizeof(Statistics_Callback));
		is_buffered = false;
		return true;
	} else {
		is_buffered = true;
	}

	return false;
}

void communication_tick(void) {
	communication_callback_tick();
}

void communication_init(void) {
	communication_callback_init();
}