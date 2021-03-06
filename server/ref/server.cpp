#pragma comment (lib,"ws2_32")
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
//#include <WinSock.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <fstream>
#include <iostream>
#include <errno.h>
#include <time.h>
using namespace std;

const long MAX_LINE_NUM = 1001;
const char ACKNOWLEDGE[100] = "RECEIVED";

WSADATA wsaData;
SOCKET AcceptSocket;
sockaddr_in service;
SOCKET ListenSocket;

void Getsockname(SOCKET sock) {
	int connfd = sizeof(struct sockaddr);
	sockaddr_in addr;
	if (getsockname(sock, (struct sockaddr *) &addr, &connfd) == SOCKET_ERROR) {
		perror("getsockname");
		closesocket(sock);
		exit(1);
	}
	cout << "Server is listening on port " << ntohs(addr.sin_port) << endl;
}

void put_file(char *name) {
	long p, buf_len;
	char filename[MAX_LINE_NUM], buf[MAX_LINE_NUM];
	p = 3;
	while (name[p] == ' ')
		++p;
	strcpy(filename, name + p);

	FILE *fout = fopen(filename, "w");
	while (buf_len = recv(AcceptSocket, buf, MAX_LINE_NUM, 0)) {
		buf[buf_len] = '\0';
		if ((buf_len == 3) && (buf[0] == 'E') && (buf[1] == 'N') && (buf[2]
				== 'D'))
			break; //signal for stopping

		fprintf(fout, "%s", buf);
		strcpy(buf, "");

		send(AcceptSocket, ACKNOWLEDGE, strlen(ACKNOWLEDGE), 0);
	}
	fclose(fout);
}

void get_file(char *name) {
	long p;
	char filename[MAX_LINE_NUM];
	p = 3;
	while (name[p] == ' ')
		++p;
	strcpy(filename, name + p);
	FILE *fin = fopen(filename, "r");
	if (!fin)
		send(AcceptSocket, "NOFILE", 6, 0);
	else {
		char line[MAX_LINE_NUM + 1];
		while (fgets(line, MAX_LINE_NUM, fin)) {
			send(AcceptSocket, line, strlen(line), 0);
			while (!recv(AcceptSocket, line, MAX_LINE_NUM, 0)) {
			} //waiting for the acknowledge message
		}
		strcpy(line, "END");
		send(AcceptSocket, line, strlen(line), 0);

		fclose(fin);
	}
}

void show_directory() {
	FILE *fin = _popen("dir", "r");
	char result[MAX_LINE_NUM * 10], line[MAX_LINE_NUM];

	strcpy(result, "");
	while (fgets(line, MAX_LINE_NUM, fin))
		strcat(result, line);
	send(AcceptSocket, result, strlen(result), 0);
	fclose(fin);
}

void change_directory(char name[]) {
	long p;
	char dir_name[MAX_LINE_NUM], result[MAX_LINE_NUM];
	p = 2;
	while ((p < strlen(name)) && (name[p] == ' '))
		++p;
	if (p >= strlen(name)) {
		strcpy(result, "NO");
		send(AcceptSocket, result, strlen(result), 0);
	}
	strcpy(dir_name, name + p);
	dir_name[strlen(name) - p] = '\0';

	if (chdir(dir_name)) {
		strcpy(result, "NO");
		send(AcceptSocket, result, strlen(result), 0);
	} else {
		strcpy(result, "YES");
		send(AcceptSocket, result, strlen(result), 0);
	}
}

void make_directory(char name[]) {
	long p;
	char dir_name[MAX_LINE_NUM], result[MAX_LINE_NUM];
	p = 2;
	while ((p < strlen(name)) && (name[p] == ' '))
		++p;
	if (p >= strlen(name)) {
		strcpy(result, "NO");
		send(AcceptSocket, result, strlen(result), 0);
	}
	strcpy(dir_name, name + p);
	dir_name[strlen(name) - p] = '\0';

	if (mkdir(dir_name)) {
		strcpy(result, "NO");
		send(AcceptSocket, result, strlen(result), 0);
	} else {
		strcpy(result, "YES");
		send(AcceptSocket, result, strlen(result), 0);
	}
}

void remove_directory(char name[]) {
	long p;
	char dir_name[MAX_LINE_NUM], result[MAX_LINE_NUM];
	p = 2;
	while ((p < strlen(name)) && (name[p] == ' '))
		++p;
	if (p >= strlen(name)) {
		strcpy(result, "NO");
		send(AcceptSocket, result, strlen(result), 0);
	}
	strcpy(dir_name, name + p);
	dir_name[strlen(name) - p] = '\0';

	if (rmdir(dir_name)) {
		strcpy(result, "NO");
		send(AcceptSocket, result, strlen(result), 0);
	} else {
		strcpy(result, "YES");
		send(AcceptSocket, result, strlen(result), 0);
	}
}

void pwd() {
	char direct[MAX_LINE_NUM];
	getcwd(direct, MAX_LINE_NUM);
	send(AcceptSocket, direct, strlen(direct), 0);
}

void main() {

	//----------------------
	//��ʼ��Winsock.

	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");

	//----------------------
	//���������õ�socket
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return;
	}

	//----------------------
	// �趨��ַ��ʽΪIPv4���󶨶˿�

	service.sin_family = AF_INET;
	service.sin_addr.s_addr = htonl(INADDR_ANY);
	service.sin_port = htons(0);

	if (bind(ListenSocket, (SOCKADDR*) &service, sizeof(service))
			== SOCKET_ERROR) {
		printf("bind() failed.\n");
		closesocket(ListenSocket);
		return;
	}

	//----------------------
	// �ڽ�����socket�м�������

	if (listen(ListenSocket, 1) == SOCKET_ERROR)
		printf("Error listening on socket.\n");

	//----------------------
	//������һ��socket��������
	printf("Waiting for client to connect...\n");
	Getsockname(ListenSocket);

	//communication with the client
	AcceptSocket = accept(ListenSocket, NULL, NULL);
	while (1) {
		char name[MAX_LINE_NUM];

		long t = recv(AcceptSocket, name, MAX_LINE_NUM, 0);
		name[t] = '\0';

		if ((name[0] == 'G') && (name[1] == 'E') && (name[2] == 'T'))
			get_file(name);
		else if ((name[0] == 'P') && (name[1] = 'U') && (name[2] == 'T'))
			put_file(name);
		else if ((name[0] == 'Q') && (name[1] = 'U') && (name[2] == 'I')
				&& (name[3] == 'T')) {
			closesocket(AcceptSocket);
			printf("Bye Bye!\n");
			break;
		} else if ((name[0] == 'D') && (name[1] = 'I') && (name[2] == 'R'))
			show_directory();
		else if ((name[0] == 'P') && (name[1] = 'W') && (name[2] == 'D'))
			pwd();
		else if ((name[0] == 'C') && (name[1] = 'D'))
			change_directory(name);
		else if ((name[0] == 'M') && (name[1] = 'D'))
			make_directory(name);
		else if ((name[0] == 'R') && (name[1] = 'D'))
			remove_directory(name);
	}

	WSACleanup();
	return;
}
