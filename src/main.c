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
#include "unzip.h"

int help()
{
	return printf(C_YEL "./NAME OPERATION ARGUMENTS\n"
		"b FILE1 FILE2           Compare 2 binary files\n"
		"e PATH1 [PATH2]         Extract files\n"
		"t FILE1 FILE2 [RESULTS] Compare 2 text files\n" C_STD);
}

int main(int argc, char **argv)
{
	FILE *out;
	char buf[BUF_256];

	if (argc < 2) return help();
	switch (argv[1][0]) {
	case 'b':
		if (argc != 4) return help();
		printf("XOR: 0x%X\n", diff_bin(argv[2], argv[3]));
		break;
	case 'e':
		if (argc != 3 && argc != 4) return help();
		if (argc == 4) strncpy(buf, argv[3], BUF_256 - 1);
		else snprintf(buf, BUF_256, "%.246s_unzipped", argv[2]);
		printf("%u files extracted to %s\n", Cvt_r(argv[2], buf), buf);
		break;
	case 't':
		if (argc != 4 && argc != 5) return help();
		if (argc == 4 || !(out = fopen(argv[4], "w"))) out = stdout;
		printf("Lines changed: %u\n", diff_txt(argv[2], argv[3], out));
		if (out != stdout) fclose(out);
		break;
	default:
		return help();
		break;
	}
	return 0;
}

