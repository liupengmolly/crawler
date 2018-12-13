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
#define LENGTH 50000
#endif
queue<URL> q;
queue<URL> visited_q;
unsigned char *dataHash;


int main(){
    // string str = "../xxgk/xsyg/index.html";
    // int pos = str.find_last_of('/');
    // cout<<pos<<endl;
    // cout<<str.substr(0,pos)<<endl;
    // cout<<str.find('..');

    Arg *arg = new Arg;
    arg->url.host = "www.bnu.edu.cn";
    arg->url.pagepath = "/";
    
	dataHash = (unsigned char *)malloc(LENGTH*17*sizeof(unsigned char));
	memset(dataHash,0,LENGTH*17*sizeof(unsigned char));

    SocketManager *sm = SocketManager::getInstance();
    int sockfd = sm->createSocket(PORT,arg);
    // sendHttpRequest(sockfd,url);
    // recvHttpRespond(sockfd,arg->ch);
    // sh = string(arg_ch);
    // char pattern[] = {"http(s)?://[a-z]*.qq.com/.*?(?=[(\">)|(\" )|(\'>)|(\' )])"};
    // reptile_regex(sh,pattern);
    // close(sockfd);
    while(!q.empty()){
        arg->url = q.front();
        q.pop();
        SocketManager *sm = SocketManager::getInstance();
        sockfd = sm->createSocket(PORT,arg);
        // if(sockfd!=-1){
        //     sendHttpRequest(sockfd,url);
        //     recvHttpRespond(sockfd,ch);
        //     sh = string(ch);
        //     reptile_regex(sh,pattern);
        //     close(sockfd); 
        // }
    }
    return 0;
}