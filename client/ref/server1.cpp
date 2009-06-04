#include<sys/socket.h>
#include<stdio.h>
#include<assert.h>
#include<iostream>
#include<cstring>
#include<cstdlib>
#include<sys/types.h>
#include<string>
#include<vector>
#include <netinet/in.h>
#include <netdb.h>
#include <termios.h>
#include <unistd.h>
#include<pthread.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<dirent.h>
#include <pwd.h>
#include<grp.h>
#include <sys/ioctl.h>
#include <fcntl.h> 
using namespace std;

void *thread(void *parameter);

const string serverhomedir = "/media/sda6";
const string clienthomedir = "";
string servercurrentdir = "/media/sda6";
string clientcurrentdir = "";
vector<string> IPs;

vector<string> split(string s, string delim)
{
	size_t last = 0;
	vector<string> ret;
	for(size_t i = 0; i + delim.size() <= s.size(); i++)
	{
		bool ok = true;
		for(size_t j = 0; j < delim.size() && ok; j++)
			ok = s[i + j] == delim[j];
		if(ok)
		{
			if(i - last) ret.push_back(s.substr(last, i - last));
			last = i + delim.size();
		}
	}
	if(last < s.size()) ret.push_back(s.substr(last));
	return ret;
}

int main()
{
	char recvBuff[1024];
	char command[1024];
	char data[1024];
	int ret;

	int sock = socket(AF_INET,SOCK_STREAM,0);									
	struct sockaddr_in ip;
	ip.sin_addr.s_addr = htonl(INADDR_ANY);
	ip.sin_family = AF_INET;
	ip.sin_port = htons(2100);
	int on = 1;
	if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on))<0)
	{
		exit(0);
	}
	if (bind(sock,(struct sockaddr*)&ip,sizeof(struct sockaddr)) == -1)
	{
		close(sock);
		cout<<"绑定失败!"<<endl;
		return 1;
	}
	listen(sock,10);
	struct sockaddr_in clientIp;
	socklen_t len = sizeof(sockaddr);
	while(true)
	{
		int server = accept(sock,(struct sockaddr*)&clientIp,&len);
		bool logged = false;
		for(int i=0;i<IPs.size();i++)
		{
			if(IPs[i] == inet_ntoa(clientIp.sin_addr))
			{
				logged = true;
				break;
			}
		}
		if(logged == true)
		{
			strcpy(command,"421\r\n");
			command[5] = '\0';
			send(server,command,strlen(command),0);
			cout<<command;
			continue;
		}
		else
		{
			strcpy(command,"220\r\n");
			command[5] = '\0';
			send(server,command,strlen(command),0);
			cout<<command;
		}
		memset(recvBuff,' ',1024);
		int total = recv(server,recvBuff,(int)strlen(recvBuff),0);
		recvBuff[total] = '\0';
		cout<<recvBuff;
		char * username = new char[strlen(recvBuff)-7+1];
		for(int i=0;i<strlen(recvBuff)-7;i++)
		{
			username[i] = recvBuff[i+5];
		}
		username[strlen(recvBuff)-7] = '\0';
		strcpy(command,"331\r\n");
		command[5] = '\0';
		send(server,command,strlen(command),0);
		cout<<command;
		memset(recvBuff,' ',1024);
		total = recv(server,recvBuff,(int)strlen(recvBuff),0);
		recvBuff[total] = '\0';
		cout<<recvBuff;
		char * password = new char[strlen(recvBuff)-7+1];
		for(int i=0;i<strlen(recvBuff)-7;i++)
		{
			password[i] = recvBuff[i+5];
		}
		password[strlen(recvBuff)-7] = '\0';
		if(strcmp(username,"stan") == 0 && strcmp(password,"iyaya") == 0)
		{
			strcpy(command,"230\r\n");
			command[5] = '\0';
			IPs.push_back(inet_ntoa(clientIp.sin_addr));
		}
		else
		{
			strcpy(command,"530\r\n");
			command[5] = '\0';
		}
		send(server,command,strlen(command),0);
		cout<<command;
		pthread_t id;
		ret = pthread_create(&id,NULL,thread,&server);
	}
}

