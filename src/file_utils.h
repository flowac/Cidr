/*
	Author: Alan Cai
	Github: flowac
	Email:  alan@ld50.bid
	License:Public Domain 2018-2019

	Description:	File related utilities
*/

#include <openssl/evp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "define.h"

const  uint32_t MAX_U24 = 0x00FFFFFF;
const  uint32_t MAX_U28 = 0x0FFFFFFF;
const  uint32_t MAX_U31 = 0x7FFFFFFF;
const  uint32_t NOT_U31 = 0x80000000;
const  uint32_t MAX_U32 = 0xFFFFFFFF;
const  uint64_t MAX_U56 = 0x00FFFFFFFFFFFFFFULL;
const  uint64_t MAX_U64 = 0xFFFFFFFFFFFFFFFFULL;
static uint32_t table[BUF_256];
extern uint32_t Cvt_r(const char *in_path, const char *out_path);

typedef struct u32_u32 {
	uint32_t  key; //Hash value of string
	uint32_t  len;
	uint32_t *data;//Line numbers
} u32_u32;

typedef struct u32_str {
	uint32_t  key; //Line number of strings
	uint32_t  len;
	char    **data;//Strings
} u32_str;

uint32_t fsize(FILE *fp)
{
	uint32_t size;
	if (!fp) return 0;
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	return size;
}

//Binary Heap Search
uint32_t u32_find(const u32_u32 *arr, const uint32_t len, const uint32_t val)
{
	uint32_t left = 0, mid = MAX_U32, right = len - 1;
	if (!arr || len == 0) goto RETURN;
	while (left <= right) {
		mid = (left + right) / 2;
		if (arr[mid].key < val) left = mid + 1;
		else if (arr[mid].key > val) {
			if (mid == 0) goto FAILED;
			right = mid - 1;
		} else {
			if (arr[mid].len > 0) goto RETURN;
			goto FAILED;
		}
	}
FAILED:	mid ^= MAX_U32;
RETURN:	return mid;
}

u32_u32 *u32_free(u32_u32 **ptr, uint32_t len)
{
	if (!*ptr) goto RETURN;
	for (uint32_t i = 0; i < len; i++) free((*ptr)[i].data);
	free(*ptr);
RETURN:	return 0;
}

u32_u32 *u32_insert(u32_u32 *arr, uint32_t *len, const uint32_t val, const uint32_t pos)
{
	uint32_t flag;
	u32_u32 *arr2 = arr;

	if (MAX_U28 < (flag = u32_find(arr2, *len, val))) {
		if (!(arr2 = realloc(arr2, sizeof(u32_u32) * (*len + 1)))) {
			arr2 = u32_free(&arr, *len);
			goto RETURN;
		}
		flag ^= MAX_U32;
		if (flag != *len) {
			if (arr2[flag].key < val) flag++;
			memmove(arr2 + flag + 1, arr2 + flag, sizeof(u32_u32) * (*len - flag));
		}
		arr2[flag].key  = val;
		arr2[flag].len  = 0;
		arr2[flag].data = 0;
		*len += 1;
	}
	if (!(arr2[flag].data = realloc(arr2[flag].data, sizeof(uint32_t) * (arr2[flag].len + 1)))) {
		arr2 = u32_free(&arr2, *len);
		goto RETURN;
	}
	arr2[flag].data[arr2[flag].len] = pos;
	arr2[flag].len++;
RETURN:	return arr2;
}

uint32_t u32_pop(u32_u32 *arr, uint32_t len, uint32_t val)
{
	uint32_t ret = MAX_U32, pos = u32_find(arr, len, val);
	if (pos >= MAX_U28) goto RETURN;
	ret = arr[pos].data[0];
	if (--arr[pos].len > 0) memmove(arr[pos].data, arr[pos].data + 1, sizeof(uint32_t) * arr[pos].len);
RETURN:	return ret;
}

