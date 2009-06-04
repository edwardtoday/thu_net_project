/*
 * client.cpp
 *
 *  Created on: May 28, 2009
 *      Author: Q
 */

#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <cstring>

using namespace std;

//#define PORT 21
#define BUFFERSIZE 512
#define DEBUG_OUTPUT true
#define CCSIZE 200
#define RESULTSIZE 200
#define DIRLENGTH 200
#define CMDBUFFER 100

int PORT;
bool firstline = true;

char* nextToken(char* src) {
	static char* in;
	static char result[RESULTSIZE];
	int curr = 0;
	if (src) {
		in = src;
	}
	if (*in == ')')
		return NULL;
	while (!isdigit(*in))
		in++;
	while (isdigit(*in)) {
		result[curr++] = *in++;
	}
	result[curr] = '\0';
	if (DEBUG_OUTPUT) {
		//        cout << "***************************" << endl;
		//        cout << (result == NULL) << endl;
		cout << strlen(result) << endl;
	}
	return result;
}

int pasv(int sockfd) {
	int numbytes;
	char cc[CCSIZE];
	char buf[BUFFERSIZE];

	memset(cc, 0, CCSIZE);
	strcpy(cc, "PASV\r\n");
	if (send(sockfd, cc, strlen(cc), 0) == -1) {
		perror("Send Error");
	}

	//    memset(buf, 0, BUFFERSIZE);
	//    if ((numbytes = recv(sockfd, buf, BUFFERSIZE, 0)) == -1) {
	//        printf("numbytes: %d\n", numbytes);
	//        perror("Recv Error");
	//        exit(1);
	//    }
	//    buf[numbytes] = '\0';
	//    printf("Recv: %s", buf);

	memset(buf, 0, BUFFERSIZE);
	firstline = true;
	while ((numbytes = recv(sockfd, buf, BUFFERSIZE, 0)) != 0) {
		if (DEBUG_OUTPUT)
			cout << "recv data........." << endl;
		if (numbytes == -1) {
			printf("numbytes: %d\n", numbytes);
			perror("Recv Error");
			exit(1);
		}
		buf[numbytes] = '\0';
		if (firstline)
			printf("Recv: %s", buf);
		else
			printf("%s", buf);
		memset(buf, 0, BUFFERSIZE);
	}

	char data_ip[20];
	memset(data_ip, 0, 20);
	char point[2] = ".";
	int data_port = 0;
	char* ret = nextToken(buf);
	int i = 0;
	while (ret) {
		if (i == 0) {
			i++;
			ret = nextToken(NULL);
			continue;
		}
		if (i < 4) {
			strcat(data_ip, ret);
			strcat(data_ip, point);
		}
		if (i == 4)
			strcat(data_ip, ret);
		if (i == 5)
			data_port += atoi(ret) * 256;
		if (i == 6)
			data_port += atoi(ret);
		i++;
		ret = nextToken(NULL);
	}

	//应该建立另外一条数据通路
	int s_sockfd = 0;
	struct sockaddr_in their_data_addr;
	if ((s_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket Error");
		exit(1);
	}

	their_data_addr.sin_family = AF_INET;
	their_data_addr.sin_port = htons(data_port);
	inet_pton(AF_INET, data_ip, &their_data_addr.sin_addr);
	bzero(&(their_data_addr.sin_zero), 8);

	memset(buf, 0, BUFFERSIZE);
	if (connect(s_sockfd, (struct sockaddr *) &their_data_addr,
			sizeof(struct sockaddr)) == -1) {
		perror("Connect Error");
		exit(1);
	}
	if (s_sockfd == 0) {
		cout << "Error pasv: s_sockfd = 0" << endl;
		exit(1);
	}
	cout << "In pasv():" << s_sockfd << endl;
	return s_sockfd;
}

void dir(int sockfd) {
	int numbytes;
	char cc[CCSIZE];
	char buf[BUFFERSIZE];

	int s_sockfd = pasv(sockfd);
	int s_numbytes;

	//上面数据通路建立成功
	//==================================

	//发送LIST -aL指令
	memset(cc, 0, CCSIZE);
	strcpy(cc, "LIST -aL\r\n");
	if (send(sockfd, cc, strlen(cc), 0) == -1) {
		perror("Send Error");
	}

	//在控制通路上获得回应
	memset(buf, 0, BUFFERSIZE);
	firstline = true;
	while ((numbytes = recv(sockfd, buf, BUFFERSIZE, 0)) != 0) {
		if (DEBUG_OUTPUT)
			cout << "recv data........." << endl;
		if (numbytes == -1) {
			printf("numbytes: %d\n", numbytes);
			perror("Recv Error");
			exit(1);
		}
		buf[numbytes] = '\0';
		if (firstline)
			printf("Recv: %s", buf);
		else
			printf("%s", buf);
		memset(buf, 0, BUFFERSIZE);
	}

	//    if ((numbytes = recv(sockfd, buf, BUFFERSIZE, 0)) == -1) {
	//        printf("numbytes: %d\n", numbytes);
	//        perror("Recv Error");
	//        exit(1);
	//    }
	//    buf[numbytes] = '\0';
	//    printf("Recv: %s", buf);

	//在数据通路上获得数据
	memset(buf, 0, BUFFERSIZE);
	while ((s_numbytes = recv(s_sockfd, buf, BUFFERSIZE, 0)) != 0) {
		if (DEBUG_OUTPUT)
			cout << "recv data........." << endl;
		if (s_numbytes == -1) {
			printf("numbytes: %d\n", s_numbytes);
			perror("Recv Error");
			exit(1);
		}
		printf("%s", buf);
		memset(buf, 0, BUFFERSIZE);
	}

	close(s_sockfd);

	//在控制通路上获得传输完成回应
	memset(buf, 0, BUFFERSIZE);
	if ((numbytes = recv(sockfd, buf, BUFFERSIZE, 0)) == -1) {
		printf("numbytes: %d\n", numbytes);
		perror("Recv Error");
		exit(1);
	}
	buf[numbytes] = '\0';
	printf("Recv: %s", buf);
}

void getCurrDir(char* src, char* currDir) {
	char *in = src;
	char result[RESULTSIZE];
	int curr = 0;
	while (*in != '\"')
		in++;
	in++;
	while (*in != '\"') {
		result[curr++] = *in++;
	}
	result[curr] = '\0';
	strcpy(currDir, result);
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

void pwd(int sock, char* currDir) {
	char pwdBuf[BUFFERSIZE];
	char cc[CCSIZE];
	int numbytes;
	memset(cc, 0, CCSIZE);
	strcpy(cc, "PWD\r\n");
	if (send(sock, cc, strlen(cc), 0) == -1) {
		perror("Send Error");
	}
	memset(pwdBuf, 0, BUFFERSIZE);
	if ((numbytes = recv(sock, pwdBuf, BUFFERSIZE, 0)) == -1) {
		printf("numbytes: %d\n", numbytes);
		perror("Recv Error");
		exit(1);
	}
	cout << "pwdBufSize: " << numbytes << endl;
	pwdBuf[numbytes] = '\0';
	cout << "currDirIsNULL: " << (currDir == NULL) << endl;
	getCurrDir(pwdBuf, currDir);
}

void cd(int sock, char* src) {
	char pwdBuf[BUFFERSIZE];
	char cc[CCSIZE];
	char s[2] = "/";
	int numbytes;
	memset(cc, 0, CCSIZE);
	strcpy(cc, "CWD ");
	char *currDir = new char;
	char *fathDir = new char;

	cout << "before pwd" << endl;
	pwd(sock, currDir);
	if (!strcmp(src, "..")) {
		getFathDir(currDir, fathDir);
		strcat(cc, fathDir);
		strcat(cc, "\r\n");
		if (send(sock, cc, strlen(cc), 0) == -1) {
			perror("Send Error");
		}
	} else {
		if (currDir[strlen(currDir) - 1] != '/') {
			strcat(currDir, s);
		}
		strcat(cc, currDir);
		strcat(cc, src);
		strcat(cc, "\r\n");
		if (send(sock, cc, strlen(cc), 0) == -1) {
			perror("Send Error");
		}
	}
	memset(pwdBuf, 0, BUFFERSIZE);
	if ((numbytes = recv(sock, pwdBuf, BUFFERSIZE, 0)) == -1) {
		printf("numbytes: %d\n", numbytes);
		perror("Recv Error");
		exit(1);
	}
	pwdBuf[numbytes] = '\0';
	printf("Recv: %s", pwdBuf);
}

void get(int sockfd, char* src) {
	cout << -1 << endl;
	int numbytes;
	char cc[CCSIZE];
	char buf[BUFFERSIZE];
	int s_sockfd = pasv(sockfd);
	int s_numbytes;
	cout << 0 << endl;
	FILE *f = fopen(src, "w");

	cout << "1" << endl;
	//上面数据通路建立成功
	//==================================

	//发送RETR指令
	memset(cc, 0, CCSIZE);
	strcpy(cc, "RETR ");
	char *currDir = new char;
	pwd(sockfd, currDir);
	char s[2] = "/";
	if (currDir[strlen(currDir) - 1] != '/') {
		strcat(currDir, s);
	}
	strcat(cc, currDir);
	strcat(cc, src);
	strcat(cc, "\r\n");
	if (send(sockfd, cc, strlen(cc), 0) == -1) {
		perror("Send Error");
	}

	cout << 2 << endl;

	//在控制通路上获得回应
	memset(buf, 0, BUFFERSIZE);
	if ((numbytes = recv(sockfd, buf, BUFFERSIZE, 0)) == -1) {
		printf("numbytes: %d\n", numbytes);
		perror("Recv Error");
		exit(1);
	}
	buf[numbytes] = '\0';
	printf("Recv: %s", buf);

	if (s_sockfd == 0)
		cout << "error" << endl;

	//在数据通路上获得数据
	memset(buf, 0, BUFFERSIZE);
	while ((s_numbytes = recv(s_sockfd, buf, BUFFERSIZE, 0)) != 0) {
		if (s_numbytes == -1) {
			printf("numbytes: %d\n", numbytes);
			perror("Recv Error");
			exit(1);
		}
		fwrite(buf, 1, strlen(buf), f);
		memset(buf, 0, BUFFERSIZE);
	}
	close(s_sockfd);
	fclose(f);

	cout << 3 << endl;
	//在控制通路上获得传输完成回应
	memset(buf, 0, BUFFERSIZE);
	if ((numbytes = recv(sockfd, buf, BUFFERSIZE, 0)) == -1) {
		printf("numbytes: %d\n", numbytes);
		perror("Recv Error");
		exit(1);
	}
	buf[numbytes] = '\0';
	printf("Recv: %s", buf);
}

void put(int sockfd, char *src) {
	int numbytes;
	char cc[CCSIZE];
	char buf[BUFFERSIZE];
	FILE *f = fopen(src, "r");

	int s_sockfd = pasv(sockfd);
	int s_numbytes;

	//上面数据通路建立成功
	//==================================

	//发送STOR指令
	memset(cc, 0, CCSIZE);
	strcpy(cc, "STOR ");
	char *currDir = new char;
	pwd(sockfd, currDir);
	char s[2] = "/";
	if (currDir[strlen(currDir) - 1] != '/') {
		strcat(currDir, s);
	}
	strcat(cc, currDir);
	strcat(cc, src);
	strcat(cc, "\r\n");
	if (send(sockfd, cc, strlen(cc), 0) == -1) {
		perror("Send Error");
	}

	//在控制通路上获得回应
	memset(buf, 0, BUFFERSIZE);
	if ((numbytes = recv(sockfd, buf, BUFFERSIZE, 0)) == -1) {
		printf("numbytes: %d\n", numbytes);
		perror("Recv Error");
		exit(1);
	}
	buf[numbytes] = '\0';
	printf("Recv: %s", buf);

	memset(buf, 0, BUFFERSIZE);
	while (fread(buf, 1, BUFFERSIZE, f) != 0) {
		if ((s_numbytes = send(s_sockfd, buf, BUFFERSIZE, 0)) == -1) {
			printf("numbytes: %d\n", numbytes);
			perror("Recv Error");
			exit(1);
		}
		memset(buf, 0, BUFFERSIZE);
	}

	fclose(f);
	cout << endl;
	close(s_sockfd);

	//在控制通路上获得传输完成回应
	memset(buf, 0, BUFFERSIZE);
	if ((numbytes = recv(sockfd, buf, BUFFERSIZE, 0)) == -1) {
		printf("numbytes: %d\n", numbytes);
		perror("Recv Error");
		exit(1);
	}
	buf[numbytes] = '\0';
	printf("Recv: %s", buf);
}

int main(int argc, char *argv[]) {
	int sockfd, numbytes;
	char buf[BUFFERSIZE];
	char cDir[DIRLENGTH];

	if (argc != 6) {
		fprintf(
				stderr,
				"Usage: client -i <ip> <username> <password> <port>\n       client -h <hostname> <username> <password> <port>\n");
		exit(1);
	}

	PORT = atoi(argv[5]);

	struct hostent *he;
	struct sockaddr_in their_addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket Error");
		exit(1);
	}
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(PORT);

	if (strcmp(argv[1], "-i") == 0) {
		cout << "-i" << endl;
		inet_pton(AF_INET, argv[2], &their_addr.sin_addr);

	} else if (strcmp(argv[1], "-h") == 0) {
		cout << "-h" << endl;
		if ((he = gethostbyname(argv[2])) == NULL) {
			herror("Gethostbyname Error");
			exit(1);
		}
		their_addr.sin_addr = *((struct in_addr *) he->h_addr);
	} else {
		cout << "bad arguments" << endl;
		exit(1);
	}

	bzero(&(their_addr.sin_zero), 8);
	if (connect(sockfd, (struct sockaddr *) &their_addr,
			sizeof(struct sockaddr)) == -1) {
		perror("Connect Error");
		exit(1);
	}
	//上面的代码建立连接完毕

	//====================================================================================

	//检查连接
	memset(buf, 0, BUFFERSIZE);
	if ((numbytes = recv(sockfd, buf, BUFFERSIZE, 0)) == -1) {
		printf("numbytes: %d\n", numbytes);
		perror("Recv Error");
		exit(1);
	}
	buf[numbytes] = '\0';
	printf("Recv: %s", buf);

	char *cc = new char[CCSIZE];
	//发送用户名
	memset(cc, 0, CCSIZE);
	strcpy(cc, "USER ");
	strcat(cc, argv[3]);
	strcat(cc, "\r\n");
	if (send(sockfd, cc, strlen(cc), 0) == -1) {
		perror("Send Error");
	}

	//检查连接
	memset(buf, 0, BUFFERSIZE);
	if ((numbytes = recv(sockfd, buf, BUFFERSIZE, 0)) == -1) {
		printf("numbytes: %d\n", numbytes);
		perror("Recv Error");
		exit(1);
	}
	buf[numbytes] = '\0';
	printf("Recv: %s", buf);

	//发送密码
	memset(cc, 0, CCSIZE);
	strcpy(cc, "PASS ");
	strcat(cc, argv[4]);
	strcat(cc, "\r\n");
	if (send(sockfd, cc, strlen(cc), 0) == -1) {
		perror("Send Error");
	}

	//检查是否登录成功

	memset(buf, 0, BUFFERSIZE);
	if ((numbytes = recv(sockfd, buf, BUFFERSIZE, 0)) == -1) {
		printf("numbytes: %d\n", numbytes);
		perror("Recv Error");
		exit(1);
	}
	buf[numbytes] = '\0';
	printf("Recv: %s", buf);

	//上面代码用户已成功登录

	//========================================================================================

	memset(cDir, 0, DIRLENGTH);
	char rootDir[2] = "/";
	strcat(cDir, rootDir);

	string s;
	cout << ">>";
	cin >> s;

	while (s != "quit" && s != "exit") {
		if (s == "get") {
			char sGet[CMDBUFFER] = { 0 };
			cin >> sGet;
			get(sockfd, sGet);
		} else if (s == "put") {
			char sPut[CMDBUFFER] = { 0 };
			cin >> sPut;
			put(sockfd, sPut);
		} else if (s == "pwd") {
			char* currDir = new char;
			pwd(sockfd, currDir);
			if (currDir != NULL)
				cout << currDir << endl;
		} else if (s == "dir" || s == "ls") {
			dir(sockfd);
		} else if (s == "cd") {
			char sCd[CMDBUFFER];
			cin >> sCd;
			cd(sockfd, sCd);
		} else if (s == "?" || s == "help") {
			cout << "ftp client help" << endl;
			cout << "===================" << endl;
			cout << " put <filename>" << endl;
			cout << " get <filename>" << endl;
			cout << " cd <dirname>" << endl;
			cout << " dir || ls" << endl;
			cout << " help || ?" << endl;
			cout << " pwd" << endl;
			cout << " quit || exit" << endl;
			cout << "===================" << endl;
		} else if (s == "test") {
			char* currDir = new char;
			char* fathDir = new char;
			pwd(sockfd, currDir);
			cout << currDir << endl;
			getFathDir(currDir, fathDir);
			cout << fathDir << endl;
		} else {
			cout << s << ": command not found\n" << endl;
			cout << "ftp client help" << endl;
			cout << "===================" << endl;
			cout << " put <filename>" << endl;
			cout << " get <filename>" << endl;
			cout << " cd <dirname>" << endl;
			cout << " dir || ls" << endl;
			cout << " help || ?" << endl;
			cout << " pwd" << endl;
			cout << " quit || exit" << endl;
			cout << "===================" << endl;
		}
		cout << ">>";
		cin >> s;
	}
	close(sockfd);
	return 0;
}
