/*
	Author: Alan Cai
	Github: flowac
	Email:  alan@ld50.bid
	License:Public Domain

	Description:	Network related utilities
*/

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
	char *msg1 = "Connection opened\r\nTesting\r\n";
	struct sockaddr_in ad;
	int sock, sock2;
	unsigned ad_len = sizeof(struct sockaddr_in);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) puts("Failed to create socket\n");
	ad.sin_family = AF_INET;
	ad.sin_addr.s_addr = INADDR_ANY;
	ad.sin_port = htons(VCS_COM1);
	bind(sock, (struct sockaddr *)&ad, ad_len);
	listen(sock, 20);
	if ((sock2 = accept(sock, (struct sockaddr *)&ad, &ad_len)) < 0) puts("Error accepting connection");

	send(sock2, msg1, strlen(msg1), 0);
	recv(sock2, buf, BUF_1K, 0);
	close(sock2);
	return 0;
}

int init_sock_client()
{
	int sock;
	char buf[BUF_1K];
	char *msg1 = "Connection opened\r\nTesting\r\n";

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) puts("Failed to create socket\n");
	send(sock, msg1, strlen(msg1), 0);
	recv(sock, buf, BUF_1K, 0);
	close(sock);
	return 0;
}

