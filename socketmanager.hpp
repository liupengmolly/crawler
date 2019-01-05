#ifndef _SOCKETMANAGER
#define _SOCKETMANAGER

#ifdef __cpluscplus
extern "C" {
#endif

#include <iostream>
#include <event.h>
#include "bloomfilter.hpp"
#include "ThreadPool.h"
using namespace std;

#ifndef _ARG
#define _ARG
typedef struct Arg {
	char ch[5000000];
	struct URL url;
    struct event* func;
}Arg;
typedef struct regex_para{
    string sh;
    string pattern;
    URL url;
}regexPara;
#endif                                                                                                                                                                           


class SocketManager{
private:
	static SocketManager* sc;
	SocketManager();
	~SocketManager();
	
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
void reptile_regex(regexPara* reg);
void on_read(int sock,short event,void *arg);
void on_send(int sock,short event,void *arg);

#ifdef __cpluscplus
};
#endif


#endif