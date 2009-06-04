/*
 * server.cpp
 *
 *  Created on: Jun 3, 2009
 *      Author: Q
 */
#include <sys/socket.h>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <string>
#include <vector>
#include <netinet/in.h>
#include <netdb.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <assert.h>
#include "../client/constants.h"

using namespace std;

#define SERVER_PORT 1988
int IP[4] = { 58, 66, 138, 127 };

char buf[BUFSIZE + 1];
int req_len = 0;

int sockfd;
int clientSock;

int backup;

char work_dir[DIRSIZE] = ".";

void wait() {
	int intval = 5000000;
	while (intval--)
		;
}
void sendResponseAll(char* Res) {
	wait();
	write(clientSock, Res, strlen(Res));
	printf("Response: %s", Res);
}

void sendResponse(int Num) {
	char Res[RESPSIZE];
	memset(Res, 0, RESPSIZE);
	sprintf(Res, "%d\r\n", Num);
	sendResponseAll(Res);
}

void getRequest() {
	wait();
	req_len = read(clientSock, buf, BUFSIZE);
	buf[req_len] = 0;
	printf(buf);
}

int strbegins(const char* strShort, const char* strLong) {
	return strncmp(strShort, strLong, strlen(strShort)) == 0;
}

int d_clientSock;

int d_init() {
	printf("debug: begin d_init in server\n");

	struct sockaddr_in d_myaddr;
	bzero(&d_myaddr, sizeof(struct sockaddr_in));
	d_myaddr.sin_family = AF_INET;
	d_myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	d_myaddr.sin_port = htons(SERVER_PORT + 1000);
	int d_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	while (bind(d_sockfd, (struct sockaddr*) &d_myaddr,
			sizeof(struct sockaddr_in)) < 0) {
		printf("Bind data port error.\n");
		d_myaddr.sin_port = htons(ntohs(d_myaddr.sin_port) + 1);
	}
	if (listen(d_sockfd, 1) < 0) {
		printf("Listen error.\n");
		return -1;
	}

	char temp[RESPSIZE];
	sprintf(temp, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", IP[0],
			IP[1], IP[2], IP[3], ntohs(d_myaddr.sin_port) >> 8,
			ntohs(d_myaddr.sin_port) & 0xFF);

	printf(temp);
	printf("debug: port %d\n", ntohs(d_myaddr.sin_port));

	sendResponseAll(temp);

	printf("Listen\n");

	struct sockaddr_in d_clientaddr;
	socklen_t sockLength = 0;

	d_clientSock = accept(d_sockfd, (struct sockaddr*) &d_clientaddr,
			&sockLength);
	if (d_clientSock < 0) {
		printf("Accept error.\n");
		return -1;
	} else {
		printf("Data Connnection Accept.\n");
		return 0;
	}
}

char liststr[DBUFSIZE];
int liststrLen = 0;

char d_buf[DBUFSIZE];

//TODO
void mycd(char *dirname) {
	if (strcmp(dirname, ".") == 0) {
		return;
	} else if (strcmp(dirname, "..") == 0) {
		int t;
		for (t = strlen(work_dir) - 1; t >= 0; t--) {
			if (work_dir[t] == '/')
				break;
		}
		if (t <= 0)
			return;
		else {
			work_dir[t] = 0;
		}
	} else {
		int l = strlen(work_dir);
		strcat(work_dir, "/");
		strcat(work_dir, dirname);
		printf("debug: work_dir %s\n", work_dir);
		if (opendir(work_dir) == NULL) {
			work_dir[l] = 0;
		}
	}
}

void genListStr() {
	liststr[0] = 0;
	liststrLen = 0;
	strcat(liststr, "DIR\r\n=================\r\n");

	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;

	if ((dp = opendir(work_dir)) == NULL) {
		return;
	}

	while ((entry = readdir(dp)) != NULL) {
		strcat(liststr, entry->d_name);
		strcat(liststr, "\r\n");
	}
	closedir(dp);

	strcat(liststr, "=================\r\n");
	liststrLen = strlen(liststr);
}

void d_mysend(FILE* d_f) {
	if (d_f) {
		while (1) {
			int readlen = fread(d_buf, 1, DBUFSIZE, d_f);
			if (readlen == 0)
				break;
			int sendlen = write(d_clientSock, d_buf, readlen);
			assert(readlen == sendlen);
		}
		printf("debug: d_clientSock %d\n", d_clientSock);
		printf("debug: clientSock %d\n", clientSock);
		close(d_clientSock);
		fclose(d_f);
	} else {
		genListStr();
		int sendlen = write(d_clientSock, liststr, liststrLen);
		assert(liststrLen == sendlen);
		close(d_clientSock);
	}

}

