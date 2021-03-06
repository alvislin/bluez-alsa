/*
 * test-utils.c
 * Copyright (c) 2016-2018 Arkadiusz Bokowy
 *
 * This file is a part of bluez-alsa.
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#include "inc/test.inc"
#include "../src/utils.c"
#include "../src/shared/defs.h"
#include "../src/shared/ffb.c"
#include "../src/shared/rt.c"

int test_dbus_profile_object_path(void) {

	static const struct {
		enum bluetooth_profile profile;
		int16_t codec;
		const char *path;
	} profiles[] = {
		/* test null/invalid path */
		{ BLUETOOTH_PROFILE_NULL, -1, "/" },
		{ BLUETOOTH_PROFILE_NULL, -1, "/Invalid" },
		/* test A2DP profiles */
		{ BLUETOOTH_PROFILE_A2DP_SOURCE, A2DP_CODEC_SBC, "/A2DP/SBC/Source" },
		{ BLUETOOTH_PROFILE_A2DP_SOURCE, A2DP_CODEC_SBC, "/A2DP/SBC/Source/1" },
		{ BLUETOOTH_PROFILE_A2DP_SOURCE, A2DP_CODEC_SBC, "/A2DP/SBC/Source/2" },
		{ BLUETOOTH_PROFILE_A2DP_SINK, A2DP_CODEC_SBC, "/A2DP/SBC/Sink" },
#if ENABLE_MPEG
		{ BLUETOOTH_PROFILE_A2DP_SOURCE, A2DP_CODEC_MPEG12, "/A2DP/MPEG12/Source" },
		{ BLUETOOTH_PROFILE_A2DP_SINK, A2DP_CODEC_MPEG12, "/A2DP/MPEG12/Sink" },
#endif
#if ENABLE_AAC
		{ BLUETOOTH_PROFILE_A2DP_SOURCE, A2DP_CODEC_MPEG24, "/A2DP/MPEG24/Source" },
		{ BLUETOOTH_PROFILE_A2DP_SINK, A2DP_CODEC_MPEG24, "/A2DP/MPEG24/Sink" },
#endif
#if ENABLE_APTX
		{ BLUETOOTH_PROFILE_A2DP_SOURCE, A2DP_CODEC_VENDOR_APTX, "/A2DP/APTX/Source" },
		{ BLUETOOTH_PROFILE_A2DP_SINK, A2DP_CODEC_VENDOR_APTX, "/A2DP/APTX/Sink" },
#endif
		/* test HSP/HFP profiles */
		{ BLUETOOTH_PROFILE_HSP_HS, -1, "/HSP/Headset" },
		{ BLUETOOTH_PROFILE_HSP_AG, -1, "/HSP/AudioGateway" },
		{ BLUETOOTH_PROFILE_HFP_HF, -1, "/HFP/HandsFree" },
		{ BLUETOOTH_PROFILE_HFP_AG, -1, "/HFP/AudioGateway" },
	};

	size_t i;

	for (i = 0; i < ARRAYSIZE(profiles); i++) {
		const char *path = g_dbus_get_profile_object_path(profiles[i].profile, profiles[i].codec);
		assert(strstr(profiles[i].path, path) == profiles[i].path);
		assert(g_dbus_object_path_to_profile(profiles[i].path) == profiles[i].profile);
	}

	return 0;
}

int test_pcm_scale_s16le(void) {

	const int16_t mute[] = { 0x0000, 0x0000, 0x0000, 0x0000 };
	const int16_t half[] = { 0x1234 / 2, 0x2345 / 2, (int16_t)0xBCDE / 2, (int16_t)0xCDEF / 2 };
	const int16_t halfl[] = { 0x1234 / 2, 0x2345, (int16_t)0xBCDE / 2, 0xCDEF };
	const int16_t halfr[] = { 0x1234, 0x2345 / 2, 0xBCDE, (int16_t)0xCDEF / 2 };
	const int16_t in[] = { 0x1234, 0x2345, 0xBCDE, 0xCDEF };
	int16_t tmp[ARRAYSIZE(in)];

	memcpy(tmp, in, sizeof(tmp));
	snd_pcm_scale_s16le(tmp, ARRAYSIZE(tmp), 1, 0, 0);
	assert(memcmp(tmp, mute, sizeof(mute)) == 0);

	memcpy(tmp, in, sizeof(tmp));
	snd_pcm_scale_s16le(tmp, ARRAYSIZE(tmp), 1, 1.0, 1.0);
	assert(memcmp(tmp, in, sizeof(in)) == 0);

	memcpy(tmp, in, sizeof(tmp));
	snd_pcm_scale_s16le(tmp, ARRAYSIZE(tmp), 1, 0.5, 0.5);
	assert(memcmp(tmp, half, sizeof(half)) == 0);

	memcpy(tmp, in, sizeof(tmp));
	snd_pcm_scale_s16le(tmp, ARRAYSIZE(tmp), 2, 0.5, 1.0);
	assert(memcmp(tmp, halfl, sizeof(halfl)) == 0);

	memcpy(tmp, in, sizeof(tmp));
	snd_pcm_scale_s16le(tmp, ARRAYSIZE(tmp), 2, 1.0, 0.5);
	assert(memcmp(tmp, halfr, sizeof(halfr)) == 0);

	return 0;
}

