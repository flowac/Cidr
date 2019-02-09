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

const  uint32_t MAX_U28 = 0x0FFFFFFF;
const  uint32_t MAX_U31 = 0x7FFFFFFF;
const  uint32_t NOT_U31 = 0x80000000;
const  uint32_t MAX_U32 = 0xFFFFFFFF;

//Psuedo hash map access time: worse case O(log(n)), best case O(1)
typedef struct u32_hash {
	uint64_t  key;//TODO: maybe change to 32 bits
	uint32_t  len;
	uint32_t *data;
} u32_hash;

uint32_t fsize(FILE *fp)
{
	uint32_t size = 0;
	if (!fp) goto RETURN;
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
RETURN:	return size;
}

//Binary Heap Search
uint32_t u32_find(const u32_hash *arr, const uint32_t len, const uint64_t val)
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

void u32_free(u32_hash **ptr, uint32_t len)
{
	uint32_t i;
	if (!*ptr) return;
	for (i = 0; i < len; i++) free((*ptr)[i].data);
	free(*ptr);
	*ptr = 0;
}

u32_hash *u32_insert(u32_hash *arr, uint32_t *len, const uint64_t val, const uint32_t pos)
{
	uint32_t flag;
	u32_hash *arr2 = arr;

	if (MAX_U28 < (flag = u32_find(arr2, *len, val))) {
		if (!(arr2 = realloc(arr2, sizeof(u32_hash) * (*len + 1)))) {
			u32_free(&arr, *len);
			goto RETURN;
		}
		flag ^= MAX_U32;
		if (flag != *len) {
			if (arr2[flag].key < val) flag++;
			memmove(arr2 + flag + 1, arr2 + flag, sizeof(u32_hash) * (*len - flag));
		}
		arr2[flag].key  = val;
		arr2[flag].len  = 0;
		arr2[flag].data = 0;
		*len += 1;
	}
	if (!(arr2[flag].data = realloc(arr2[flag].data, sizeof(uint32_t) * (arr2[flag].len + 1)))) {
		u32_free(&arr2, *len);
		goto RETURN;
	}
	arr2[flag].data[arr2[flag].len] = pos;
	arr2[flag].len++;
RETURN:	return arr2;
}

//Pop: out-of-order removal O(log(n))
uint32_t u32_pop(u32_hash *arr, uint32_t len, uint64_t val)
{
	uint32_t ret = MAX_U32, pos = u32_find(arr, len, val);
	if (pos >= MAX_U28) goto RETURN;
	ret = arr[pos].data[0];
	if (--arr[pos].len > 0) memmove(arr[pos].data, arr[pos].data + 1, sizeof(uint32_t) * arr[pos].len);
RETURN:	return ret;
}

//Dequeue: in-order removal O(1)
uint32_t u32_dq(u32_hash *arr, uint32_t len, uint32_t *pos, uint64_t val)
{
	uint32_t ret = MAX_U32;
	if (arr[*pos].key != val || arr[*pos].len == 0) {
		if (*pos + 1 >= len || arr[*pos+1].key != val) goto RETURN;
		else *pos += 1;
	}
	ret = arr[*pos].data[0];
	if (--arr[*pos].len > 0) memmove(arr[*pos].data, arr[*pos].data + 1, sizeof(uint32_t) * arr[*pos].len);
RETURN:	return ret;
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

//TODO: Calculate collision rate for data size of 200
uint64_t hash64(const char *data, uint32_t len)
{
	static uint64_t table[BUF_256] = {0};
	uint32_t i, j, k;
	uint64_t l, sum = 0ULL, buf;
	if (!table[0]) for (i = 0; i < BUF_256; table[i++] = l ^ 0xBEE50000C0FEFE00ULL)
		for (k = 0, l = (uint64_t)i; k < 11; k++)
			l = (l & 1 ? 0ULL : 0x42F0E1EBA9EA3693ULL) ^ l >> 1;

	for (i = j = 0, k = (len + 7) / 8; i < k; j = ++i % BUF_256) {
		memcpy(&buf, &(data[i*8]), 8);
		sum = (sum << 5) ^ (buf ^ table[j]);
	}
	return sum;
}

void diff_txt(const char *left_file, const char *right_file, FILE *out)
{
	char buf[BUF_1K] = {0};
	FILE *left = 0, *right = 0;
	//The position where lines needed to be inserted on the left side is recorded in *add
	u32_hash *add = 0, *chksum = 0;
	//Line number offset for the right side is stored in *offset
	int32_t *offset = 0;
	//Lookup table stores whether line on left side is removed at most significant bit
	//Then it stores the line number where right side is added (WRT left) at lower 31 bits
	uint32_t flag, i, j, left_len = 0, *lookup = 0;
	uint32_t chksum_len = 0, right_len = 0, add_len = 0, inserts = 0, removes = 0;
	uint64_t sum;

	if (!(left  = fopen(left_file,  "r"))) goto RETURN;
	if (!(right = fopen(right_file, "r"))) goto RETURN;
	//Get checksum of each line
	for (memset(buf, 0, BUF_1K); fgets(buf, BUF_1K, right); memset(buf, 0, BUF_1K), right_len++) {
		if (!(lookup = realloc(lookup, sizeof(uint32_t) * (right_len + 1)))) goto RETURN;
		if (!(offset = realloc(offset, sizeof(uint32_t) * (right_len + 2)))) goto RETURN;
		lookup[right_len] = MAX_U31;
		offset[right_len+1] = ftell(right);
		sum = hash64(buf, strlen(buf));
		if (!(chksum = u32_insert(chksum, &chksum_len, sum, right_len))) goto RETURN;
	}
	offset[0] = 0;
	for (; fgets(buf, BUF_1K, left); memset(buf, 0, BUF_1K), left_len++) {
		if (left_len > right_len) if (!(lookup = realloc(lookup, sizeof(uint32_t) * (left_len + 1)))) goto RETURN;
		sum = hash64(buf, strlen(buf));
		if (MAX_U32 != (flag = u32_pop(chksum, chksum_len, sum))) {
			lookup[flag]     &= NOT_U31;
			lookup[flag]     |= left_len + 1;
			lookup[left_len] &= MAX_U31;
		} else  lookup[left_len] |= NOT_U31;
	}
	fseek(left, 0L, SEEK_SET);
	//Process and print output
	for (i = flag = 0; i < right_len; i++) {
		if ((lookup[i] & MAX_U31) < MAX_U31) {
			flag = lookup[i] & MAX_U31;
			continue;
		}
		if (!(add = u32_insert(add, &add_len, (uint64_t)flag, i))) goto RETURN;
	}
	for (i = j = 0; fgets(buf, BUF_1K, left) && i < left_len; i++) {
		if ((lookup[i] >> 31) & 1) {
			fprintf(out, C_RED "--%5d %s" C_STD, i + 1, buf);
			removes++;
		} //else fprintf(out, "  %5d %s", i + 1, buf);
		while (MAX_U32 != (flag = u32_dq(add, add_len, &j, (uint64_t)i))) {
			fseek(right, offset[flag], SEEK_SET);
			fgets(buf, BUF_1K, right);
			fprintf(out, C_GRN "++%5d %s" C_STD, i + 1, buf);
			inserts++;
		}
	}
	printf("Left     %7u     Right   %7u\n", left_len, right_len);
	printf("Removes  %7u     Inserts %7u\n", removes, inserts);
RETURN:	if (left)   fclose(left);
	if (right)  fclose(right);
	if (lookup) free(lookup);
	if (offset) free(offset);
	if (add)    u32_free(&add, add_len);
	if (chksum) u32_free(&chksum, chksum_len);
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

