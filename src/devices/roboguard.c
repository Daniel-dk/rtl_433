
/** @file
    Decoder for Roboguard devices.
    Copyright (C) 2022 Daniel de Kock
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 */

/**
The device uses OOK PWM encoding, short pulse 1250us long pulse 2500us, and repeats 8 times
24 bit preamble 0xFFFFFF
You can use a flex decoder -X 'n=Roboguard,m=OOK_PWM,s=1250,l=2500,r=30000,g=12000,y=13000,t=100,repeats>=5,preamble={24}0xffffff
Powercode packet structure is 32 bits,3 examples follow
		   data     addr
		00111010 01111001 10001100 - IR trigger
		10111010 01111001 10001100 - Tamper switch opened
		11011010 01111001 10001100 - Battery low
		|||
		|||      16bit Serial number
		|||
		|||
	 Tamper_/||
	  Battery/|
	    Alarm_/

Protocol sniffed form several roboguard devices:
*/

#include "decoder.h"

static int roboguard_decode(r_device *decoder, bitbuffer_t *bitbuffer)
{
    uint8_t *msg;
    data_t *data;
    int id;

    // 32 bits expected per row, 8 packet repetitions, OK if we see at least 5
	 decoder_log(decoder, 2, __func__, "repeat searching for repeats ");

	int row = bitbuffer_find_repeated_row(bitbuffer, 5, 24);
	decoder_log(decoder, 2, __func__, "repeat search found a row: ");


     if (row < 0)
     {
        decoder_log(decoder, 2, __func__, "DECODE_ABORT_LENGTH: repeat row search failed");
        return DECODE_ABORT_LENGTH;
     }


    // exit if incorrect number of bits in row
    if (bitbuffer->bits_per_row[row] != 24) {
        decoder_log(decoder, 2, __func__, "DECODE_ABORT_LENGTH: too few bits in a row");
        return DECODE_ABORT_LENGTH;
	}

    // extract message, don't drop any bits, keep  32
    //bitbuffer_extract_bytes(bitbuffer, row, 0, msg, 32);
	msg = bitbuffer->bb[row];

    // No need to decode/extract values for simple test
    if (!msg[0] && !msg[1] && !msg[2] && !msg[3] ) {
        decoder_log(decoder, 2, __func__, "DECODE_FAIL_SANITY data all 0x00");
        return DECODE_FAIL_SANITY;
    }

    // debug
    decoder_logf(decoder, 2, __func__, "data byte is %02x", msg[0]);

    // format device id ( last 16 bits)
    //sprintf(id, "%02x%02x", msg[1], msg[2]);
     id = ( msg[1] << 8 | msg[2] );

    // populate data byte fields
    /* clang-format off */
    data = data_make(
            "model",        "Model",        DATA_STRING, "Roboguard",
            "id",           "ID",           DATA_INT, 	id,
            "tamper",       "Tamper",       DATA_INT,    ((0x80 & msg[0]) == 0x80) ? 1 : 0,
            "alarm",        "Alarm",        DATA_INT,    ((0x20 & msg[0]) == 0x20) ? 1 : 0,
            "battery_ok",   "Battery",      DATA_INT,    ((0x40 & msg[0]) == 0x40) ? 0 : 1,
            NULL);
    /* clang-format on */

    // return data
    decoder_output_data(decoder, data);
    return 1;
}

static char *output_fields[] = {
        "model",
        "id",
        "tamper",
        "alarm",
        "battery_ok",
        NULL,
};

r_device roboguard = {
        .name        = "Roboguard",
       // .modulation  = OOK_PULSE_PIWM_RAW,
	.modulation  = OOK_PULSE_PWM,
        .short_width = 1200,
        .long_width  = 2400,
	.sync_width  = 13000,
        .gap_limit   = 3600,
        .reset_limit = 28000,
	//.tolerance   = 100,
        .decode_fn   = &roboguard_decode,
        .fields      = output_fields,
};

