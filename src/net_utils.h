/*
	Author: Alan Cai
	Github: flowac
	Email:  alan@ld50.bid
	License:Public Domain

	Description:	Network related utilities
*/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include "define.h"

enum NET_PORTS
{
	VCS_COM1 = 7890,
	VCS_COM2 = 8333,
	VCS_COM3 = 9777
};

int init_sock_server()
{
	char buf[BUF_1K];
	char *msg1 = "Host Testing";
	struct sockaddr_in ad;
	int sock = 0, sock2 = 0, ret;
	unsigned ad_len = sizeof(ad);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) goto RETURN;
	ad.sin_family = AF_INET;
	ad.sin_addr.s_addr = INADDR_ANY;
	ad.sin_port = htons(VCS_COM1);
	if (bind(sock, (struct sockaddr *)&ad, ad_len) < 0) goto RETURN;
	if (listen(sock, 20) < 0) goto RETURN;
	puts("Host start listening");
	if ((sock2 = accept(sock, (struct sockaddr *)&ad, &ad_len)) < 0) goto RETURN;

	ret = send(sock2, msg1, strlen(msg1), 0);
	printf("Host sent %d bytes: [%s]\n", ret, msg1);
	ret = recv(sock2, buf, BUF_1K, 0);
	printf("Host recv %d bytes: [%s]\n", ret, buf);
RETURN:	close(sock2);
	return 0;
}

int init_sock_client()
{
	int sock = 0, ret;
	char buf[BUF_1K];
	char *msg1 = "Client Testing";
	struct sockaddr_in ad;

	memset(&ad, 0, sizeof(ad));
	ad.sin_family = AF_INET;
	ad.sin_port = htons(VCS_COM1);

	if (inet_pton(AF_INET, "127.0.0.1", &ad.sin_addr) < 1) {
		puts("Can't convert IP");
		goto RETURN;
	}
	if (connect(sock, (struct sockaddr *)&ad, sizeof(ad)) < 0) {
		puts("Can't open socket");
		goto RETURN;
	}
	ret = send(sock, msg1, strlen(msg1), 0);
	printf("\tClient sent %d bytes: [%s]\n", ret, msg1);
	ret = recv(sock, buf, BUF_1K, 0);
	printf("\tClient recv %d bytes: [%s]\n", ret, buf);
RETURN:	close(sock);
	return 0;
}

