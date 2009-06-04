/*
 * constants.h
 *
 *  Created on: Jun 4, 2009
 *      Author: Q
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define DEBUG_OUTPUT
#define DEBUG_HDR			" Debug output message \n"
#define DEBUG_TAIL			"\n End of debug output message \n"

#define BUFSIZE					8192
#define DBUFSIZE				8192
#define DIRSIZE					512
#define LOGINBUFSIZE			64
#define CMDSIZE					512
#define ARGSIZE					512
#define RESPSIZE				32767

#define IP_ADDR					1
#define HOSTNAME_ADDR			2

#define DEFAULTHOST				"59.66.131.167"
#define DEFAULTUSER				"user9"
#define DEFAULTPASS				"NewTerm"
#define DEFAULTPORT				21

#define SOCKET_ERROR 			-101
#define GETHOSTBYNAME_ERROR 	-102
#define ADDRTYPE_ERROR			-103
#define CONNECT_ERROR			-104
#define RECV_ERROR				-105
#define SENDCMD_ERROR			-106
#define RECVRESP_ERROR			-107

#endif /* CONSTANTS_H_ */