uint32_t str_find(const u32_str *arr, const uint32_t len, const uint32_t val)
{
	uint32_t left = 0, mid = MAX_U32, right = len - 1;
	if (!arr || len == 0) goto RETURN;
	while (left <= right) {
		mid = (left + right) / 2;
		if (arr[mid].key < val) left = mid + 1;
		else if (arr[mid].key > val) {
			if (mid == 0) goto FAILED;
			right = mid - 1;
		} else  goto RETURN;
	}
FAILED:	mid ^= MAX_U32;
RETURN:	return mid;
}

u32_str *str_free(u32_str **ptr, uint32_t len)
{
	if (!*ptr) goto RETURN;
	for (uint32_t i = 0; i < len; i++) free((*ptr)[i].data);
	free(*ptr);
RETURN:	return 0;
}

u32_str *str_insert(u32_str *arr, uint32_t *len, const char *str, const uint32_t pos)
{
	char *str2 = malloc(strlen(str) + 1);
	u32_str *arr2 = arr;
	uint32_t flag;

	if (!str2) goto FAILED;
	if (MAX_U28 < (flag = str_find(arr2, *len, pos))) {
		if (!(arr2 = realloc(arr2, sizeof(u32_u32) * (*len + 1)))) goto FAILED;
		flag ^= MAX_U32;
		if (flag != *len) {
			if (arr2[flag].key < pos) flag++;
			memmove(arr2 + flag + 1, arr2 + flag, sizeof(u32_str) * (*len - flag));
		}
		arr2[flag].key  = pos;
		arr2[flag].len  = 0;
		arr2[flag].data = 0;
		*len += 1;
	}
	if (!(arr2[flag].data = realloc(arr2[flag].data, sizeof(char *) * (arr2[flag].len + 1)))) goto FAILED;
	strcpy(str2, str);
	arr2[flag].data[arr2[flag].len] = str2;
	arr2[flag].len++;
	goto RETURN;
FAILED:	arr2 = str_free(&arr, *len);
	if (str2) free(str2);
RETURN:	return arr2;
}

char *str_pop(u32_str *arr, uint32_t len, uint32_t val)
{
	uint32_t pos = str_find(arr, len, val);
	char *str = 0;
	if (MAX_U28 < pos)     goto RETURN;
	if (arr[pos].len == 0) goto RETURN;
	str = arr[pos].data[0];
	if (--arr[pos].len > 0) memmove(arr[pos].data, arr[pos].data + 1, sizeof(char *) * arr[pos].len);
RETURN:	return str;
}

char *is_zip(const char *name)
{
	char *tmp1 = strstr(name, ".Z");
	char *tmp2 = strstr(name, ".z");
	if (tmp1 < tmp2) tmp1 = tmp2;
	return tmp1 == name + strlen(name) - 2 ? tmp1 : 0;
}

uint8_t *sha512(const char *name)
{
	EVP_MD_CTX *md_ctx = 0;
	FILE *fp = fopen(name, "rb");
	uint32_t md_len, data_len = fsize(fp);
	uint8_t *md_val = 0, *data = malloc(data_len);

	fread(data, 1, data_len, fp);
	md_ctx = EVP_MD_CTX_new();
	if (!fp || !data || !md_ctx)             goto RETURN;
	if (!(md_val = malloc(EVP_MAX_MD_SIZE))) goto RETURN;
	memset(md_val, 0, EVP_MAX_MD_SIZE);
	EVP_DigestInit_ex(md_ctx, EVP_sha3_512(), NULL);
	EVP_DigestUpdate(md_ctx, data, data_len);
	EVP_DigestFinal_ex(md_ctx, md_val, &md_len);
RETURN:	if (fp)     fclose(fp);
	if (data)   free(data);
	if (md_ctx) EVP_MD_CTX_free(md_ctx);
	return md_val;
}

uint64_t shake128(const char *data, uint32_t len)
{
	EVP_MD_CTX *md_ctx = 0;
	uint64_t md_val2 = 0ULL;
	uint8_t md_val[8], i;

	if (!(md_ctx = EVP_MD_CTX_new())) goto RETURN;
	EVP_DigestInit_ex(md_ctx, EVP_shake128(), NULL);
	EVP_DigestUpdate(md_ctx, data, len);
	EVP_DigestFinalXOF(md_ctx, md_val, 8);
	for (i = 0; i < 8; i++) md_val2 |= (md_val[i] & 0xFFULL) << ((7 - i) * 8);
RETURN:	if (md_ctx) EVP_MD_CTX_free(md_ctx);
	return md_val2;
}

