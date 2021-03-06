/*
	Author: Alan Cai
	Github: flowac
	Email:  alan@ld50.bid
	License:Public Domain

	Description:	Test file to make sure headers compile
*/

#include "define.h"
#include "file_utils.h"
#include "net_utils.h"

int help()
{
	return printf(C_YEL "./NAME OPERATION ARGUMENTS\n"
		"b FILE1 FILE2           Compare 2 binary files\n"
		"t FILE1 FILE2 [RESULTS] Compare 2 text files\n"
		"h                       Start network host\n"
		"c                       Start network client\n"
		"s FILE                  Test SHA3-512 hashing\n" C_STD);
}

int main(int argc, char **argv)
{
	FILE    *out;
	int      i;
	uint8_t *u8_ptr;

	if (argc < 2) return help();
	switch (argv[1][0]) {
	case 'b':
		if (argc != 4) return help();
		i = diff_bin(argv[2], argv[3]);
		if (i  < 0) puts("Unable to compare.");
		if (i == 0) puts("The files are the same.");
		if (i == 1) puts("The files are different.");
		break;
	case 't':
		if (argc != 4 && argc != 5) return help();
		if (argc == 4 || !(out = fopen(argv[4], "w"))) out = stdout;
		diff_txt(argv[2], argv[3], out);
		if (out != stdout) fclose(out);
		break;
	case 'h':
		init_sock_server();
		break;
	case 'c':
		init_sock_client();
		break;
	case 's':
		if (argc != 3) return help();
		if ((u8_ptr = sha512(argv[2]))) {
			puts("SHA3-512 hash:");
			for (i = 0; i < EVP_MAX_MD_SIZE; i++)
				printf("%02X", u8_ptr[i]);
			puts("");
			free(u8_ptr);
		}
		break;
	default:
		return help();
		break;
	}
	return 0;
}

