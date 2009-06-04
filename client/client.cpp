/*
 * client.cpp
 *
 *  Created on: Jun 4, 2009
 *      Author: Q
 */

#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include "constants.h"
#include <bits/stringfwd.h>
#include <assert.h>
#include <pthread.h>

using namespace std;

char SERVER_IP[20];

int recvlen = 0;
int sendlen = 0;
char buf[BUFSIZE + 1];
int sockfd;
struct sockaddr_in saddr;

void mysend()//send buffer
{
	buf[sendlen] = 0;
	printf(buf);
	int len = write(sockfd, buf, sendlen);
	if (sendlen != len) {
		printf("send command error!\n");
	}
	assert(sendlen == len);
}

void myrecv() {
	do {
		printf("debug: receiving...\n");
		recvlen = read(sockfd, buf, BUFSIZE);
	} while (recvlen == 0);
	buf[recvlen] = 0;
	printf(buf);
}

void sendCmd(char *s) {
	strcpy(buf, s);
	sendlen = strlen(s);
	mysend();
}

int getCmdNum() {
	int ret = 0;
	assert(sscanf(buf, "%d", &ret) == 1);
	return ret;
}

int d_sockfd;
struct sockaddr_in d_saddr;

int d_init() {
	printf("debug: begin d_init in client.\n");

	sendCmd("PASV\r\n");
	myrecv();
	int portH, portL;
	int IP[4];
	if (sscanf(buf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &IP[3],
			&IP[2], &IP[1], &IP[0], &portH, &portL) != 6) {
		printf("PASV Error\n");
		return -1;
	} else {
		int dataPort = (portH << 8) | portL;
		printf("//Server data port: %d\n", dataPort);
		bzero(&saddr, sizeof(struct sockaddr_in));
		d_saddr.sin_family = AF_INET;
		d_saddr.sin_addr.s_addr = inet_addr(SERVER_IP);
		d_saddr.sin_port = htons(dataPort);
		d_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (connect(d_sockfd, (struct sockaddr*) &d_saddr,
				sizeof(struct sockaddr_in)) < 0) {
			printf("Data connect error.\n");
			close(d_sockfd);
			return -1;
		} else {
			printf("Dataport connected.\n");
		}
	}

	return 0;
}

char d_buf[DBUFSIZE + 1];

void d_mysend(FILE* d_f) {
	while (1) {
		int readlen = fread(d_buf, 1, DBUFSIZE, d_f);
		if (readlen == 0)
			break;
		int sendlen = write(d_sockfd, d_buf, readlen);
		if (sendlen != readlen) {
			printf("send data error!\n");
		}
		assert(sendlen == readlen);
	}
	printf("debug: d_sockfd %d\n", d_sockfd);
	printf("debug: sockfd %d\n", sockfd);
	close(d_sockfd);
}

void d_myrecv(FILE *d_f) {
	while (1) {
		recvlen = read(d_sockfd, d_buf, DBUFSIZE);

		if (d_f)
			printf("debug: recv %d bytes\n", recvlen);

		if (recvlen == 0)
			break;

		d_buf[recvlen] = 0;
		if (d_f) {
			fwrite(d_buf, 1, recvlen, d_f);
		} else
			printf(d_buf);
	}
	close(d_sockfd);
}

int log_in_user_pass(char* user, char* pass) {
	char cmd[100];
	sprintf(cmd, "USER %s\r\n", user);
	sendCmd(cmd);
	myrecv();
	if (getCmdNum() != 331) {
		printf("user is not allowed, quit.\n");
		close(sockfd);
		return -1;
	} else {
		printf("User OK\n");
	}

	sprintf(cmd, "PASS %s\r\n", pass);
	sendCmd(cmd);
	myrecv();
	if (getCmdNum() != 230) {
		printf("Wrong password, quit.\n");
		close(sockfd);
		return -1;
	} else {
		printf("Password OK\n");
	}
	return 0;
}

int log_in_anonymous() {
	sendCmd("USER anonymous\r\n");
	myrecv();
	if (getCmdNum() != 230) {
		printf("Anonymous user is not allowed, quit.\n");
		close(sockfd);
		return -1;
	}
	return 0;
}

