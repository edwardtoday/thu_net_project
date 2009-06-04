/***************************************************************************/
/*									   */
/* qftp.c - command line driven ftp file transfer program		   */
/* Copyright (C) 1996-2001 Thomas Pfau, pfau@eclipse.net		   */
/*	1407 Thomas Ave, North Brunswick, NJ, 08902			   */
/*									   */
/* This program is free software; you can redistribute it and/or    	   */
/* modify it under the terms of the GNU General Public License		   */
/* as published by the Free Software Foundation; either version 2	   */
/* of the License, or (at your option) any later version.		   */
/*		   							   */
/* This program is distributed in the hope that it will be useful,	   */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	   */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	   */
/* GNU General Public License for more details. 			   */
/*							   		   */
/* You should have received a copy of the GNU General Public License	   */
/* along with this progam; if not, write to the Free Software  		   */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA		   */
/* 02111-1307, USA.							   */
/*									   */
/***************************************************************************/

#if defined(__unix__) || defined(__VMS)
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#if defined(_WIN32)
#include <winsock.h>
#include <io.h>
#include "getopt.h"
#endif
#if defined(VAX)
#include "getopt.h"
#endif

#include "ftplib.h"

#if !defined(S_ISDIR)
#define S_ISDIR(m) ((m&S_IFMT) == S_IFDIR)
#endif

/* exit values */
#define EX_SYNTAX 2 	/* command syntax errors */
#define EX_NETDB 3	/* network database errors */
#define EX_CONNECT 4	/* network connect errors */
#define EX_LOGIN 5	/* remote login errors */
#define EX_REMCMD 6	/* remote command errors */
#define EX_SYSERR 7	/* system call errors */

#define FTP_SEND 1	/* send files */
#define FTP_GET 2	/* retreive files */
#define FTP_DIR 3	/* verbose directory */
#define FTP_RM 4	/* delete files */
#define FTP_LIST 5	/* terse directory */

#define DIRBUF_SIZE 1024 /* for wildcard processing */

static int logged_in = 0;
static char *host = NULL;
static char *user = NULL;
static char *pass = NULL;
static char mode = 'I';
static int action = 0;
static char *invocation;
static netbuf *conn = NULL;
static int wildcard = 0;

void usage(void)
{
    printf(
        "usage:  %s <host>\n"
        "\t[ -l user [ -p pass ] ]  defaults to anonymous/user@hostname\n"
        "\t[\n"
        "\t  [ -v level ]        debug level\n"
        "\t  [ -r rootpath ]     chdir path\n"
        "\t  [ -m umask ]        umask for created files\n"
        "\t  [ -a | -i ] ]       ascii/image transfer file\n"
	"\t  [ -w ]              toggle wildcard mode\n"
        "\t  [ file ]            file spec for directory or file to transfer\n"
        "\t]...\n\n"
        "If no files are specified on command line, the program\n"
        "will read file names from stdin.\n", invocation);
}

void ftp_connect(void)
{
    if (conn)
        return;
    if (host == NULL)
    {
	fprintf(stderr,"Host name not specified\n");
	usage();
	exit(EX_SYNTAX);
    }
    if (!logged_in)
    {
    	if (user == NULL)
    	{
	    user = "anonymous";
	    if (pass == NULL)
	    {
	    	char *u,h[64];
	    	u = getenv("USER");
	    	if (gethostname(h,64) < 0)
	    	{
		    perror("gethostname");
		    exit(EX_NETDB);
	    	}
	    	if ((u != NULL) && (h != NULL))
	    	{
		    static char xxx[64];
		    sprintf(xxx,"%s@%s",u,h);
		    pass = xxx;
	    	}
	    }
    	}
	else if (pass == NULL)
#if defined(_WIN32) || defined(VMS)
	    exit(EX_LOGIN);
#else
	    if ((pass = getpass("Password: ")) == NULL)
		exit(EX_SYSERR);
#endif
    	if (!FtpConnect(host,&conn))
    	{
	    fprintf(stderr,"Unable to connect to node %s\n%s",host,ftplib_lastresp);
	    exit(EX_CONNECT);
    	}
    	if (!FtpLogin(user,pass,conn))
    	{
	    fprintf(stderr,"Login failure\n%s",FtpLastResponse(conn));
	    exit(EX_LOGIN);
    	}
	logged_in++;
    }
}

