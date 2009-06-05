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

#define BUFSIZE					4096
#define DBUFSIZE				8192
#define DIRSIZE					512
#define LOGINBUFSIZE			64
#define CMDSIZE					512
#define ARGSIZE					512
#define RESPSIZE				81923

#define IP_ADDR					1
#define HOSTNAME_ADDR			2

#define DEFAULTHOST				"59.66.131.167"
#define DEFAULTUSER				"user9"
#define DEFAULTPASS				"NewTerm"
#define DEFAULTPORT				21

//#TODO define ftp response num, don't forget the server side
#define _TRANSTART			125
#define _FILESTATUSOK		150
#define _CMDOK				200
#define _SYSSTAT			211
#define _DIRSTAT			212
#define _FILESTAT			213
#define _HLPMSG				214
#define _SVCREADYFORNEWU	220
#define _CLOSECTRLCON		221
#define _DATACONOPEN		225
#define _DATACONCLOSE		226
#define _ENTPASVMODE		227
#define _LOGGEDIN			230
#define _FILEACTOK			250
#define _PATHNAMECREATED	257
#define _USERNAMEOK			331
#define _NEEDACCOUNT		332
#define _SERVNOTAVAIL		421
#define	_ERROPENDATACON		425
#define _TRANSABORTED		426
#define _FILEBUSY			450
#define _LOCALERROR			451
#define _INSUFSTORAGE		452
#define _CMDERROR			500
#define _ARGERROR			501
#define _CMDNOTIMPLEMENTED	502
#define _NOTLOGGEDIN		530
#define _FILEUNAVAIL		550
#define _PAGETYPEUNKNOWN	551
#define _EXCDSTORALLOC		552
#define _FILENAMEERROR		553

#define SOCKET_ERROR 			-101
#define GETHOSTBYNAME_ERROR 	-102
#define ADDRTYPE_ERROR			-103
#define CONNECT_ERROR			-104
#define RECV_ERROR				-105
#define SENDCMD_ERROR			-106
#define RECVRESP_ERROR			-107

#endif /* CONSTANTS_H_ */
