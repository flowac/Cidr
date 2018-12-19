/*
	Author: Alan Cai
	Github: flowac
	Email:  alan@ld50.bid
	License:Public Domain 2018

	Description:	File related utilities
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

extern uint8_t Cvt(const char *in_name, const char *out_name);

uint32_t fsize(FILE *fp)
{
	uint32_t size;
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	return size;
}

char *is_zip(const char *name)
{
	char *tmp1 = strstr(name, ".Z");
	char *tmp2 = strstr(name, ".z");
	if (!tmp1 || (tmp2 && tmp1 < tmp2)) tmp1 = tmp2;
	return tmp1 == name + strlen(name) - 1 ? tmp1 : NULL;
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
	if (!*table) for (i = 0; i < 256; i++) table[i] = crc32_table(i);
	for (i = 0; i < size; i++) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
	free(data);
	return crc;
}

//Analyse text file differences, might leak memory if operation failed
int32_t diff_txt(const char *left, const char *right, const char *out)
{
	char  left2[256],  *left3,  left4[256] = {0};
	char right2[256], *right3, right4[256] = {0};
	FILE *out2, *left5, *right5;
	int32_t line = 0, diff = 0;
	strcpy(left2, left);
	strcpy(right2, right);

	if (left3 = is_zip(left2)) {
		*left3 = 0;
		if (!Cvt(left, left2))      return -1;
	}
	if (right3 = is_zip(right2)) {
		*right3 = 0;
		if (!Cvt(right, right2))    return -2;
	}
	if (!(out2   = fopen(out,    "w"))) return -3;
	if (!(left5  = fopen(left2,  "r"))) return -4;
	if (!(right5 = fopen(right2, "r"))) return -5;
	for (; fgets(left4, 256, left5) || fgets(right4, 256, right5)
	     ; *left4 = 0, *right4 = 0, line++) {
		if (strcmp(left4, right4) == 0) continue;
		if (*left4)  fprintf(out2, "--%5d %s\r\n", line, left4);
		if (*right4) fprintf(out2, "++%5d %s\r\n", line, right4);
		diff++;
	}

	fclose(out2);
	fclose(left5);
	fclose(right5);
	if (left3)  remove(left2);
	if (right3) remove(right2);
	return diff;
}

int32_t diff_bin(const char *left, const char *right) {return crc32(left) ^ crc32(right);}

