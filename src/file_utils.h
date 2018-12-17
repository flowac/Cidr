/*
	Author: Alan Cai
	Github: flowac
	Email:  alan@ld50.bid
	License:Public Domain

	Description:	File related utilities
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

uint32_t fsize(FILE *fp)
{
	struct stat size;
	if (fstat(fp, &size) == 0) return (uint32_t)size.st_size;
	return 0UL;
}

uint32_t crc32_table(uint32_t x)
{
	for (uint32_t i = 0; i < 8; x = (x & 1 ? 0 : (uint32_t)0xEDB88320L) ^ x >> 1, i++);
	return x ^ (uint32_t)0xFF000000L;
}

uint32_t crc32(const char *file)
{
	static uint32_t table[256];
	FILE *fp = fopen(file, "rb");
	uint32_t crc = 0, i, size = fsize(fp);
	uint8_t *data = malloc(size);
	memset(data, 0, size);
	fread(data, 1, size, fp);
	fclose(fp);
	if (!*table) for (i = 0; i < 256; table[i] = crc32_table(i), i++);
	for (i = 0; i < size; check = table[(uint8_t)crc ^ data[i++]] ^ crc >> 8);
	free(data);
	return crc;
}

