/*
 * constants.h
 *
 *  Created on: Jun 4, 2009
 *      Author: Q
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define DEBUG_OUTPUT
#define DEBUG_MSG_HDR			" Debug output message \n"
#define DEBUG_MSG_TAIL			"\n End of debug output message \n"

#define BUFSIZE					1024
#define DIRSIZE					256
#define LOGINBUFSIZE			64
#define CMDSIZE					256
#define ARGSIZE					250

#define IP_ADDR					1
#define HOSTNAME_ADDR			2

#define DEFAULTHOST				"localhost"
#define DEFAULTUSER				"418"
#define DEFAULTPASS				"418A"
#define DEFAULTPORT				21

#define SOCKET_ERROR 			-101
#define GETHOSTBYNAME_ERROR 	-102
#define ADDRTYPE_ERROR			-103
#define CONNECT_ERROR			-104
#define RECV_ERROR				-105
#define SENDCMD_ERROR			-106
#define RECVRESP_ERROR			-107

#endif /* CONSTANTS_H_ */
