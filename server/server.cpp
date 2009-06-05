/*
 * server.cpp
 *
 *  Created on: Jun 1, 2009
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

#define LISTEN_PORT 1988
int local_ip[4] = { 58, 66, 138, 127 };

static int sockfd;
static int clientsock;
static int clientsock_orig;
static int data_clientsock;

static char ctrl_buffer[BUFSIZE + 1];
static int recv_bytes = 0;
static char list_buf[DBUFSIZE];
static int list_buf_size = 0;
static char data_buf[DBUFSIZE];
static char work_dir[DIRSIZE] = ".";

void wait(const int time = 1) {
	int intval = WAIT_TIME * time;
	while (intval--)
		;
}
void FTP_SEND_RESP(char* Res) {
	wait();
	write(clientsock, Res, strlen(Res));
#ifdef DEBUG_OUTPUT
	cout << "Response: " << Res;
#endif
}

void SEND_RESP(int Num) {
	char Res[RESPSIZE];
	memset(Res, 0, RESPSIZE);
	sprintf(Res, "%d\r\n", Num);
	FTP_SEND_RESP(Res);
}

void GET_RQST() {
	wait();
	recv_bytes = read(clientsock, ctrl_buffer, BUFSIZE);
	ctrl_buffer[recv_bytes] = 0;
#ifdef DEBUG_OUTPUT
	cout << ctrl_buffer;
#endif
}

int begin_with(const char* strShort, const char* strLong) {
	return strncmp(strShort, strLong, strlen(strShort)) == 0;
}

int PASV() {
	struct sockaddr_in d_myaddr;
	bzero(&d_myaddr, sizeof(struct sockaddr_in));
	d_myaddr.sin_family = AF_INET;
	d_myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	d_myaddr.sin_port = htons(LISTEN_PORT + 1000);
	int d_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	while (bind(d_sockfd, (struct sockaddr*) &d_myaddr,
			sizeof(struct sockaddr_in)) < 0) {
		cout << "Bind data port error.\n";
		d_myaddr.sin_port = htons(ntohs(d_myaddr.sin_port) + 1);
	}
	if (listen(d_sockfd, 1) < 0) {
		cout << "Listen error.\n";
		return -1;
	}

	char temp[RESPSIZE];
	sprintf(temp, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n",
			local_ip[0], local_ip[1], local_ip[2], local_ip[3],
			ntohs(d_myaddr.sin_port) >> 8, ntohs(d_myaddr.sin_port) & 0xFF);

	cout << temp;
	cout << "port " << ntohs(d_myaddr.sin_port) << "\n";

	FTP_SEND_RESP(temp);

	cout << "Listen\n";

	struct sockaddr_in d_clientaddr;
	socklen_t sockLength = 0;

	data_clientsock = accept(d_sockfd, (struct sockaddr*) &d_clientaddr,
			&sockLength);
	if (data_clientsock < 0) {
		cout << "Accept error.\n";
		return -1;
	} else {
		cout << "Data Connnection Accept.\n";
		return 0;
	}
}

void FTP_CD(char *dirname) {
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
		cout << "debug: work_dir " << work_dir << "\n";
		if (opendir(work_dir) == NULL) {
			work_dir[l] = 0;
		}
	}
}

void FILL_LIST_BUF() {
	list_buf[0] = 0;
	list_buf_size = 0;
	strcat(list_buf, "DIR\r\n=================\r\n");

	DIR *dp;
	struct dirent *entry;
	//	struct stat statbuf;

	if ((dp = opendir(work_dir)) == NULL) {
		return;
	}

	while ((entry = readdir(dp)) != NULL) {
		strcat(list_buf, entry->d_name);
		strcat(list_buf, "\r\n");
	}
	closedir(dp);

	strcat(list_buf, "=================\r\n");
	list_buf_size = strlen(list_buf);
}

void SEND_FILE(FILE* d_f) {
	if (d_f) {
		while (1) {
			int readlen = fread(data_buf, 1, DBUFSIZE, d_f);
			if (readlen == 0)
				break;
			int sendlen = write(data_clientsock, data_buf, readlen);
			assert(readlen == sendlen);
		}
		cout << "data_clientsock is " << data_clientsock << endl
				<< "clientsock is " << clientsock << endl;
		close(data_clientsock);
		fclose(d_f);
	} else {
		FILL_LIST_BUF();
		int sendlen = write(data_clientsock, list_buf, list_buf_size);
		assert(list_buf_size == sendlen);
		close(data_clientsock);
	}

}

void RECV_FILE(FILE* d_f) {
	while (1) {
		int recvlen = read(data_clientsock, data_buf, DBUFSIZE);
		cout << "recv " << recvlen << " bytes\n";
		if (recvlen == 0)
			break;
		data_buf[recvlen] = 0;
		fwrite(data_buf, 1, recvlen, d_f);
	}

	cout << "data_clientsock is " << data_clientsock << endl
			<< "clientsock is " << clientsock << endl << "file is " << d_f
			<< endl;

	fclose(d_f);
	close(data_clientsock);
}

int start_server() {
	struct sockaddr_in myaddr;
	bzero(&myaddr, sizeof(struct sockaddr_in));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(LISTEN_PORT);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(sockfd, (struct sockaddr*) &myaddr, sizeof(struct sockaddr_in))
			< 0) {
		//		cout << "Bind error.\n";
		wait(5);
		return -1;
	}

	if (listen(sockfd, 1) < 0) {
		cout << "Listen error.\n";
		return -1;
	}

	struct sockaddr_in clientaddr;
	socklen_t sockLength = 0;

	clientsock = accept(sockfd, (struct sockaddr*) &clientaddr, &sockLength);
	if (clientsock < 0) {
		cout << "Error accept client socket.";
		return -1;
	}

	SEND_RESP(_SVCREADYFORNEWU);

	char user[LOGINBUFSIZE];
	char password[LOGINBUFSIZE];

	while (1) {
		GET_RQST();
		if (strcmp("USER anonymous\r\n", ctrl_buffer) == 0) {
			strcpy(user, ctrl_buffer + 5);
			cout << "USER " << user << endl;
			SEND_RESP(_LOGGEDIN);
			break;
		} else if (begin_with("USER ", ctrl_buffer)) {
			strcpy(user, ctrl_buffer + 5);
			SEND_RESP(_USERNAMEOK);
			while (1) {
				GET_RQST();
				if (begin_with("PASS ", ctrl_buffer)) {
					strcpy(password, ctrl_buffer + 5);
					cout << "PASS " << password << endl;
					SEND_RESP(_LOGGEDIN);
					break;
				} else
					SEND_RESP(_NOTLOGGEDIN);
			}
			break;
		} else
			SEND_RESP(_NOTLOGGEDIN);
	}

	cout << "FTP service ready.";

	char temp[CMDSIZE + ARGSIZE];
	while (1) {
		GET_RQST();
		if (strcmp("QUIT\r\n", ctrl_buffer) == 0) {
			SEND_RESP(_CLOSECTRLCON);
			break;
		} else if (strcmp("TYPE I\r\n", ctrl_buffer) == 0) {
			SEND_RESP(_CMDOK);
		} else if (strcmp("TYPE A\r\n", ctrl_buffer) == 0) {
			cout << "TYPE A\n";
			SEND_RESP(_CMDOK);
		} else if (strcmp("PWD\r\n", ctrl_buffer) == 0) {
			if (strcmp(work_dir, ".") == 0)
				sprintf(temp, "257 \"/\" is current directory.\r\n");
			else
				sprintf(temp, "257 \"%s\" is current directory.\r\n", work_dir
						+ 1);
			FTP_SEND_RESP(temp);
		} else if (begin_with("CWD ", ctrl_buffer)) {
			strcpy(temp, ctrl_buffer + 4);
			int t = strlen(temp) - 2;
			temp[t] = 0;
			cout << "CWD " << temp << endl;
			FTP_CD(temp);
			//			FTP_SEND_RESP("250 CWD command successful.\r\n");
			SEND_RESP(_FILEACTOK);
		} else if (strcmp("PASV\r\n", ctrl_buffer) == 0) {
			if (PASV() < 0) {
				cout << "Error entering PASV mode.";
				break;
			}
		} else if (strcmp("LIST\r\n", ctrl_buffer) == 0) {
			SEND_RESP(_FILESTATUSOK);
			SEND_FILE(0);
			SEND_RESP(_DATACONCLOSE);
		} else if (begin_with("RETR ", ctrl_buffer)) {
			strcpy(temp, work_dir);
			strcat(temp, "/");
			strcat(temp, ctrl_buffer + 5);
			int t = strlen(temp) - 2;
			temp[t] = 0;
			cout << "RETR " << temp << endl;

			FILE *f = fopen(temp, "rb");

			SEND_RESP(_FILESTATUSOK);
			SEND_FILE(f);
			SEND_RESP(_DATACONCLOSE);

		} else if (begin_with("STOR ", ctrl_buffer)) {
			strcpy(temp, work_dir);
			strcat(temp, "/");
			strcat(temp, ctrl_buffer + 5);
			int t = strlen(temp) - 2;
			temp[t] = 0;
			cout << "STOR " << temp << endl;

			FILE *f = fopen(temp, "wb");

			SEND_RESP(_FILESTATUSOK);
			clientsock_orig = clientsock;
			RECV_FILE(f);
			clientsock = clientsock_orig;
			SEND_RESP(_DATACONCLOSE);
		} else {
			printf("debug: Not support: \"%s\"\n", ctrl_buffer);
			SEND_RESP(_CMDNOTIMPLEMENTED);
		}
	}

	close(clientsock);
	return 1;
}
int reset() {
	close(clientsock);
	close(sockfd);
	memset(ctrl_buffer, 0, BUFSIZE + 1);
	recv_bytes = 0;
	sockfd = 0;
	clientsock = 0;
	clientsock_orig = 0;
	memset(work_dir, 0, DIRSIZE);
	strcpy(work_dir, ".");

	return 0;
}

int main() {
	while (true) {
		reset();
		wait(20);
		start_server();
	}
	return 0;
}