int main(int argc, char** argv) {
	if (argc != 1 && argc != 3 && argc != 5) {
		printf("./client [IP port [user pass]]\n");
	}
	int port;
	if (argc == 1) {
		strcpy(SERVER_IP, "127.0.0.1");
		port = 2437;
	} else {
		strcpy(SERVER_IP, argv[1]);
		assert(sscanf(argv[2], "%d", &port) == 1);
	}

	bzero(&saddr, sizeof(struct sockaddr_in));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	saddr.sin_port = htons(port);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(sockfd, (struct sockaddr*) &saddr, sizeof(struct sockaddr_in))
			< 0) {
		printf("Connect error.\n");
		close(sockfd);
		return -1;
	}
	myrecv();
	if (getCmdNum() != 220) {
		printf("Not success, quit.\n");
		close(sockfd);
		return -1;
	}

	if (argc == 5) {
		if (log_in_user_pass(argv[3], argv[4]) < 0)
			return -1;
	} else {
		if (log_in_anonymous() < 0)
			return -1;
	}

	char cmd[200];
	while (1) {
		printf("ftp>");
		scanf("%s", cmd);
		if (strcmp(cmd, "?") == 0) {
			printf("get filename\tget remote file.\n");
			printf("put filename\tsend file.\n");
			printf("pwd\t\tshow remote current directory.\n");
			printf("dir\t\tshow remote files in current directory.\n");
			printf("cd dirname\tchange remote directory.\n");
			printf("quit\t\tquit ftp.\n");
			printf("?\t\tshow this help.\n");
		} else if (strcmp(cmd, "quit") == 0) {
			sendCmd("QUIT\r\n");
			myrecv();
			if (getCmdNum() != 221) {
				printf("Goodbye response error.\n");
			}
			break;
		} else if (strcmp(cmd, "get") == 0) {
			char filename[200];
			scanf("%s", filename);
			printf("debug: filename %s\n", filename);

			sendCmd("TYPE I\r\n");
			myrecv();
			if (getCmdNum() != 200) {
				printf("get error.\n");
			}

			/*
			 sprintf(cmd, "SIZE %s\r\n", filename);
			 sendCmd(cmd);
			 myrecv();
			 if(getCmdNum() != 213)
			 {
			 printf("get response error.\n");
			 }*/

			d_init();

			sprintf(cmd, "RETR %s\r\n", filename);
			sendCmd(cmd);
			myrecv();
			if (getCmdNum() != 150) {
				printf("get response error.\n");
			}

			FILE *f = fopen(filename, "wb");
			d_myrecv(f);

			myrecv();
			if (getCmdNum() != 226) {
				printf("get response error.\n");
			}

			fclose(f);

		} else if (strcmp(cmd, "put") == 0) {
			char filename[200];
			scanf("%s", filename);
			printf("debug: filename %s\n", filename);

			sendCmd("TYPE I\r\n");
			myrecv();
			if (getCmdNum() != 200) {
				printf("get error.\n");
			}

			d_init();

			sprintf(cmd, "STOR %s\r\n", filename);
			sendCmd(cmd);
			myrecv();

			if (getCmdNum() != 150) {
				printf("get response error.\n");
			}

			FILE *f = fopen(filename, "rb");
			d_mysend(f);

			printf("debug: send done.\n");

			myrecv();
			if (getCmdNum() != 226) {
				printf("get response error.\n");
			}

			fclose(f);
		} else if (strcmp(cmd, "pwd") == 0) {
			sendCmd("PWD\r\n");
			myrecv();
			if (getCmdNum() != 257) {
				printf("PWD response error.\n");
			}
		} else if (strcmp(cmd, "dir") == 0) {
			sendCmd("TYPE A\r\n");
			myrecv();
			if (getCmdNum() != 200) {
				printf("dir error.\n");
			}

			d_init();

			printf("debug: before send LIST\n");

			sendCmd("LIST\r\n");
			myrecv();
			if (getCmdNum() != 150) {
				printf("dir error.\n");
			}

			d_myrecv(0);

			myrecv();
			if (getCmdNum() != 226) {
				printf("dir error.\n");
			}

			else {
				printf("debug: transfer complete.\n");
			}
		} else if (strcmp(cmd, "cd") == 0) {
			char dirname[200];
			scanf("%s", dirname);
			sprintf(cmd, "CWD %s\r\n", dirname);
			sendCmd(cmd);
			myrecv();
			if (getCmdNum() != 250) {
				printf("CWD response error.\n");
			}
		} else {
			printf("Invalid command.\n");
		}
	}
	close(sockfd);
	return 0;
}
