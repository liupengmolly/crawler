#ifndef _SOCKETMANAGER
#define _SOCKETMANAGER

#ifdef __cpluscplus
extern "C" {
#endif

#include <iostream>
#include <event.h>
#include "url.hpp"
using namespace std;

#ifndef _ARG
#define _ARG
typedef struct Arg {
	char ch[5000000];
	struct URL url;
    struct event* func;
}Arg;
#endif                                                                                                                                                                           

class SocketManager{
private:
	static SocketManager* sc;
	SocketManager();
	~SocketManager();
	class Garbo {  //它的唯一工作就是在析构函数中删除parser的实例
	public:
		~Garbo() {
			if (SocketManager::sc)
				delete SocketManager::sc;
		}
	};
	static Garbo garbo;
public:
	int pages_count;
    struct event_base *base;
    static SocketManager *getInstance();
    int createSocket(int port,Arg *arg);
    int sendHttpRequest(int sock,URL url);
    int recvHttpRespond(int sockfd,Arg *arg);
    int closeSocket(int sockfd);
	void pageCount(Arg *arg);
};

string combine_url(string pre,string cur);
void reptile_regex(string buf,char *pattern);
void on_read(int sock,short event,void *arg);
void on_send(int sock,short event,void *arg);

#ifdef __cpluscplus
};
#endif


#endif