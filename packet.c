#include <math.h>
//#include <WinSock2.h>
#include "packet.h"

//#define WINXP
//#ifdef WINXP
unsigned int htonf(float Value) {
	unsigned int *bytes = (unsigned int*)&Value;
	return bytes[0] << 24 |
		bytes[1] << 16 |
		bytes[2] << 8 |
		bytes[3];
}
float ntohf(unsigned Value) {
	unsigned int* bytes = (unsigned int*)&Value;
	unsigned int val = bytes[0] << 24 |
		bytes[1] << 16 |
		bytes[2] << 8 |
		bytes[3];
	return *((float *)&val);
}
//#endif

int packet_bytecount(int head) {
	return (head / 8) + ((head % 8) ? 1 : 0);
}
void packet_write_bit(uint8_t *buf, int *bitlen, int value) {
	if (value) buf[*bitlen / 8] |= (1 << (*bitlen % 8));
	else buf[*bitlen / 8] &= ~(1 << (*bitlen % 8));
	(*bitlen)++;
}
int packet_read_bit(const uint8_t *buf, int *bitlen) {
	int value = buf[*bitlen / 8] & (1 << (*bitlen % 8));
	(*bitlen)++;
	return value ? 1 : 0;
}
void packet_write_bits(uint8_t *buf, int *bitlen, uint64_t value, int numbits) {
	int i;
	for (i = 0; i < numbits; i++)
		packet_write_bit(buf, bitlen, value & (uint64_t)1 << (uint64_t)i);
}
uint64_t packet_read_bits(const uint8_t *buf, int *bitlen, int numbits) {
	uint64_t value = 0, i;
	for (i = 0; i < numbits; i++) {
		value |= (uint64_t)packet_read_bit(buf, bitlen) << i;
	}
	return value;
}
void packet_write_u8(uint8_t *buf, int *bitlen, uint8_t value) {
	if (!value) {
		packet_write_bit(buf, bitlen, 0);
	}
	else if (!(value & ~0xf)) {
		packet_write_bits(buf, bitlen, 1, 2);
		packet_write_bits(buf, bitlen, (uint64_t)value, 4);
	}
	else {
		packet_write_bits(buf, bitlen, 3, 2);
		packet_write_bits(buf, bitlen, (uint64_t)value, 8);
	}
}
void packet_write_u16(uint8_t *buf, int *bitlen, uint16_t value) {
	if (!value) {
		packet_write_bits(buf, bitlen, 0, 2);
	}
	else if (!(value & ~0x1f)) {
		packet_write_bits(buf, bitlen, 1, 2);
		packet_write_bits(buf, bitlen, (uint64_t)value, 5);
	}
	else if (!(value & ~0xff)) {
		packet_write_bits(buf, bitlen, 2, 2);
		packet_write_bits(buf, bitlen, (uint64_t)value, 8);
	}
	else {
		packet_write_bits(buf, bitlen, 3, 2);
		packet_write_bits(buf, bitlen, (uint64_t)value, 16);
	}
}
void packet_write_u32(uint8_t *buf, int *bitlen, uint32_t value) {
	if (!value) {
		packet_write_bits(buf, bitlen, 0, 2);
	}
	else if (!(value & ~0x3f)) {
		packet_write_bits(buf, bitlen, 1, 2);
		packet_write_bits(buf, bitlen, (uint64_t)value, 6);
	}
	else if (!(value & ~0x7ff)) {
		packet_write_bits(buf, bitlen, 2, 2);
		packet_write_bits(buf, bitlen, (uint64_t)value, 11);
	}
	else {
		packet_write_bits(buf, bitlen, 3, 2);
		packet_write_bits(buf, bitlen, (uint64_t)value, 32);
	}
}
void packet_write_s16(uint8_t *buf, int *bitlen, int16_t value) {
	if (!value) {
		packet_write_bits(buf, bitlen, 0, 2);
	}
	else if (value >= -0x10 && value < 0x10) {
		packet_write_bits(buf, bitlen, 1, 2);
		packet_write_bits(buf, bitlen, (uint64_t)(value+0x10), 5);
	}
	else if (value >= -0x80 && value < 0x80) {
		packet_write_bits(buf, bitlen, 2, 2);
		packet_write_bits(buf, bitlen, (uint64_t)(value+0x80), 8);
	}
	else {
		packet_write_bits(buf, bitlen, 3, 2);
		packet_write_bits(buf, bitlen, (uint64_t)((int64_t)value+0x8000), 16);
	}
}
void packet_write_s32(uint8_t *buf, int *bitlen, int32_t value) {
	if (!value) {
		packet_write_bits(buf, bitlen, 0, 2);
	}
	else if (value >= -0x20 && value < 0x20) {
		packet_write_bits(buf, bitlen, 1, 2);
		packet_write_bits(buf, bitlen, (uint64_t)(value+0x20), 6);
	}
	else if (value >= -0x400 && value < 0x400) {
		packet_write_bits(buf, bitlen, 2, 2);
		packet_write_bits(buf, bitlen, (uint64_t)(value+0x400), 11);
	}
	else {
		packet_write_bits(buf, bitlen, 3, 2);
		packet_write_bits(buf, bitlen, (uint64_t)((int64_t)value+0x80000000), 32);
	}
}
void packet_write_float(uint8_t *buf, int *bitlen, float value) {
	float a = fabsf(value);
	uint16_t fixed = 0;

	if (a < 0.001f) {
		packet_write_bits(buf, bitlen, 0, 2);
	}
	else if (a < 8.0f) {
		packet_write_bits(buf, bitlen, 1, 2);
		fixed |= value < 0;
		fixed |= ((uint16_t)a & 0x7) << 1;
		fixed |= (uint16_t)(fmodf(a, 1.0f) * 1024.0f) << 4;
		packet_write_bits(buf, bitlen, (uint64_t)fixed, 14);
	}
	else if (a < 127.0f) {
		packet_write_bits(buf, bitlen, 2, 2);
		fixed |= value < 0;
		fixed |= ((uint16_t)a & 0x7f) << 1;
		fixed |= (uint16_t)(fmodf(a, 1.0f) * 256.0f) << 8;
		packet_write_bits(buf, bitlen, (uint64_t)fixed, 16);
	}
	else {
		packet_write_bits(buf, bitlen, 3, 2);
		packet_write_bits(buf, bitlen, (uint64_t)htonf(value), 32);
	}
}