void d_myreceive(FILE* d_f) {
	while (1) {
		int recvlen = read(d_clientSock, d_buf, DBUFSIZE);
		printf("debug: recv %d bytes\n", recvlen);
		if (recvlen == 0)
			break;
		d_buf[recvlen] = 0;
		fwrite(d_buf, 1, recvlen, d_f);
	}

	printf("debug: d_clientSock %d\n", d_clientSock);
	printf("debug: clientSock %d\n", clientSock);
	printf("debug: d_f %d\n", d_f);

	fclose(d_f);
	close(d_clientSock);
}

int main() {
	struct sockaddr_in myaddr;
	bzero(&myaddr, sizeof(struct sockaddr_in));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(SERVER_PORT);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(sockfd, (struct sockaddr*) &myaddr, sizeof(struct sockaddr_in))
			< 0) {
		printf("Bind error.\n");
		return -1;
	}

	printf("Bind\n");

	if (listen(sockfd, 1) < 0) {
		printf("Listen error.\n");
		return -1;
	}

	printf("Listen\n");

	struct sockaddr_in clientaddr;
	socklen_t sockLength = 0;

	clientSock = accept(sockfd, (struct sockaddr*) &clientaddr, &sockLength);
	if (clientSock < 0) {
		printf("Accept error.\n");
		return -1;
	}

	printf("Accept\n");
	sendResponse(220);

	char user[LOGINBUFSIZE];
	char password[LOGINBUFSIZE];

	while (1) {
		getRequest();
		if (strcmp("USER anonymous\r\n", buf) == 0) {
			strcpy(user, buf + 5);
			printf("UserName: %s\n", user);
			sendResponse(230);
			break;
		} else if (strbegins("USER ", buf)) {
			strcpy(user, buf + 5);
			sendResponse(331);
			while (1) {
				getRequest();
				if (strbegins("PASS ", buf)) {
					strcpy(password, buf + 5);
					printf("UserName: %s\n", user);
					printf("Password: %s\n", password);
					sendResponse(230);
					break;
				} else
					sendResponse(502);
			}
			break;
		} else
			sendResponse(502);
	}

	printf("Begin ftp service\n");

	char temp[CMDSIZE + ARGSIZE];
	while (1) {
		getRequest();
		if (strcmp("QUIT\r\n", buf) == 0) {
			sendResponse(221);
			break;
		} else if (strcmp("TYPE I\r\n", buf) == 0) {
			sendResponse(200);
		} else if (strcmp("TYPE A\r\n", buf) == 0) {
			printf("debug: deal TYPE A\n");
			sendResponse(200);
		} else if (strcmp("PWD\r\n", buf) == 0) {
			if (strcmp(work_dir, ".") == 0)
				sprintf(temp, "257 \"/\" is current directory.\r\n");
			else
				sprintf(temp, "257 \"%s\" is current directory.\r\n", work_dir
						+ 1);
			sendResponseAll(temp);
		} else if (strbegins("CWD ", buf)) {
			strcpy(temp, buf + 4);
			int t = strlen(temp) - 2;
			temp[t] = 0;
			printf("debug: dirname %s\n", temp);
			mycd(temp);
			sendResponseAll("250 CWD command successful.\r\n");
		} else if (strcmp("PASV\r\n", buf) == 0) {
			if (d_init() < 0) {
				printf("PASV fail!\n");
				break;
			}
		} else if (strcmp("LIST\r\n", buf) == 0) {
			sendResponse(150);
			d_mysend(0);
			sendResponse(226);
		} else if (strbegins("RETR ", buf)) {
			strcpy(temp, work_dir);
			strcat(temp, "/");
			strcat(temp, buf + 5);
			int t = strlen(temp) - 2;
			temp[t] = 0;
			printf("debug: filename %s\n", temp);

			FILE *f = fopen(temp, "rb");

			sendResponse(150);
			d_mysend(f);
			sendResponse(226);

		} else if (strbegins("STOR ", buf)) {
			strcpy(temp, work_dir);
			strcat(temp, "/");
			strcat(temp, buf + 5);
			int t = strlen(temp) - 2;
			temp[t] = 0;
			printf("debug: filename %s\n", temp);

			FILE *f = fopen(temp, "wb");

			sendResponse(150);
			backup = clientSock;
			d_myreceive(f);
			clientSock = backup;
			sendResponse(226);
		} else {
			printf("debug: Not support: \"%s\"\n", buf);
			sendResponse(502);
		}
	}

	close(clientSock);
	close(sockfd);
	return 0;
}
