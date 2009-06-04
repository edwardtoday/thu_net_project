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
extern int addr_type;
extern char* addr;
extern char* username;
extern char* password;
extern int port;

void debug_output(const string msg){
	cout<<DEBUG_MSG_HDR<<msg<<DEBUG_MSG_TAIL;
}

void err_handle(const int ERR_NO){
	//@TODO Complete the error handler output
	switch(ERR_NO){
	case SOCKET_ERROR:
		cout<<"Socket";
		break;
	case GETHOSTBYNAME_ERROR:
		cout<<"Get host by name";
		break;
	case ADDRTYPE_ERROR:
		cout<<"Address type";
		break;
	case CONNECT_ERROR:
		cout<<"Connect host";
		break;
	case RECV_ERROR:
		cout<<"Recv";
		break;
	default:
		break;
	}
	cout<<" error"<<endl;
	exit(ERR_NO);
}

int ftp_connect(const char* addr, const int addr_type, const int port){
	struct hostent *remote_host;
	struct sockaddr_in remote_addr;

	#ifdef DEBUG_OUTPUT
	cout<<DEBUG_MSG_HDR
		<<"ready to open socket. \t port = "<<port
		<<DEBUG_MSG_TAIL;
	#endif
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1)
		err_handle(SOCKET_ERROR);
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(port);
	#ifdef DEBUG_OUTPUT
	cout<<DEBUG_MSG_HDR
		<<"socket open.\t port = "<<ntohs(remote_addr.sin_port)
		<<DEBUG_MSG_TAIL;
	#endif

	#ifdef DEBUG_OUTPUT
	cout<<DEBUG_MSG_HDR
		<<"ready to get addr.\t addr = "<<addr<<"\t addr_type = "<<addr_type
		<<DEBUG_MSG_TAIL;
	#endif

	if(addr_type == IP_ADDR){
		inet_pton(AF_INET, addr, &remote_addr.sin_addr);
	} else if(addr_type == HOSTNAME_ADDR){
		if((remote_host = gethostbyname(addr)) == NULL)
			err_handle(GETHOSTBYNAME_ERROR);
		remote_addr.sin_addr = *((struct in_addr *)remote_host->h_addr);
	} else{
		err_handle(ADDRTYPE_ERROR);
	}

	#ifdef DEBUG_OUTPUT
	cout<<DEBUG_MSG_HDR
		<<"addr got.\t addr = "<<remote_addr.sin_addr.s_addr
		<<DEBUG_MSG_TAIL;
	#endif

	#ifdef DEBUG_OUTPUT
	cout<<DEBUG_MSG_HDR
		<<"ready to connect.\t sockfd = "<<sockfd<<"\t addr = "<<remote_addr.sin_addr.s_addr
		<<DEBUG_MSG_TAIL;
	#endif

	#ifdef DEBUG_OUTPUT
	int connect_ret = 0;
	#endif

	bzero(&(remote_addr.sin_zero), 8);
	if ((connect_ret = connect(sockfd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr))) == -1)
	        err_handle(CONNECT_ERROR);

	#ifdef DEBUG_OUTPUT
	cout<<DEBUG_MSG_HDR
		<<"connection established."
//		<<"connect = "<<connect_ret
		<<DEBUG_MSG_TAIL;
	#endif

	return 0;
}

bool ftp_send_cmd(const char* cmd){
	return (send(sockfd, cmd, strlen(cmd), 0) != -1);
}

void ftp_recv_response(){
	memset(buf, 0, BUFSIZE);
	if ((numbytes = recv(sockfd, buf, BUFSIZE, 0)) == -1)
		err_handle(RECV_ERROR);
	buf[numbytes] = '\0';
	printf("Recv: %s", buf);
}

int ftp_login(const char* user, const char* pass){
			ftp_recv_response();

		    char *cc = new char[LOGINBUFSIZE];
		    memset(cc, 0, LOGINBUFSIZE);
		    strcpy(cc, "USER ");
		    strcat(cc, user);
		    strcat(cc, "\r\n");

		    ftp_send_cmd(cc);

		    ftp_recv_response();

		    memset(cc, 0, LOGINBUFSIZE);
		    strcpy(cc, "PASS ");
		    strcat(cc, pass);
		    strcat(cc, "\r\n");

		    ftp_send_cmd(cc);

		    ftp_recv_response();

		    return 0;
}

int ftp_close(){
	ftp_send_cmd("QUIT");
	close(sockfd);
}

int ftp_get(char* filename){

}

int ftp_put(char* filename){

}

int ftp_pwd(char* dir){

}
int ftp_dir(){

}
int ftp_cd(char* dir){

}