void *thread(void *parameter)
{
	int *server = (int *)parameter;
	char recvBuff[1024];
	char command[1024];
	char data[1024];
	int total = 0;
	while(true)
	{
		memset(recvBuff,' ',1024);
		total = recv(*server,recvBuff,(int)strlen(recvBuff),0);
		recvBuff[total] = '\0';
		cout<<recvBuff;
		string strBuff = recvBuff;
		vector<string> subBuff = split(strBuff," ");
		if(subBuff[0] == "PASV\r\n")
		{
			int datasock = socket(AF_INET,SOCK_STREAM,0);									
			struct sockaddr_in datasockip;
			datasockip.sin_addr.s_addr = htonl(INADDR_ANY);
			datasockip.sin_family = AF_INET;
			datasockip.sin_port = htons(171*256+100);
			int dataon = 1;
			if(setsockopt(datasock,SOL_SOCKET,SO_REUSEADDR,&dataon,sizeof(dataon))<0)
			{
				exit(0);
			}
			if (bind(datasock,(struct sockaddr*)&datasockip,sizeof(struct sockaddr)) == -1)
			{
				close(datasock);
				cout<<"绑定失败!"<<endl;
			}
			struct sockaddr_in dataclientIp;
			socklen_t datalen = sizeof(sockaddr);
			memset(command,' ',1024);
			strcpy(command,"227 Entering Passive Mode (127,0,0,1,171,100)\r\n");
			send(*server,command,strlen(command),0);
			cout<<command;
			listen(datasock,10);
			int dataserver = accept(datasock,(struct sockaddr*)&dataclientIp,&datalen);
			memset(recvBuff,' ',1024);
			total = recv(*server,recvBuff,(int)strlen(recvBuff),0);
			recvBuff[total] = '\0';
			cout<<recvBuff;
			strBuff = recvBuff;
			subBuff = split(strBuff," ");
			if(subBuff[0] == "RETR")
			{
				for(int i = 2;i < subBuff.size();i++)
				{
					subBuff[1] += " ";
					subBuff[1] += subBuff[i];
				}
				struct stat st;
				string filename;
				if(subBuff[1][0] == '/')
				{
					filename = serverhomedir + subBuff[1];
				}
				else
				{
					if(servercurrentdir == "/media/sda6/" && clientcurrentdir == "/")
					{
						filename = servercurrentdir + subBuff[1];
					}
					else
					{
						filename = servercurrentdir + "/";
						filename = filename + subBuff[1];
					}
					
				}
				if(stat(filename.substr(0,filename.size()-2).c_str(),&st) != 0)
				{
					close(dataserver);
					close(datasock);
					strcpy(command,"500\r\n");
					command[5] = '\0';
					send(*server,command,strlen(command),0);
					cout<<command;
					continue;
				}
				if (S_ISDIR(st.st_mode))
				{
					close(dataserver);
					close(datasock);
					strcpy(command,"500\r\n");
					command[5] = '\0';
					send(*server,command,strlen(command),0);
					cout<<command;
					continue;
				}
				strcpy(command,"150\r\n");
				command[5] = '\0';
				send(*server,command,strlen(command),0);
				cout<<command;
				int from_fd;
				int byte_read;
				if((from_fd = open(filename.substr(0,filename.size()-2).c_str(),O_RDONLY)) == -1)
				{
					cout<<"open failed"<<endl;
					continue;
				}
				memset(data,' ',1024);
				byte_read = read(from_fd,data,strlen(data));
				if(byte_read < 1024)
				{
					data[byte_read] = '\0';
				}
				while(byte_read > 0)
				{
					send(dataserver,data,strlen(data),0);
					byte_read = read(from_fd,data,strlen(data));
					if(byte_read < 1024)
					{
						data[byte_read] = '\0';
					}
				}
				close(from_fd);
				close(dataserver);
				close(datasock);
				strcpy(command,"226\r\n");
				command[5] = '\0';
				send(*server,command,strlen(command),0);
				cout<<command;
			}
			else if(subBuff[0] == "STOR")
			{
				for(int i = 2;i < subBuff.size();i++)
				{
					subBuff[1] += " ";
					subBuff[1] += subBuff[i];
				}
				string filename;
				if(subBuff[1][0] == '/')
				{
					filename = serverhomedir + subBuff[1];
				}
				else
				{
					if(servercurrentdir == "/media/sda6/" && clientcurrentdir == "/")
					{
						filename = servercurrentdir + subBuff[1];
					}
					else
					{
						filename = servercurrentdir + "/";
						filename = filename + subBuff[1];
					}
					
				}
				int to_fd;
				if((to_fd = open(filename.substr(0,filename.size()-2).c_str(),O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR)) == -1)
				{
					cout<<"open failed"<<endl;
					close(to_fd);
					close(dataserver);
					close(datasock);
					continue;
				}
				strcpy(command,"150\r\n");
				command[5] = '\0';
				send(*server,command,strlen(command),0);
				cout<<command;
				memset(data,' ',1024);
				total = recv(dataserver,data,(int)strlen(data),0);
				if(total < 1024)
				data[total] = '\0';
				while(total > 0)
				{
					write(to_fd,data,total);
					memset(data,' ',1024);
					total = recv(dataserver,data,(int)strlen(data),0);
					if(total < 1024)
						data[total] = '\0';
				}
				close(to_fd);
				close(dataserver);
				close(datasock);
				strcpy(command,"226\r\n");
				command[5] = '\0';
				send(*server,command,strlen(command),0);
				cout<<command;
			}
			else if(subBuff[0] == "LIST\r\n")
			{
				DIR *dir;
				struct dirent *ptr;
				dir = opendir(servercurrentdir.c_str());
				string datastr = "";
				while((ptr = readdir(dir)) != NULL)
				{
					string name = servercurrentdir+"/"+ptr->d_name;
					struct stat stbuf;
					if(stat(name.c_str(),&stbuf) == 0)
					{
						char first[11] = "----------";
						if (S_ISDIR(stbuf.st_mode)) first[0] = 'd';
                                                if (stbuf.st_mode & S_IRUSR) first[1] = 'r';
                                                if (stbuf.st_mode & S_IWUSR) first[2] = 'w';
                                                if (stbuf.st_mode & S_IXUSR) first[3] = 'x';
                                                if (stbuf.st_mode & S_IRGRP) first[4] = 'r';
                                                if (stbuf.st_mode & S_IWGRP) first[5] = 'w';
                                                if (stbuf.st_mode & S_IXGRP) first[6] = 'x';
                                                if (stbuf.st_mode & S_IROTH) first[7] = 'r';
                                                if (stbuf.st_mode & S_IWOTH) first[8] = 'w';
                                                if (stbuf.st_mode & S_IXOTH) first[9] = 'x';
						datastr += first;
						datastr += "\t";
						char linknum[10];
						bzero(linknum,10);
						snprintf(linknum,9,"%d",stbuf.st_nlink);
						datastr += linknum;
						datastr += "\t";
						struct passwd *pw;
						pw = getpwuid(stbuf.st_uid);
						datastr += pw->pw_name;
						datastr += "\t";
						struct group *grp = getgrgid(stbuf.st_gid);
						datastr += grp->gr_name;
						datastr += "\t";
						char fdsize[20];
						bzero(fdsize,20);
						snprintf(fdsize,19,"%d",stbuf.st_size);
						datastr += fdsize;
						datastr += "\t";
						char fdtime[50];
						strcpy(fdtime,ctime(&stbuf.st_mtime));
						datastr += fdtime;
						datastr = datastr.substr(0,datastr.size()-1);
						datastr += "\t";
						datastr += ptr->d_name;
					}
					datastr += "\r\n";
				}
				send(dataserver,datastr.c_str(),strlen(datastr.c_str()),0);
			}
			close(dataserver);
			close(datasock);
			strcpy(command,"150\r\n");
			command[5] = '\0';
			send(*server,command,strlen(command),0);
			cout<<command;
			strcpy(command,"226\r\n");
			command[5] = '\0';
			send(*server,command,strlen(command),0);
			cout<<command;
		}		
		else if(subBuff[0] == "PWD\r\n")
		{
			memset(command,' ',1024);
			strcpy(command,"257 \"");
			if(clientcurrentdir.size() == 0)
				strcat(command,"/");
			else
				strcat(command,clientcurrentdir.c_str());
			strcat(command,"\" is current directory.\r\n\0");
			send(*server,command,strlen(command),0);
			cout<<command;
		}		
		else if(subBuff[0] == "CWD" || subBuff[0] == "CWD\r\n")
		{
			if(subBuff[0] == "CWD\r\n")
			{
				servercurrentdir = serverhomedir;
				clientcurrentdir = clienthomedir;
			}
			else
			{
				for(int i = 2;i < subBuff.size();i++)
				{
					subBuff[1] += " ";
					subBuff[1] += subBuff[i];
				}
				if(subBuff[1][0] == '/')
				{
					DIR *dir;
					dir = opendir((serverhomedir+subBuff[1].substr(0,subBuff[1].size()-2)).c_str());
					if(dir != NULL)
					{
						servercurrentdir = serverhomedir+subBuff[1].substr(0,subBuff[1].size()-2);
						clientcurrentdir = subBuff[1].substr(0,subBuff[1].size()-2);
						strcpy(command,"250\r\n");
						command[5] = '\0';
						send(*server,command,strlen(command),0);
						cout<<command;
					}
					else
					{
						strcpy(command,"500\r\n");
						command[5] = '\0';
						send(*server,command,strlen(command),0);
						cout<<command;
					}
				}
				else if(subBuff[1] == "..\r\n")
				{
					if((servercurrentdir == "/media/sda6" && clientcurrentdir == "") 
						|| (servercurrentdir == "/media/sda6/" && clientcurrentdir == "/"))
					{}
					else
					{
						int tempindex = clientcurrentdir.find_last_of('/');
						clientcurrentdir = clientcurrentdir.substr(0,tempindex);
						servercurrentdir = serverhomedir + clientcurrentdir;
					}
					strcpy(command,"250\r\n");
					command[5] = '\0';
					send(*server,command,strlen(command),0);
					cout<<command;
				}
				else
				{
					DIR *dir;
					string newserverdir;
					string newclientdir;
					if((servercurrentdir == "/media/sda6/" && clientcurrentdir == "/"))
					{
						newserverdir = servercurrentdir+subBuff[1].substr(0,subBuff[1].size()-2);
						newclientdir = clientcurrentdir+subBuff[1].substr(0,subBuff[1].size()-2);
					}
					else
					{
						newserverdir = servercurrentdir+"/"+subBuff[1].substr(0,subBuff[1].size()-2);
						newclientdir = clientcurrentdir+"/"+subBuff[1].substr(0,subBuff[1].size()-2);
					}
					dir = opendir(newserverdir.c_str());
					if(dir != NULL)
					{
						servercurrentdir = newserverdir;
						clientcurrentdir = newclientdir;
						strcpy(command,"250\r\n");
						command[5] = '\0';
						send(*server,command,strlen(command),0);
						cout<<command;
					}
					else
					{
						strcpy(command,"500\r\n");
						command[5] = '\0';
						send(*server,command,strlen(command),0);
						cout<<command;
					}
				}
			}
		}
		else if(subBuff[0] == "HELP\r\n")
		{
			strcpy(command,"214\r\n");
			command[5] = '\0';
			send(*server,command,strlen(command),0);
			cout<<command;
		}
		else if(subBuff[0] == "QUIT\r\n")
		{
			strcpy(command,"221\r\n");
			command[5] = '\0';
			send(*server,command,strlen(command),0);
			cout<<command;
			memset(recvBuff,' ',1024);
			total = recv(*server,recvBuff,(int)strlen(recvBuff),0);
			recvBuff[total] = '\0';
			cout<<recvBuff;
			string IP = recvBuff;
			vector<string>::iterator iter = IPs.begin();
			while((*iter) != IP)
			{
				iter++;
			}
			IPs.erase(iter);
			break;
		}
	}
	close(*server);
	return NULL;
}
