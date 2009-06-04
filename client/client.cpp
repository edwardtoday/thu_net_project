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
char buf[BUFSIZE] = {};
char cDir[DIRSIZE] = {};
int addr_type = HOSTNAME_ADDR;
char* addr = DEFAULTHOST;
char* username = DEFAULTUSER;
char* password = DEFAULTPASS;
int port = DEFAULTPORT;


void usage(void){
	//@TODO complete the usage message
	cout<<"usage message here"<<endl;
}

void user_operation(){
	memset(cDir, 0, DIRSIZE);
	char rootDir[2] = "/";
	strcat(cDir, rootDir);

	string s;


	//@TODO 改成switch case
	do{
		cout << ">>";
		cin >> s;
		if (s == "get") {
			char sGet[CMDSIZE] = { 0 };
			cin >> sGet;
			ftp_get(sGet);
		} else if (s == "put") {
			char sPut[CMDSIZE] = { 0 };
			cin >> sPut;
			ftp_put(sPut);
		} else if (s == "pwd") {
			char* currDir = new char;
			ftp_pwd(currDir);
			if (currDir != NULL)
				cout << currDir << endl;
		} else if (s == "dir" || s == "ls") {
			ftp_dir();
		} else if (s == "cd") {
			char sCd[CMDSIZE];
			cin >> sCd;
			ftp_cd(sCd);
		} else {
			usage();
		}
	}while (s != "quit" && s != "exit");
}

int main(int argc, char * argv[]){

	//get_login_info();
	ftp_connect(addr, addr_type, port);
	ftp_login(username, password);
	user_operation();
	ftp_close();
	return 0;
}
