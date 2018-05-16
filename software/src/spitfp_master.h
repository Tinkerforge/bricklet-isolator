/* isolator-bricklet
 * Copyright (C) 2018 Olaf Lüke <olaf@tinkerforge.com>
 *
 * spitfp_master.h: Implementation of SPI TF Protocol Master
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

#ifndef SPITFP_MASTER_H
#define SPITFP_MASTER_H

#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/protocols/spitfp/spitfp.h"
#include "bricklib2/utility/ringbuffer.h"

#define SPITFP_MASTER_BUFFER_LENGTH 1024 // Use power of 2 here
#define SPITFP_MASTER_BUFFER_MASK   (SPITFP_MASTER_BUFFER_LENGTH-1)

typedef struct {
    uint32_t zero_count_recv;
    uint32_t counter_send;
    uint32_t counter_recv;

    uint8_t buffer_recv[SPITFP_MASTER_BUFFER_LENGTH];
    uint8_t buffer_send[TFP_MESSAGE_MAX_LENGTH + SPITFP_PROTOCOL_OVERHEAD*2]; // *2 for send message overhead and additional ACK

    uint8_t buffer_message_from_brick[TFP_MESSAGE_MAX_LENGTH];
    uint8_t buffer_message_from_brick_length;

    uint8_t buffer_message_from_bricklet[TFP_MESSAGE_MAX_LENGTH];
    uint8_t buffer_message_from_bricklet_length;

    uint8_t buffer_send_length;
    uint8_t buffer_send_index;

	uint8_t buffer_recv_tmp[TFP_MESSAGE_MAX_LENGTH + SPITFP_PROTOCOL_OVERHEAD*2];
	uint8_t buffer_recv_tmp_length;

	uint8_t current_sequence_number;
	uint8_t last_sequence_number_seen;
	uint32_t last_send_started;

    Ringbuffer ringbuffer_recv;
} SPITFPMaster;

extern SPITFPMaster spitfp_master;

void spitfp_master_init(void);
void spitfp_master_tick(void);

void spitfp_master_check(void);

#endif