int test_difftimespec(void) {

	struct timespec ts1, ts2, ts;

	ts1.tv_sec = ts2.tv_sec = 12345;
	ts1.tv_nsec = ts2.tv_nsec = 67890;
	assert(difftimespec(&ts1, &ts2, &ts) == 0);
	assert(ts.tv_sec == 0 && ts.tv_nsec == 0);

	ts1.tv_sec = 10;
	ts1.tv_nsec = 100000000;
	ts2.tv_sec = 10;
	ts2.tv_nsec = 500000000;
	assert(difftimespec(&ts1, &ts2, &ts) > 0);
	assert(ts.tv_sec == 0 && ts.tv_nsec == 400000000);

	ts1.tv_sec = 10;
	ts1.tv_nsec = 800000000;
	ts2.tv_sec = 12;
	ts2.tv_nsec = 100000000;
	assert(difftimespec(&ts1, &ts2, &ts) > 0);
	assert(ts.tv_sec == 1 && ts.tv_nsec == 300000000);

	ts1.tv_sec = 10;
	ts1.tv_nsec = 500000000;
	ts2.tv_sec = 10;
	ts2.tv_nsec = 100000000;
	assert(difftimespec(&ts1, &ts2, &ts) < 0);
	assert(ts.tv_sec == 0 && ts.tv_nsec == 400000000);

	ts1.tv_sec = 12;
	ts1.tv_nsec = 100000000;
	ts2.tv_sec = 10;
	ts2.tv_nsec = 800000000;
	assert(difftimespec(&ts1, &ts2, &ts) < 0);
	assert(ts.tv_sec == 1 && ts.tv_nsec == 300000000);

	return 0;
}

int test_fifo_buffer(void) {

	ffb_uint8_t ffb_u8 = { 0 };
	ffb_int16_t ffb_16 = { 0 };

	assert(ffb_init(&ffb_u8, 64) != NULL);
	assert(ffb_u8.data == ffb_u8.tail);
	assert(ffb_u8.size == 64);

	assert(ffb_init(&ffb_16, 64) != NULL);
	assert(ffb_16.data == ffb_16.tail);
	assert(ffb_16.size == 64);

	memcpy(ffb_u8.data, "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ", 36);
	ffb_seek(&ffb_u8, 36);

	memcpy(ffb_16.data, "11223344556677889900AABBCCDDEEFFGGHHIIJJKKLLMMNNOOPPQQRRSSTTUUVVWWXXYYZZ", 36 * 2);
	ffb_seek(&ffb_16, 36);

	assert(ffb_len_in(&ffb_u8) == 64 - 36);
	assert(ffb_blen_in(&ffb_u8) == 64 - 36);
	assert(ffb_len_out(&ffb_u8) == 36);
	assert(ffb_blen_out(&ffb_u8) == 36);
	assert(ffb_u8.tail[-1] == 'Z');

	assert(ffb_len_in(&ffb_16) == 64 - 36);
	assert(ffb_blen_in(&ffb_16) == (64 - 36) * 2);
	assert(ffb_len_out(&ffb_16) == 36);
	assert(ffb_blen_out(&ffb_16) == 36 * 2);
	assert(ffb_16.tail[-1] == 0x5a5a);

	ffb_shift(&ffb_u8, 15);
	assert(ffb_len_in(&ffb_u8) == 64 - (36 - 15));
	assert(ffb_len_out(&ffb_u8) == 36 - 15);
	assert(memcmp(ffb_u8.data, "FGHIJKLMNOPQRSTUVWXYZ", ffb_len_out(&ffb_u8)) == 0);
	assert(ffb_u8.tail[-1] == 'Z');

	ffb_rewind(&ffb_u8);
	assert(ffb_u8.data == ffb_u8.tail);

	ffb_uint8_free(&ffb_u8);
	assert(ffb_u8.data == NULL);

	return 0;
}

int main(void) {
	test_run(test_dbus_profile_object_path);
	test_run(test_pcm_scale_s16le);
	test_run(test_difftimespec);
	test_run(test_fifo_buffer);
	return 0;
}
