/*
 * ftp_method.h
 *
 *  Created on: Jun 4, 2009
 *      Author: Q
 */

#ifndef FTP_METHOD_H_
#define FTP_METHOD_H_

#include <cstring>
#include "constants.h"
#include <bits/stringfwd.h>
using namespace std;

//int sockfd = -1, numbytes = -1;
//char buf[BUFSIZE] = {};
//char cDir[DIRSIZE] = {};
//int addr_type = HOSTNAME_ADDR;
//char* addr = DEFAULTHOST;
//char* username = DEFAULTUSER;
//char* password = DEFAULTPASS;
//int port = DEFAULTPORT;

int ftp_connect(const char *addr, const int addr_type, const int port);
int ftp_login(const char* user, const char* pass);
int ftp_close();
int ftp_get(char *filename);
int ftp_put(char *filename);
int ftp_pwd(char *path);
int ftp_dir();
int ftp_cd();

#endif /* FTP_METHOD_H_ */
