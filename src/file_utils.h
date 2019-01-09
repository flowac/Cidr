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
#include "define.h"

static uint32_t table[BUF_256];
extern uint32_t Cvt_r(const char *in_path, const char *out_path);

uint32_t fsize(FILE *fp)
{
	uint32_t size;
	if (!fp) return 0;
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	return size;
}

char *is_zip(const char *name)
{
	char *tmp1 = strstr(name, ".Z");
	char *tmp2 = strstr(name, ".z");
	if (tmp1 < tmp2) tmp1 = tmp2;
	return tmp1 == name + strlen(name) - 2 ? tmp1 : 0;
}

uint32_t crc32_table(uint32_t x)
{
	for (uint32_t i = 0; i < 8; i++) x = (x & 1 ? 0 : 0xEDB88320UL) ^ x >> 1;
	return x ^ 0xFF000000UL;
}

uint32_t crc32_raw(uint8_t *data, uint32_t size)
{
	uint32_t crc = 0, i;
	if (!data || size == 0) goto RETURN;
	if (!*table) for (i = 0; i < BUF_256; i++) table[i] = crc32_table(i);
	for (i = 0; i < size; i++) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
RETURN: return crc;
}

uint32_t crc32(const char *file)
{
	//TODO: Use SHA512 or MD6
	FILE *fp = fopen(file, "rb");
	uint32_t crc = 0, i, size = fsize(fp);
	uint8_t *data = malloc(size);

	if (!fp || !data) goto RETURN;
	memset(data, 0, size);
	size = fread(data, 1, size, fp);
	if (!*table) for (i = 0; i < BUF_256; i++) table[i] = crc32_table(i);
	for (i = 0; i < size; i++) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
RETURN:	if (fp)   fclose(fp);
	if (data) free(data);
	return crc;
}

uint32_t diff_txt(const char *left, const char *right, FILE *out)
{
	//TODO: Replace this with diff_txt2 once tested
	char  left2[BUF_256],  *left3 = 0,  left4[BUF_256] = {0};
	char right2[BUF_256], *right3 = 0, right4[BUF_256] = {0};
	FILE *left5 = 0, *right5 = 0;
	uint32_t line = 1, diff = 0;
	strcpy(left2, left);
	strcpy(right2, right);

	if (left3 = is_zip(left2)) {
		*left3 = 0;
		if (!Cvt_r(left, left2))    goto RETURN;
	}
	if (right3 = is_zip(right2)) {
		*right3 = 0;
		if (!Cvt_r(right, right2))  goto RETURN;
	}
	if (!(left5  = fopen(left2,  "r"))) goto RETURN;
	if (!(right5 = fopen(right2, "r"))) goto RETURN;
	for (; fgets(left4, BUF_256, left5) - fgets(right4, BUF_256, right5)
	     ; *left4 = 0, *right4 = 0, line++) {
		if (strcmp(left4, right4) == 0) continue;
		if (*left4)  fprintf(out, C_RED "--%05d %s" C_STD, line, left4);
		if (*right4) fprintf(out, C_GRN "++%05d %s" C_STD, line, right4);
		diff++;
	}
RETURN:	if (left3)  remove(left2);
	if (right3) remove(right2);
	if (left5)  fclose(left5);
	if (right5) fclose(right5);
	return diff;
}

uint32_t diff_txt2(const char *left, const char *right, FILE *out)
{
	char  left2[BUF_256],  *left3 = 0;
	char right2[BUF_256], *right3 = 0;
	FILE *left4 = 0, *right4 = 0;
	uint8_t buf[BUF_1K] = {0};
	uint32_t diff = 0, len, len2;
	uint32_t *left5 = 0, *right5 = 0, left6, right6;
	strcpy(left2, left);
	strcpy(right2, right);

	if (left3 = is_zip(left2)) {
		*left3 = 0;
		if (!Cvt_r(left, left2))    goto RETURN;
	}
	if (right3 = is_zip(right2)) {
		*right3 = 0;
		if (!Cvt_r(right, right2))  goto RETURN;
	}
	if (!(left4  = fopen(left2,  "r"))) goto RETURN;
	if (!(right4 = fopen(right2, "r"))) goto RETURN;
	//Get checksum of each line
	for (len = left6 = 0; fgets((char *)buf, BUF_1K, left4); *buf = 0, left6++) {
		if (len == left6) {
			len += 100;
			left5 = realloc(left5, sizeof(uint32_t) * len);
		}
		left5[left6] = crc32_raw(buf, strlen((char *)buf));
	}
	for (len = right6 = 0; fgets((char *)buf, BUF_1K, right4); *buf = 0, right6++) {
		if (len == right6) {
			len += 100;
			right5 = realloc(right5, sizeof(uint32_t) * len);
		}
		right5[right6] = crc32_raw(buf, strlen((char *)buf));
	}
	if (sizeof(left5) < sizeof(right5))
		left5 = realloc(left5, sizeof(right5));
	else if (sizeof(left5) > sizeof(right5))
		right5 = realloc(right5, sizeof(left5));
	printf("size: %d, %d lines\n", left6, right6);
	//Find difference (worst case: 2n^2)
	for (len = 0; len < left6 || len < right6; len++) {
		if (left5[len] == right5[len]) {
			left5 [len] = 0;
			right5[len] = 0;
			diff++;
		} else for (len2 = len; len2 < left6 || len2 < right6; len2++) {
			if (left5[len] == right5[len2]) {
				left5 [len]  = 0;
				right5[len2] = 0;
				diff++;
				break;
			}
			if (left5[len2] == right5[len]) {
				left5 [len2] = 0;
				right5[len]  = 0;
				diff++;
				break;
			}
		}
	}
	fseek( left4, 0L, SEEK_SET);
	fseek(right4, 0L, SEEK_SET);
	for (*buf = len = 0; len < left6 || len < right6; *buf = 0, len++) {
		if (fgets((char *)buf, BUF_1K, left4) && left5[len])
			fprintf(out, C_RED "--%05d %s" C_STD, len, (char *)buf);
		else if (fgets((char *)buf, BUF_1K, right4) && right5[len])
			fprintf(out, C_GRN "++%05d %s" C_STD, len, (char *)buf);
		else if (*buf)
			fprintf(out, C_STD "  %05d %s" C_STD, len, (char *)buf);
	}
	for (len = 0; len < left6 || len < right6; *buf = 0, len++) {
		printf("%4d: %X %X\n", len, left5[len], right5[len]);
	}

RETURN:	if (left3)  remove(left2);
	if (right3) remove(right2);
	if (left4)  fclose(left4);
	if (right4) fclose(right4);
	if (left5)  free(left5);
	if (right5) free(right5);
	return diff;
}

uint32_t diff_bin(const char *left, const char *right) {return crc32(left) ^ crc32(right);}