void change_directory(char *root)
{
    ftp_connect();
    if (!FtpChdir(root, conn))
    {
	fprintf(stderr,"Change directory failed\n%s",FtpLastResponse(conn));
	exit(EX_REMCMD);
    }
}

struct REMFILE {
    struct REMFILE *next;
    int fsz;
    char *fnm;
};

static int log_progress(netbuf *ctl, int xfered, void *arg)
{
    struct REMFILE *f = (struct REMFILE *) arg;
    int pct = (xfered * 100) / f->fsz;
    printf("%s %3d%%\r", f->fnm, pct);
    fflush(stdout);
    return 1;
}

void process_file(char *fnm)
{
    int sts=0;
    int fsz;
    struct REMFILE *filelist = NULL;
    struct REMFILE rem;

    ftp_connect();
    FtpOptions(FTPLIB_CALLBACK, (long) NULL, conn);
    if ((action == FTP_SEND) || (action == FTP_GET))
    {
	if (action == FTP_SEND)
	{
	    struct stat info;
	    if (stat(fnm,&info) == -1)
	    {
	    	perror(fnm);
		return;
	    }
	    if (S_ISDIR(info.st_mode))
	    {
		if (!ftpMkdir(fnm))
		    fprintf(stderr,"mkdir %s failed\n%s",fnm,FtpLastResponse(conn));
		else
		    if (ftplib_debug)
			fprintf(stderr,"Directory %s created\n",fnm);
		return;
	    }
            fsz = info.st_size;
	}
        else
        {
	    if (!wildcard)
	    {
		struct REMFILE *f;
		f = (struct REMFILE *) malloc(sizeof(struct REMFILE));
		memset(f,0,sizeof(struct REMFILE));
		f->next = filelist;
		filelist = f;
		f->fnm = strdup(fnm);
	    } else {
		netbuf *dir;
		char *buf;
		if (!FtpAccess(fnm, FTPLIB_DIR, FTPLIB_ASCII, conn, &dir))
		{
		    fprintf(stderr,"error requesting directory of %s\n%s\n",
			    fnm, FtpLastResponse(conn));
		    return;
		}
		buf = malloc(DIRBUF_SIZE);
		while (FtpRead(buf, DIRBUF_SIZE, dir) > 0)
		{
		    struct REMFILE *f;
		    char *p;
		    f = (struct REMFILE *) malloc(sizeof(struct REMFILE));
		    memset(f,0,sizeof(struct REMFILE));
		    f->next = filelist;
		    p = strchr(buf,'\n');
		    if (p)
			*p = '\0';
		    f->fnm = strdup(buf);
		    filelist = f;
		}
		free(buf);
		FtpClose(dir);
	    }
        }
    }
    switch (action)
    {
      case FTP_DIR :
	sts = FtpDir(NULL, fnm, conn);
	break;
      case FTP_LIST :
	sts = FtpNlst(NULL, fnm, conn);
	break;
      case FTP_SEND :
	rem.next = NULL;
	rem.fnm = fnm;
	rem.fsz = fsz;
	fsz /= 10;
	if (fsz > 100000)
	    fsz = 100000;
        if (ftplib_debug && fsz)
        {
            FtpOptions(FTPLIB_CALLBACK, (long) log_progress, conn);
            FtpOptions(FTPLIB_IDLETIME, (long) 1000, conn);
            FtpOptions(FTPLIB_CALLBACKARG, (long) &rem, conn);
            FtpOptions(FTPLIB_CALLBACKBYTES, (long) fsz, conn);
        }
	sts = FtpPut(fnm,fnm,mode,conn);
	if (ftplib_debug && sts)
	    printf("%s sent\n",fnm);
	break;
      case FTP_GET :
	while (filelist)
	{
	    struct REMFILE *f = filelist;
	    filelist = f->next;
	    if (!FtpSize(f->fnm, &fsz, mode, conn))
		fsz = 0;
	    f->fsz = fsz;
	    fsz /= 10;
	    if (fsz > 100000)
		fsz = 100000;
	    if (ftplib_debug && fsz)
	    {
		FtpOptions(FTPLIB_CALLBACK, (long) log_progress, conn);
		FtpOptions(FTPLIB_IDLETIME, (long) 1000, conn);
		FtpOptions(FTPLIB_CALLBACKARG, (long) f, conn);
		FtpOptions(FTPLIB_CALLBACKBYTES, (long) fsz, conn);
	    }
	    sts = FtpGet(f->fnm,f->fnm,mode,conn);
	    if (ftplib_debug && sts)
		printf("%s retrieved\n",f->fnm);
	    free(f->fnm);
	    free(f);
	}
	break;
      case FTP_RM :
	while (filelist)
	{
	    struct REMFILE *f = filelist;
	    filelist = f->next;
	    sts = FtpDelete(f->fnm,conn);
	    if (ftplib_debug && sts)
		printf("%s deleted\n", f->fnm);
	    free(f->fnm);
	    free(f);
	}
	break;
    }
    if (!sts)
	printf("ftp error\n%s\n",FtpLastResponse(conn));
    return;
}