uint32_t crc32_table(uint32_t x)
{
	for (uint32_t i = 0; i < 8; i++) x = (x & 1 ? 0 : 0xEDB88320UL) ^ x >> 1;
	return x ^ 0xFF000000UL;
}

uint32_t crc32(char *data, uint32_t size)
{
	uint32_t crc = 0, i;
	if (!data || size == 0) goto RETURN;
	if (!*table) for (i = 0; i < BUF_256; i++) table[i] = crc32_table(i);
	for (i = 0; i < size; i++) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
RETURN: return crc;
}

void diff_txt(const char *left, const char *right, FILE *out)
{
	char buf[BUF_1K] = {0}, *tmp;
	FILE *left4 = 0, *right4 = 0;
	uint32_t  flag, i, j, left6 = 0;
	uint32_t *right5 = 0, right6 = 0, right8 = 0, right10 = 0;
	u32_u32  *right7 = 0;
	u32_str  *right9 = 0;

	if (!(left4  = fopen(left,  "r"))) goto RETURN;
	if (!(right4 = fopen(right, "r"))) goto RETURN;
	//Get checksum of each line
	for (; fgets((char *)buf, BUF_1K, right4); *buf = 0, right6++) {
		if (!(right5 = realloc(right5, sizeof(uint32_t) * (right6 + 1)))) goto RETURN;
		right5[right6] = MAX_U31;
		flag = crc32(buf, strlen(buf));
//		shake128(buf, strlen(buf));
		if (!(right7 = u32_insert(right7, &right8, flag, right6))) goto RETURN;
	}
	for (; fgets((char *)buf, BUF_1K, left4); *buf = 0, left6++) {
		if (left6 > right6) if (!(right5 = realloc(right5, sizeof(uint32_t) * (left6 + 1)))) goto RETURN;
		flag = crc32(buf, strlen(buf));
//		shake128(buf, strlen(buf));
		if (MAX_U32 != (flag = u32_pop(right7, right8, flag))) {
			right5[flag] &= NOT_U31;
			right5[flag] |= left6 + 1;
			right5[left6] &= MAX_U31;
		} else  right5[left6] |= NOT_U31;
	}
	fseek( left4, 0L, SEEK_SET);
	fseek(right4, 0L, SEEK_SET);
	for (i = j = flag = 0; fgets(buf, BUF_1K, right4) && i < right6; i++) {
		if ((right5[i] & MAX_U31) < MAX_U24) {
			j = right5[i] & MAX_U31;
			continue;
		}
		if (!(right9 = str_insert(right9, &right10, buf, j))) goto RETURN;
	}
	for (i = 0; fgets(buf, BUF_1K, left4) && i < left6; i++) {
		if ((right5[i] >> 31) & 1) fprintf(out, C_RED "--%5d %s" C_STD, i + 1, buf);
//		else           fprintf(out, "  %5d %s", i + 1, buf);
		while ((tmp = str_pop(right9, right10, i))) {
			fprintf(out, C_GRN "++%5d %s" C_STD, i + 1, tmp);
			free(tmp);
		}
	}
	printf("Line count: left = %d, right = %d\n", left6, right6);
RETURN:	if (left4)  fclose(left4);
	if (right4) fclose(right4);
	if (right5) free(right5);
	if (right7) u32_free(&right7, right8);
	if (right9) str_free(&right9, right10);
}

char diff_bin(const char *left, const char *right) {
	uint8_t *left_hash = sha512(left), *right_hash = sha512(right);
	char diff = -1;
	if (!left_hash || !right_hash) goto RETURN;
	diff = memcmp(left_hash, right_hash, EVP_MAX_MD_SIZE) == 0 ? 0 : 1;

RETURN:	if (left_hash)  free(left_hash);
	if (right_hash) free(right_hash);
	return diff;
}

