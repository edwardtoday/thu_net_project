/*
 * client.cpp
 *
 *  Created on: May 28, 2009
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

#define DEBUG_MSG

using namespace std;

static char server_ip[20];

static int recv_bytes = 0;
static int send_bytes = 0;
static char ctrl_buffer[BUFSIZE + 1];
static char data_buf[DBUFSIZE + 1];

static int sockfd;
static struct sockaddr_in saddr;

static int d_sockfd;
static struct sockaddr_in d_saddr;
static char cmd[200];

void debug_msg(const int MSGNO, const int msg = 0) {
#ifdef DEBUG_MSG
	cout << DEBUG_HDR;

	switch (MSGNO) {
	case SENDCMD_ERROR:
		cout << "Error send command.";
		break;
	case INITDATACON:
		cout << "Initializing data connection.";
		break;
	case _ERROPENDATACON:
		cout << "Error open data connection.";
		break;
	case _DATACONOPEN:
		cout << "Data connection open.";
		break;
	case _DATACONCLOSE:
		cout << "Data connection closed.";
		break;
	case _TRANSABORTED:
		cout << "Error transfer data. Transfer aborted.";
		break;
	case PASV_ERROR:
		cout << "Error entering PASV mode.";
		break;
	case SHOW_DATA_PORT:
		cout << "Data port is\t" << msg;
		break;
	case SHOW_PORT:
		cout << "Port is\t" << msg;
		break;
	case RECEIVING:
		cout << "Receiving.";
		break;
	case RECV_LEN:
		cout << "Receiving " << msg << " bytes.";
		break;
	case USERNAME_ERROR:
		cout << "User does not exist or not authorized to login.";
		break;
	case _USERNAMEOK:
		cout << "User is accepted.";
		break;
	case _NOTLOGGEDIN:
		cout << "Error login.";
		break;
	case _LOGGEDIN:
		cout << "User login successful.";
		break;
	case ANONYM_ERROR:
		cout << "Error login as anonymous user.";
		break;
	case SOCKET_ERROR:
		cout << "Error open socket.";
		break;
	case _SVCREADYFORNEWU:
		cout << "Service not ready for new user.";
		break;
	case _CLOSECTRLCON:
		cout << "Control connection close. Quit now.";
		break;
	case _CMDERROR:
		cout << "Error run command.";
		break;
	case _FILEUNAVAIL:
		cout << "File unavailable.";
		break;
	case SHOW_CMD:
		cout << "Sending command:\n\t\t" << ctrl_buffer;
		break;
	case SHOW_DBUF:
		cout << "Data received is\n\t\t" << data_buf;
	default:
		cout << "Error unknown.";
	}
	cout << DEBUG_TAIL;
#endif
}

int usage(const int type = 1) {
	cout << USAGE_HDR;
	switch (type) {
	case 0:
		cout << "\tTo connect a ftp server, please type the cammand:\n";
		cout << "\t\t./client <server_ip> [server_port] [username password]";
		break;
	case 2:
		cout << "Command not implemented.";
		break;
	default:
		cout << "get <filename>\t:\tdownload file:<filename> from server.\n";
		cout << "put <filename>\t:\tupload file:<filename> to server.\n";
		cout << "pwd\t\t:\tshow current directory at server.\n";
		cout << "dir\t\t:\tlist files in the current directory.\n";
		cout << "cd <dirname>\t:\tchange current directory to ./<dirname>.\n";
		cout << "quit\t\t:\tquit client.\n";
		cout << "?\t\t\t:\tshow this help.";
		break;
	}
	cout << USAGE_TAIL;
	return 0;
}
void send_ctrl_buffer()//send buffer
{
	ctrl_buffer[send_bytes] = 0;
	debug_msg(SHOW_CMD);
	int len = write(sockfd, ctrl_buffer, send_bytes);
	if (send_bytes != len)
		debug_msg(SENDCMD_ERROR);
	assert(send_bytes == len);
}

void recv_ctrl_buffer() {
	do {
		debug_msg(RECEIVING);
		recv_bytes = read(sockfd, ctrl_buffer, BUFSIZE);
	} while (recv_bytes == 0);
	ctrl_buffer[recv_bytes] = 0;
	debug_msg(SHOW_CMD);
}

void send_cmd(char *s) {
	strcpy(ctrl_buffer, s);
	send_bytes = strlen(s);
	send_ctrl_buffer();
}

int ftp_code() {
	int ret = 0;
	assert(sscanf(ctrl_buffer, "%d", &ret) == 1);
	return ret;
}

int pasv() {
	debug_msg(INITDATACON);

	send_cmd("PASV\r\n");
	recv_ctrl_buffer();
	int portH, portL;
	int IP[4];
	if (sscanf(ctrl_buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
			&IP[3], &IP[2], &IP[1], &IP[0], &portH, &portL) != 6) {
		debug_msg(PASV_ERROR);
		return -1;
	} else {
		int dataPort = (portH << 8) | portL;
		//		printf("//Server data port: %d\n", dataPort);
		debug_msg(SHOW_DATA_PORT, dataPort);
		bzero(&saddr, sizeof(struct sockaddr_in));
		d_saddr.sin_family = AF_INET;
		d_saddr.sin_addr.s_addr = inet_addr(server_ip);
		d_saddr.sin_port = htons(dataPort);
		d_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (connect(d_sockfd, (struct sockaddr*) &d_saddr,
				sizeof(struct sockaddr_in)) < 0) {
			debug_msg(_ERROPENDATACON);
			close(d_sockfd);
			return -1;
		} else
			debug_msg(_DATACONOPEN);
	}

	return 0;
}

void send_file(FILE* d_f) {
	while (1) {
		int readlen = fread(data_buf, 1, DBUFSIZE, d_f);
		if (readlen == 0)
			break;
		int sendlen = write(d_sockfd, data_buf, readlen);
		if (sendlen != readlen) {
			debug_msg(_TRANSABORTED);
		}
		assert(sendlen == readlen);
	}

	debug_msg(SHOW_DATA_PORT, d_sockfd);
	debug_msg(SHOW_PORT, sockfd);
	close(d_sockfd);
}

void recv_file(FILE *d_f) {
	while (1) {
		recv_bytes = read(d_sockfd, data_buf, DBUFSIZE);

		if (d_f)
			debug_msg(RECV_LEN, recv_bytes);

		if (recv_bytes == 0)
			break;

		data_buf[recv_bytes] = 0;
		if (d_f) {
			fwrite(data_buf, 1, recv_bytes, d_f);
		} else
			debug_msg(SHOW_DBUF);
	}
	close(d_sockfd);
}

int login(char* user, char* pass) {
	char cmd[100];
	sprintf(cmd, "USER %s\r\n", user);
	send_cmd(cmd);
	recv_ctrl_buffer();
	if (ftp_code() != _USERNAMEOK) {
		debug_msg(USERNAME_ERROR);
		close(sockfd);
		return -1;
	} else
		debug_msg(_USERNAMEOK);

	sprintf(cmd, "PASS %s\r\n", pass);
	send_cmd(cmd);
	recv_ctrl_buffer();
	if (ftp_code() != _LOGGEDIN) {
		debug_msg(_NOTLOGGEDIN);
		close(sockfd);
		return -1;
	} else
		debug_msg(_LOGGEDIN);

	return 0;
}

int login_anonymous() {
	send_cmd("USER anonymous\r\n");
	recv_ctrl_buffer();
	if (ftp_code() != _LOGGEDIN) {
		debug_msg(ANONYM_ERROR);
		close(sockfd);
		return -1;
	}
	return 0;
}

int ftp_quit() {
	send_cmd("QUIT\r\n");
	recv_ctrl_buffer();
	if (ftp_code() != _CLOSECTRLCON)
		debug_msg(_CLOSECTRLCON);
	close(sockfd);
	exit(0);
}

int ftp_get() {
	char filename[200];
	scanf("%s", filename);
	//			scanf("%255[^\n]", filename);
	send_cmd("TYPE I\r\n");
	recv_ctrl_buffer();
	if (ftp_code() != _CMDOK)
		debug_msg(_CMDERROR);

	/*
	 sprintf(cmd, "SIZE %s\r\n", filename);
	 send_cmd(cmd);
	 recv_ctrl_buffer();
	 if(ftp_code() != _FILESTAT)
	 {
	 printf("get response error.\n");
	 }*/

	pasv();

	sprintf(cmd, "RETR %s\r\n", filename);
	send_cmd(cmd);
	recv_ctrl_buffer();
	if (ftp_code() != _FILESTATUSOK)
		debug_msg(_FILEUNAVAIL);

	FILE *f = fopen(filename, "wb");
	recv_file(f);

	recv_ctrl_buffer();
	if (ftp_code() != _DATACONCLOSE)
		debug_msg(_TRANSABORTED);

	fclose(f);
	return 0;
}
int ftp_cd() {
	char dirname[DIRSIZE];
	scanf("%255[^\n]", dirname);
	sprintf(cmd, "CWD %s\r\n", dirname + 1);
	send_cmd(cmd);
	recv_ctrl_buffer();
	if (ftp_code() != _FILEACTOK)
		debug_msg(ftp_code());
	return 0;
}
int ftp_dir() {
	send_cmd("TYPE A\r\n");
	recv_ctrl_buffer();
	if (ftp_code() != _CMDOK)
		debug_msg(_CMDERROR);

	pasv();
	debug_msg(_DATACONOPEN);

	send_cmd("LIST\r\n");
	recv_ctrl_buffer();
	if (ftp_code() != _FILESTATUSOK)
		debug_msg(_FILEUNAVAIL);

	recv_file(0);

	recv_ctrl_buffer();
	if (ftp_code() != _DATACONCLOSE)
		debug_msg(_TRANSABORTED);
	else
		debug_msg(_DATACONCLOSE);
	return 0;
}
int ftp_pwd() {
	send_cmd("PWD\r\n");
	recv_ctrl_buffer();
	if (ftp_code() != _PATHNAMECREATED)
		debug_msg(_CMDERROR);
	return 0;
}
int ftp_put() {
	char filename[200];
	scanf("%s", filename);
	//			scanf("%255[^\n]", filename);

	send_cmd("TYPE I\r\n");
	recv_ctrl_buffer();
	if (ftp_code() != _CMDOK)
		debug_msg(_CMDERROR);

	pasv();

	sprintf(cmd, "STOR %s\r\n", filename);
	send_cmd(cmd);
	recv_ctrl_buffer();

	if (ftp_code() != _FILESTATUSOK)
		debug_msg(_FILEUNAVAIL);

	FILE *f = fopen(filename, "rb");
	send_file(f);

	recv_ctrl_buffer();
	if (ftp_code() != _DATACONCLOSE)
		debug_msg(_TRANSABORTED);

	fclose(f);
	return 0;
}
int main(int argc, char** argv) {
	if (argc != 1 && argc != 3 && argc != 5) {
		usage(0);
	}
	int port;
	strcpy(server_ip, argv[1]);
	assert(sscanf(argv[2], "%d", &port) == 1);

	bzero(&saddr, sizeof(struct sockaddr_in));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(server_ip);
	saddr.sin_port = htons(port);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(sockfd, (struct sockaddr*) &saddr, sizeof(struct sockaddr_in))
			< 0) {
		debug_msg(SOCKET_ERROR);
		close(sockfd);
		return -1;
	}
	recv_ctrl_buffer();
	if (ftp_code() != _SVCREADYFORNEWU) {
		debug_msg(_SVCREADYFORNEWU);
		close(sockfd);
		return -1;
	}

	if (argc == 5) {
		if (login(argv[3], argv[4]) < 0)
			return -1;
	} else {
		if (login_anonymous() < 0)
			return -1;
	}

	while (1) {
		printf("ftp>");
		scanf("%s", cmd);
		if (strcmp(cmd, "?") == 0) {
			usage();
		} else if (strcmp(cmd, "quit") == 0) {
			ftp_quit();
		} else if (strcmp(cmd, "get") == 0) {
			ftp_get();
		} else if (strcmp(cmd, "put") == 0) {
			ftp_put();
		} else if (strcmp(cmd, "pwd") == 0) {
			ftp_pwd();
		} else if (strcmp(cmd, "dir") == 0) {
			ftp_dir();
		} else if (strcmp(cmd, "cd") == 0) {
			ftp_cd();
		} else
			usage(2);
	}

	return 0;
}
