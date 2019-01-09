/*
	Author: Alan Cai
	Github: flowac
	Email:  alan@ld50.bid
	License:Public Domain 2018

	Description:	Lightweight recursive RedSea file unzip
*/

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "define.h"

#pragma pack(1)
extern uint32_t fsize(FILE *fp);
extern char    *is_zip(const char *name);

enum ARC_BITS
{
	CT_NONE		= 1,
	CT_7_BIT	= 2,
	CT_8_BIT	= 3,
	ARC_MAX_BITS	= 12
};

typedef struct CArcEntry
{
	struct CArcEntry *next;
	uint16_t basecode;
	uint8_t ch, pad;
} CArcEntry;

typedef struct CArcCtrl //control structure
{
	uint32_t src_pos, src_size, dst_pos, dst_size;
	uint8_t *src_buf, *dst_buf;
	uint32_t min_bits, min_table_entry;
	CArcEntry *cur_entry, *next_entry;
	uint32_t cur_bits_in_use, next_bits_in_use;
	uint8_t *stk_ptr, *stk_base;
	uint32_t free_idx, free_limit, saved_basecode, entry_used, last_ch;
	CArcEntry compress[1 << ARC_MAX_BITS], *hash[1 << ARC_MAX_BITS];
} CArcCtrl;

typedef struct CArcCompress
{
	uint32_t compressed_size, compressed_size_hi, expanded_size, expanded_size_hi;
	uint8_t  compression_type;
	uint8_t  body[1];
} CArcCompress;

int Bt(int bit_num, uint8_t *bit_field)
{
	bit_field += bit_num >> 3;
	bit_num &= 7;
	return (*bit_field & (1 << bit_num)) ? 1 : 0;
}

int Bts(int bit_num, uint8_t *bit_field)
{
	int res;
	bit_field += bit_num >> 3;
	bit_num &= 7;
	res = *bit_field & (1 << bit_num);
	*bit_field |= (1 << bit_num);
	return res ? 1 : 0;
}

uint32_t BFieldExtDWORD(uint8_t *src,uint32_t pos,uint32_t bits)
{
	uint32_t i, res=0;
	for (i = 0; i < bits; i++) if (Bt(pos + i,src)) Bts(i, (uint8_t *)&res);
	return res;
}

void ArcEntryGet(CArcCtrl *c)
{
	uint32_t i;
	CArcEntry *tmp, *tmp1;

	if (!c->entry_used) return;

	i = c->free_idx;
	c->entry_used = 0;
	c->cur_entry = c->next_entry;
	c->cur_bits_in_use = c->next_bits_in_use;
	if (c->next_bits_in_use < ARC_MAX_BITS) {
		c->next_entry = &c->compress[i++];
		if (i == c->free_limit) {
			c->next_bits_in_use++;
			c->free_limit = 1 << c->next_bits_in_use;
		}
	} else {
		do if (++i == c->free_limit) i = c->min_table_entry;
		while (c->hash[i]);
		tmp = &c->compress[i];
		c->next_entry = tmp;
		tmp1 = (CArcEntry *)&c->hash[tmp->basecode];
		while (tmp1 && tmp1->next != tmp)
			tmp1 = tmp1->next;
		if (tmp1)
			tmp1->next = tmp->next;
	}
	c->free_idx = i;
}

void ArcExpandBuf(CArcCtrl *c)
{
	uint8_t *dst_ptr, *dst_limit;
	uint32_t basecode, lastcode, code;
	CArcEntry *tmp, *tmp1;

	dst_ptr   = c->dst_buf + c->dst_pos;
	dst_limit = c->dst_buf + c->dst_size;

	while (dst_ptr < dst_limit && c->stk_ptr != c->stk_base)
		*dst_ptr++ = * -- c->stk_ptr;

	if (c->stk_ptr == c->stk_base && dst_ptr < dst_limit) {
		if (c->saved_basecode == 0xFFFFFFFFL) {
			lastcode = BFieldExtDWORD(c->src_buf, c->src_pos, c->next_bits_in_use);
			c->src_pos = c->src_pos + c->next_bits_in_use;
			*dst_ptr++ = lastcode;
			ArcEntryGet(c);
			c->last_ch = lastcode;
		} else
			lastcode = c->saved_basecode;
		while (dst_ptr < dst_limit && c->src_pos + c->next_bits_in_use <= c->src_size) {
			basecode = BFieldExtDWORD(c->src_buf, c->src_pos, c->next_bits_in_use);
			c->src_pos = c->src_pos + c->next_bits_in_use;
			if (c->cur_entry == &c->compress[basecode]) {
				*c->stk_ptr++ = c->last_ch;
				code = lastcode;
			} else
				code = basecode;
			while (code >= c->min_table_entry) {
				*c->stk_ptr++ = c->compress[code].ch;
				code = c->compress[code].basecode;
			}
			*c->stk_ptr++ = code;
			c->last_ch = code;

			c->entry_used = 1;
			tmp = c->cur_entry;
			tmp->basecode = lastcode;
			tmp->ch = c->last_ch;
			tmp1 = (CArcEntry *)&c->hash[lastcode];
			tmp->next = tmp1->next;
			tmp1->next = tmp;

			ArcEntryGet(c);
			while (dst_ptr < dst_limit && c->stk_ptr != c->stk_base)
				*dst_ptr++ = * -- c->stk_ptr;
			lastcode = basecode;
		}
		c->saved_basecode = lastcode;
	}
	c->dst_pos = dst_ptr-c->dst_buf;
}

