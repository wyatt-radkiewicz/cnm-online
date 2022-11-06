#ifndef _packet_h_
#define _packet_h_
#include <stdint.h>

int packet_bytecount(int head);
void packet_write_bit(uint8_t *buf, int *head, int value);
int packet_read_bit(const uint8_t *buf, int *head);
void packet_write_bits(uint8_t *buf, int *head, uint64_t value, int numbits);
uint64_t packet_read_bits(const uint8_t *buf, int *head, int numbits);
void packet_write_u8(uint8_t *buf, int *head, uint8_t value);
void packet_write_u16(uint8_t *buf, int *head, uint16_t value);
void packet_write_u32(uint8_t *buf, int *head, uint32_t value);
void packet_write_s16(uint8_t *buf, int *head, int16_t value);
void packet_write_s32(uint8_t *buf, int *head, int32_t value);
void packet_write_float(uint8_t *buf, int *head, float value);
uint8_t packet_read_u8(const uint8_t *buf, int *head);
uint16_t packet_read_u16(const uint8_t *buf, int *head);
uint32_t packet_read_u32(const uint8_t *buf, int *head);
int16_t packet_read_s16(const uint8_t *buf, int *head);
int32_t packet_read_s32(const uint8_t *buf, int *head);
float packet_read_float(const uint8_t *buf, int *head);

#endif