void set_umask(char *m)
{
    char buf[80];
    sprintf(buf,"umask %s", m);
    ftp_connect();
    FtpSite(buf, conn);
}

int main(int argc, char *argv[])
{
    int files_processed = 0;
    int opt;

    invocation = argv[0];
    optind = 1;
    if (strstr(argv[0],"send") != NULL)
	action = FTP_SEND;
    else if (strstr(argv[0],"get") != NULL)
	action = FTP_GET;
    else if (strstr(argv[0],"dir") != NULL)
	action = FTP_DIR;
    else if (strstr(argv[0],"list") != NULL)
	action = FTP_LIST;
    else if (strstr(argv[0],"rm") != NULL)
	action = FTP_RM;
    if ((action == 0) && (argc > 2))
    {
	if (strcmp(argv[1],"send") == 0)
	    action = FTP_SEND;
    	else if (strcmp(argv[1],"get") == 0)
	    action = FTP_GET;
    	else if (strcmp(argv[1],"dir") == 0)
	    action = FTP_DIR;
	else if (strcmp(argv[1],"list") == 0)
	    action = FTP_LIST;
    	else if (strcmp(argv[1],"rm") == 0)
	    action = FTP_RM;
	if (action)
	    optind++;
    }
    if (action == 0)
    {
	usage();
	exit(EX_SYNTAX);
    }

    FtpInit();

    while (argv[optind] != NULL)
    {
	if (argv[optind][0] != '-')
	{
	    if (host == NULL)
		host = argv[optind++];
	    else
	    {
		files_processed++;
		process_file(argv[optind++]);
	    }
	    continue;
	}
	opt = getopt(argc,argv,"ail:m:p:r:v:w");
	switch (opt)
	{
	  case '?' :
	    usage();
	    exit(EX_SYNTAX);
	  case ':' :
	    usage();
	    exit(EX_SYNTAX);
	  case 'a' : mode = 'A'; break;
	  case 'i' : mode = 'I'; break;
	  case 'l' : user = optarg; break;
	  case 'm' : set_umask(optarg); break;
	  case 'p' : pass = optarg; break;
	  case 'r' : change_directory(optarg); break;
	  case 'v' :
	    if (opt == ':')
		ftplib_debug++;
	    else
		ftplib_debug = atoi(optarg);
	    break;
	  case 'w' : wildcard = !wildcard; break;
	  default :
	    usage();
	    exit(EX_SYNTAX);
	}
    }

    if (files_processed == 0)
    {
	ftp_connect();
	if ((action == FTP_DIR) || (action == FTP_LIST))
	    process_file(NULL);
	else
	{
	    char fnm[256];
	    do
	    {
	        char *nl;
		if (isatty(fileno(stdin)))
		    printf("file> ");
		if (fgets(fnm, sizeof(fnm), stdin) == NULL)
		    break;
		if ((nl = strchr(fnm,'\n')) != NULL)
		    *nl = '\0';
		process_file(fnm);
	    }
	    while (1);
	}
    }
    if (conn)
	FtpClose(conn);
    return 0;
}