CArcCtrl *ArcCtrlNew(uint32_t expand,uint32_t compression_type)
{
	CArcCtrl *c;
	c = malloc(sizeof(CArcCtrl));
	memset(c, 0, sizeof(CArcCtrl));
	if (expand) {
		c->stk_base = malloc(1 << ARC_MAX_BITS);
		c->stk_ptr  = c->stk_base;
	}
	if (compression_type == CT_7_BIT)
		c->min_bits = 7;
	else
		c->min_bits = 8;
	c->min_table_entry = 1 << c->min_bits;
	c->free_idx = c->min_table_entry;
	c->next_bits_in_use = c->min_bits+1;
	c->free_limit = 1 << c->next_bits_in_use;
	c->saved_basecode = 0xFFFFFFFFl;
	c->entry_used = 1;
	ArcEntryGet(c);
	c->entry_used = 1;
	return c;
}

uint8_t *ExpandBuf(CArcCompress *arc)
{
	if (arc->expanded_size >= 0x20000000l) return NULL;
	CArcCtrl *c;
	uint8_t *res = malloc(arc->expanded_size + 1);

	res[arc->expanded_size] = 0; //terminate
	switch (arc->compression_type) {
	case CT_NONE:
		memcpy(res, arc->body, arc->expanded_size);
		break;
	case CT_7_BIT:
	case CT_8_BIT:
		c = ArcCtrlNew(1, arc->compression_type);
		c->src_size = arc->compressed_size * 8;
		c->src_pos  = (sizeof(CArcCompress) - 1) * 8;
		c->src_buf  = (uint8_t *)arc;
		c->dst_size = arc->expanded_size;
		c->dst_buf  = res;
		c->dst_pos  = 0;
		ArcExpandBuf(c);
		free(c->stk_base);
		free(c);
		break;
	default:
		free(res);
		res = NULL;
		break;
	}
	return res;
}

uint8_t Cvt(const char *in_name, const char *out_name)
{
	uint32_t out_size, in_size;
	CArcCompress *arc;
	uint8_t *out_buf, ret = 0;
	FILE *io_file;

	if (!(io_file = fopen(in_name,"rb"))) return ret;
	in_size = fsize(io_file);
	arc = malloc(in_size);
	fread(arc, 1, in_size, io_file);
	out_size = arc->expanded_size;
//	printf("%-45s %d-->%d\r\n", in_name, (uint32_t)in_size, out_size);
	fclose(io_file);
	if (arc->compressed_size == in_size && (out_buf = ExpandBuf(arc))) {
/*		uint32_t i = 0, j = 0;
		for (; i < out_size; i++)
			if (out_buf[i] == 31)
				out_buf[j++] = 32;
			else if (out_buf[i] != 5)
				out_buf[j++] = out_buf[i];
		out_size = j;
*/		if (io_file = fopen(out_name, "wb")) {
			fwrite(out_buf, 1, out_size, io_file);
			fclose(io_file);
			ret = 1;
		}else printf("Can't open [%s]", out_name);
		free(out_buf);
	}else puts("arc failed");
	free(arc);
	return ret;
}

//TODO: Single file unzip might be broken
//Recursively executes the Cvt command (out_path must exist)
uint32_t Cvt_r(const char *in_path, const char *out_path)
{
	char in1[BUF_256], out[BUF_256], *tmp;
	DIR *in2;
	struct dirent *in3;
	uint32_t ret = 0;

	if (is_zip(in_path)) return Cvt(in_path, out_path);
	for (in2 = opendir(in_path); in2 && (in3 = readdir(in2));) {
		if (strcmp(in3->d_name,  ".") == 0
		||  strcmp(in3->d_name, "..") == 0) continue;
		snprintf(in1, BUF_256, "%.200s/%.54s",  in_path, in3->d_name);
		snprintf(out, BUF_256, "%.200s/%.54s", out_path, in3->d_name);
		if (tmp = is_zip(out)) *tmp = 0;
		else mkdir(out, 0755);
		ret += Cvt_r(in1, out);
	}
	if (in2) closedir(in2);
	return ret;
}

