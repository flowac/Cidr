/*
	Author: Alan Cai
	Github: flowac
	Email:  alan@ld50.bid
	License:Public Domain

	Description:	Test file to make sure headers compile
*/

#include "file_utils.h"
#include "unzip.h"

int main(int argc, char **argv)
{
	uint32_t left, right;
	if (argc != 3) {
		printf("Usage: %s FILE1 FILE2\n", argv[0]);
		return 0;
	}

	left  = crc32(argv[1]);
	right = crc32(argv[2]);
	printf("CRC32 checksum:\nLeft : 0x%X\nRight: 0x%X\nXOR  : 0x%X\n",
		left, right, diff_bin(argv[1], argv[2]));
	return 0;
}