uint8_t packet_read_u8(const uint8_t *buf, int *bitlen) {
	uint64_t value;
	if (!packet_read_bit(buf, bitlen)) {
		return 0;
	}
	else if (!packet_read_bit(buf, bitlen)) {
		value = packet_read_bits(buf, bitlen, 4);
		return (uint8_t)value;
	}
	else {
		value = packet_read_bits(buf, bitlen, 8);
		return (uint8_t)value;
	}
}
uint16_t packet_read_u16(const uint8_t *buf, int *bitlen) {
	uint64_t value = packet_read_bits(buf, bitlen, 2);

	if (!value) {
		return 0;
	}
	else if (value == 1) {
		value = packet_read_bits(buf, bitlen, 5);
		return (uint16_t)value;
	}
	else if (value == 2) {
		value = packet_read_bits(buf, bitlen, 8);
		return (uint16_t)value;
	}
	else {
		value = packet_read_bits(buf, bitlen, 16);
		return (uint16_t)value;
	}
}
uint32_t packet_read_u32(const uint8_t *buf, int *bitlen) {
	uint64_t value = packet_read_bits(buf, bitlen, 2);

	if (!value) {
		return 0;
	}
	else if (value == 1) {
		value = packet_read_bits(buf, bitlen, 6);
		return (uint32_t)value;
	}
	else if (value == 2) {
		value = packet_read_bits(buf, bitlen, 11);
		return (uint32_t)value;
	}
	else {
		value = packet_read_bits(buf, bitlen, 32);
		return (uint32_t)value;
	}
}
int16_t packet_read_s16(const uint8_t *buf, int *bitlen) {
	uint64_t value = packet_read_bits(buf, bitlen, 2);

	if (!value) {
		return 0;
	}
	else if (value == 1) {
		return (int16_t)packet_read_bits(buf, bitlen, 5)-0x10;
	}
	else if (value == 2) {
		return (int16_t)packet_read_bits(buf, bitlen, 8)-0x80;
	}
	else {
		return (int16_t)((int32_t)packet_read_bits(buf, bitlen, 16)-0x8000);
	}
}
int32_t packet_read_s32(const uint8_t *buf, int *bitlen) {
	uint64_t value = packet_read_bits(buf, bitlen, 2);

	if (!value) {
		return 0;
	}
	else if (value == 1) {
		return (int32_t)packet_read_bits(buf, bitlen, 6)-0x20;
	}
	else if (value == 2) {
		return (int32_t)packet_read_bits(buf, bitlen, 11)-0x400;
	}
	else {
		return (int32_t)((int64_t)packet_read_bits(buf, bitlen, 32)-0x80000000);
	}
}
float packet_read_float(const uint8_t *buf, int *bitlen) {
	uint64_t type;
	float a, sign;

	type = packet_read_bits(buf, bitlen, 2);
	if (type == 0) {
		sign = 1.0f;
		a = 0.0f;
	}
	else if (type == 1) {
		sign = packet_read_bit(buf, bitlen) ? -1.0f : 1.0f;
		a = (float)packet_read_bits(buf, bitlen, 3);
		a += (float)packet_read_bits(buf, bitlen, 10) / 1024.0f;
	}
	else if (type == 2) {
		sign = packet_read_bit(buf, bitlen) ? -1.0f : 1.0f;
		a = (float)packet_read_bits(buf, bitlen, 7);
		a += (float)packet_read_bits(buf, bitlen, 8) / 256.0f;
	}
	else {
		return ntohf((unsigned int)packet_read_bits(buf, bitlen, 32));
	}

	return a * sign;
}
