/*
 * ftp_method.cpp
 *
 *  Created on: Jun 4, 2009
 *      Author: Q
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <iostream>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "ftp_method.h"
#include "constants.h"

using namespace std;

extern int sockfd, numbytes;
extern char buf[BUFSIZE];
extern char cDir[DIRSIZE];
extern char cmd[CMDSIZE];
extern int addr_type;
extern char* addr;
extern char* username;
extern char* password;
extern int port;
extern char cmd_argu[ARGSIZE];

void debug_output(const string msg) {
	cout << DEBUG_MSG_HDR << msg << DEBUG_MSG_TAIL;
}

void err_handle(const int ERR_NO) {
	switch (ERR_NO) {
	case SOCKET_ERROR:
		cout << "Socket";
		break;
	case GETHOSTBYNAME_ERROR:
		cout << "Get host by name";
		break;
	case ADDRTYPE_ERROR:
		cout << "Address type";
		break;
	case CONNECT_ERROR:
		cout << "Connect host";
		break;
	case RECV_ERROR:
		cout << "Receive";
		break;
	case SENDCMD_ERROR:
		cout << "Send command";
		break;
	case RECVRESP_ERROR:
		cout << "Receive response";
		break;
	default:
		;
	}
	cout << " error" << endl;
	exit(ERR_NO);
}

int ftp_connect(const char *addr, const int addr_type, const int port) {
	struct hostent *remote_host;
	struct sockaddr_in remote_addr;

#ifdef DEBUG_OUTPUT
	cout << DEBUG_MSG_HDR << "ready to open socket. \t port = " << port
	<< DEBUG_MSG_TAIL;
#endif
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		err_handle(SOCKET_ERROR);
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(port);
#ifdef DEBUG_OUTPUT
	cout << DEBUG_MSG_HDR << "socket open.\t port = "
	<<ntohs(remote_addr.sin_port) << DEBUG_MSG_TAIL;
#endif

#ifdef DEBUG_OUTPUT
	cout << DEBUG_MSG_HDR << "ready to get addr.\t addr = " << addr
	<< "\t addr_type = " << addr_type << DEBUG_MSG_TAIL;
#endif

	if (addr_type == IP_ADDR) {
		inet_pton(AF_INET, addr, &remote_addr.sin_addr);
	} else if (addr_type == HOSTNAME_ADDR) {
		if ((remote_host = gethostbyname(addr)) == NULL)
			err_handle(GETHOSTBYNAME_ERROR);
		remote_addr.sin_addr = *((struct in_addr *) remote_host->h_addr);
	} else {
		err_handle(ADDRTYPE_ERROR);
	}

#ifdef DEBUG_OUTPUT
	cout << DEBUG_MSG_HDR << "addr got.\t addr = "
	<< remote_addr.sin_addr.s_addr << DEBUG_MSG_TAIL;
#endif

#ifdef DEBUG_OUTPUT
	cout << DEBUG_MSG_HDR << "ready to connect.\t sockfd = " << sockfd
	<< "\t addr = " << remote_addr.sin_addr.s_addr << DEBUG_MSG_TAIL;
#endif

#ifdef DEBUG_OUTPUT
	int connect_ret = 0;
#endif

	bzero(&(remote_addr.sin_zero), 8);
	if ((connect_ret = connect(sockfd, (struct sockaddr *) &remote_addr,
			sizeof(struct sockaddr))) == -1)
		err_handle(CONNECT_ERROR);

#ifdef DEBUG_OUTPUT
	cout << DEBUG_MSG_HDR << "connection established."
	//		<<"connect = "<<connect_ret
	<< DEBUG_MSG_TAIL;
#endif

	return 0;
}

int ftp_send_buf() {
	buf[strlen(buf)] = '\0';
#ifdef DEBUG_OUTPUT
	cout << DEBUG_MSG_HDR;
	printf(buf);
	cout << DEBUG_MSG_TAIL;
#endif
	if (send(sockfd, buf, strlen(buf), 0) != strlen(buf))
		err_handle(SENDCMD_ERROR);
}

int ftp_send_cmd(const char *command) {
	strcpy(buf, command);
	ftp_send_buf();
}

int ftp_recv_response() {
	do {
#ifdef DEBUG_OUTPUT
		cout << DEBUG_MSG_HDR << "receiving data..." << DEBUG_MSG_TAIL;
#endif
		numbytes = recv(sockfd, buf, BUFSIZE, 0);
	} while (numbytes == 0);
	buf[numbytes] = '\0';
	printf("Recv: %s", buf);
	if (buf == NULL)
		err_handle(RECVRESP_ERROR);
	return true;
}

void reset(char *tar, const int tar_size) {
	memset(tar, 0, tar_size);
}

void reset_cmd() {
	memset(cmd, 0, CMDSIZE);
}

void reset_buf() {
	memset(buf, 0, BUFSIZE);
}

int ftp_login(const char *user, const char *pass) {
	ftp_recv_response();

	reset(cmd, CMDSIZE);
	sprintf(cmd, "USER %s\r\n", user);
	ftp_send_cmd(cmd);
	ftp_recv_response();

	reset(cmd, CMDSIZE);
	sprintf(cmd, "PASS %s\r\n", pass);
	ftp_send_cmd(cmd);
	ftp_recv_response();

	return 0;
}

int ftp_close() {
	ftp_send_cmd("QUIT\r\n");
	ftp_recv_response();
	close(sockfd);
}

int ftp_get(string filename) {

}

int ftp_put(string filename) {

}

int ftp_pwd(char *path) {
	int l = DIRSIZE;
	char *b = path;
	char *s;

	ftp_send_cmd("PWD\r\n");
	ftp_recv_response();

	s = strchr(buf, '"');
	if (s == NULL)
		return 0;
	s++;
	while ((--l) && (*s) && (*s != '"'))
		*b++ = *s++;
	*b++ = '\0';
	return 1;
}
int ftp_dir() {

}

void getFathDir(char *currDir, char *fathDir) {
	if (!strcmp(currDir, "/")) {
		strcpy(fathDir, "/");
	} else {
		int i = strlen(currDir);
		while (i > 1 && currDir[i] != '/')
			i--;
		currDir[i] = '\0';
		strcpy(fathDir, currDir);
	}
}

int ftp_cd(const char *dir) {
	reset(cmd, CMDSIZE);
	sprintf(cmd, "CWD %s\r\n", dir);
	ftp_send_cmd(cmd);
	ftp_pwd(cDir);
}

int ftp_cd() {
	reset(cmd_argu, ARGSIZE);
	scanf("%s", cmd_argu);
	ftp_cd(cmd_argu);
}
