// This example is not self-contained.
// It requres usage of the example driver specific to your platform.
// See the HAL documentation.

#include "bindings/hal_common.h"
#include "bindings/bricklet_isolator.h"

#define UID "XYZ" // Change XYZ to the UID of your Isolator Bricklet

void check(int rc, const char* msg);

void example_setup(TF_HalContext *hal);
void example_loop(TF_HalContext *hal);


static TF_Isolator i;

void example_setup(TF_HalContext *hal) {
	// Create device object
	check(tf_isolator_create(&i, UID, hal), "create device object");

	// Get current statistics
	uint32_t messages_from_brick, messages_from_bricklet;
	uint16_t connected_bricklet_device_identifier; char connected_bricklet_uid[8];
	check(tf_isolator_get_statistics(&i, &messages_from_brick, &messages_from_bricklet,
	                                 &connected_bricklet_device_identifier,
	                                 connected_bricklet_uid), "get statistics");

	tf_hal_printf("Messages From Brick: %I32u\n", messages_from_brick);
	tf_hal_printf("Messages From Bricklet: %I32u\n", messages_from_bricklet);
	tf_hal_printf("Connected Bricklet Device Identifier: %I16u\n", connected_bricklet_device_identifier);
	tf_hal_printf("Connected Bricklet UID: %s\n", connected_bricklet_uid);
}

void example_loop(TF_HalContext *hal) {
	// Poll for callbacks
	tf_hal_callback_tick(hal, 0);
}
