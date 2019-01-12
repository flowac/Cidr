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

char **insert_string(char **arr, uint32_t *len, char *str, uint32_t pos)
{
	uint32_t len2 = *len + 1, i;
	char **arr2 = 0, *str2 = 0;
	if (pos > *len || !(str2 = malloc(strlen(str) + 1))) goto RETURN;
	if (!(arr2 = realloc(arr, sizeof(char *) * len2)))   goto RETURN;
	for (i = *len; i > pos; i--) arr2[i] = arr2[i-1];
	strcpy(str2, str);
	arr2[pos] = str2;
	*len = len2;
RETURN: if (str2 && !arr2) free(str2);
	return arr2;
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

	fread(data, 1, BUF_1K, fp);
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

//TODO: code cleanup and rename to diff_txt
void diff_txt2(const char *left, const char *right, FILE *out)
{
	const uint32_t MAX_6B = 0x00FFFFFF;
	const uint32_t MAX_8B = 0xFFFFFFFF;
	char  left2[BUF_256],  *left3 = 0;
	char right2[BUF_256], *right3 = 0;
	char buf[BUF_1K] = {0}, buf2[BUF_1K] = {0}, **buf3 = 0;
	void *buf3_head = 0;
	FILE *left4 = 0, *right4 = 0;
	uint32_t buf3_len = 0, flag, len, len2;
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
	for (left6 = 0; fgets((char *)buf, BUF_1K, left4); *buf = 0, left6++)
		if (!(left5 = realloc(left5, sizeof(uint32_t) * (left6 + 1)))) goto RETURN;
		else left5[left6] = crc32(buf, strlen((char *)buf));
	for (right6 = 0; fgets((char *)buf, BUF_1K, right4); *buf = 0, right6++)
		if (!(right5 = realloc(right5, sizeof(uint32_t) * (right6 + 1)))) goto RETURN;
		else right5[right6] = crc32(buf, strlen((char *)buf));
	printf("Line count: left = %d, right = %d\n", left6, right6);
	//Find difference
	for (len = 0; len < left6; len++) {
		flag = left5[len];
		for (len2 = 0; len2 < right6; len2++)
			if (flag == right5[len2]) {
				flag = MAX_8B;
				right5[len2] = len + 1;
				break;
			}
		if (flag != MAX_8B) left5[len] = len;
	}
	fseek( left4, 0L, SEEK_SET);
	fseek(right4, 0L, SEEK_SET);
	for (*buf = len = 0; len < left6; *buf = 0, len++) {
		fgets((char *)buf, BUF_1K, left4);
		if (left5[len] < MAX_6B)
			snprintf(buf2, BUF_1K, C_RED "--%05d %.999s" C_STD, left5[len] + 1, buf);
		else
			snprintf(buf2, BUF_1K, C_STD "  %05d %.999s" C_STD, len + 1, buf);
		if (!(buf3 = insert_string(buf3, &buf3_len, buf2, buf3_len))) goto RETURN;
		else buf3_head = buf3;
	}
	for (*buf = flag = len = len2 = 0; len < right6; *buf = 0, len++) {
		fgets((char *)buf, BUF_1K, right4);
		if (right5[len] < MAX_6B) {
			flag = right5[len];
			continue;
		}
		snprintf(buf2, BUF_1K, C_GRN "++%05d %.999s" C_STD, flag + 1, buf);
		if (!(buf3 = insert_string(buf3, &buf3_len, buf2, flag + len2))) goto RETURN;
		else buf3_head = buf3;
		len2++;
	}
	for (len = 0; len < buf3_len; len++) fprintf(out, "%s", buf3[len]);
/*	for (len = 0; len < left6 || len < right6; *buf = 0, len++)
		printf("%4d: %8X %8X\n", len+1, len < left6 ? left5[len] : MAX_8B,
			len < right6 ? right5[len] : MAX_8B);
*/
RETURN:	if (left3)     remove(left2);
	if (right3)    remove(right2);
	if (left4)     fclose(left4);
	if (right4)    fclose(right4);
	if (left5)     free(left5);
	if (right5)    free(right5);
	if (buf3_head) {
		for (buf3 = buf3_head, len = 0; len < buf3_len; len++) free(buf3[len]);
		free(buf3_head);
	}
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

