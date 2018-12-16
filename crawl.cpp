#include <iostream>
#include <string.h>
#include <string>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <event.h>
#include <queue>
#include "socketmanager.hpp"
using namespace std;
#define PORT 80
#ifndef LENGTH
#define LENGTH 16000000
#endif
queue<URL> q;
queue<URL> visited_q;
unsigned char *dataHash;
unsigned char *nHostHash;
string log;

int main(){

    Arg *arg = new Arg;
    arg->url.host = "www.bnu.edu.cn";
    arg->url.pagepath = "/";
    
	dataHash = (unsigned char *)malloc(LENGTH*17*sizeof(unsigned char));
	memset(dataHash,0,LENGTH*17*sizeof(unsigned char));

	nHostHash = (unsigned char *)malloc(LENGTH*17*sizeof(unsigned char));
	memset(nHostHash,0,LENGTH*17*sizeof(unsigned char));

    SocketManager *sm = SocketManager::getInstance();
    int sockfd = sm->createSocket(PORT,arg);
    int count = 0;
    while(!q.empty()){
        count += 1;
        arg->url = q.front();
        q.pop();
        SocketManager *sm = SocketManager::getInstance();
        sockfd = sm->createSocket(PORT,arg);
    }
    return 0;
}