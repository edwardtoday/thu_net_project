/*
 * client.cpp
 *
 *  Created on: Jun 4, 2009
 *      Author: Q
 */

#include <arpa/inet.h>
//#include <cstdlib>
//#include <cerrno>
//#include <cstring>
//#include <ctype.h>
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
#include "ftp_method.h"
#include "constants.h"
#include <bits/stringfwd.h>

using namespace std;

//extern int sockfd, numbytes;
//extern char buf[BUFSIZE];
//extern char cDir[DIRSIZE];
//extern int addr_type;
//extern char* addr;
//extern char* username;
//extern char* password;
//extern int port;

int sockfd = -1, numbytes = -1;
char buf[BUFSIZE] = { };
char cDir[DIRSIZE] = { };
char cmd[CMDSIZE] = { };
int addr_type = HOSTNAME_ADDR;
char *addr = DEFAULTHOST;
char *username = DEFAULTUSER;
char *password = DEFAULTPASS;
int port = DEFAULTPORT;
char *cmd_argu;

void usage(void) {
	//@TODO complete the usage message
	cout << "usage message here" << endl;
}
int get_cmd_argu(char *cmd_argu) {
	cin.getline(cmd_argu, CMDSIZE - 6);
#ifdef DEBUG_OUTPUT
	cout << DEBUG_MSG_HDR << "command argument is: " << cmd_argu
			<< DEBUG_MSG_TAIL;
#endif
	return (cmd_argu == NULL) ? 1 : 0;
}

void user_operation() {
	memset(cDir, 0, DIRSIZE);
	char rootDir[2] = "/";
	strcat(cDir, rootDir);

	string s;

	//@TODO 改成switch case
	do {
		cout << "ftp>";
		cin >> s;
		if (s == "get") {

		} else if (s == "put") {

		} else if (s == "pwd") {
			char* path = new char;
			ftp_pwd(path);
		} else if (s == "dir" || s == "ls") {

		} else if (s == "cd") {
			get_cmd_argu(cmd_argu);
			ftp_cd(cmd_argu);
		} else if (s == "quit" || s == "exit") {
			break;
		} else {
			usage();
		}
	} while (true);
}

int main(int argc, char * argv[]) {

	//get_login_info();
	ftp_connect(addr, addr_type, port);
	ftp_login(username, password);
	user_operation();
	ftp_close();
	return 0;
}
