#include "socketmanager.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <queue>
#include <regex>
#include <event.h>
#include <fcntl.h>
#define BUF_SIZE 1024

SocketManager::SocketManager() {
    base = event_base_new();
    event_base_dispatch(base);              //初始dispatch,没有这个就会出问题，没太懂，以后再看吧
}
SocketManager::~SocketManager() {
    event_base_free(base);
}

SocketManager* SocketManager::getInstance() {   //饿汉模式的单例
    if (sc == NULL) {
        sc = new SocketManager();
    }
    return sc;
};

SocketManager* SocketManager::sc = NULL;
SocketManager::Garbo SocketManager::garbo;

void reptile_regex(string buf,char *pattern){
    regex img_regex(pattern);

    cout << " = = = = = = = = = = = = = = = = = = = = = = = = ="<<endl;
    auto words_begin = sregex_iterator(buf.begin(),buf.end(),img_regex);
    auto words_end = sregex_iterator();
    regex p1("[a-zA-Z]{1,10}\.qq\.com");
    match_results<string::const_iterator> result;
    regex p2("http(s)?:\/\/[a-zA-Z]{1,10}\.qq\.com\/");
    string t("\/");
    string match_str;
    URL extracted_url;

    for(sregex_iterator i = words_begin;i!=words_end;++i){
        smatch match = *i;
        match_str = match.str();

        string::const_iterator start = match_str.begin();
        string::const_iterator end = match_str.end();
        regex_search(start,end,result,p1);    
        if(result.empty()){
            printf("host not match");
            return;
        }

        extracted_url.host = result[0];
        extracted_url.pagepath = regex_replace(match_str,p2,t);
        cout<<extracted_url.host<<" "<<extracted_url.pagepath<<"\t";
        process_url(extracted_url);
    }
    return ;
}

void on_read(int sock,short event,void *arg){
    Arg *argv = (Arg*)arg;
    SocketManager *sm = SocketManager::getInstance();
    sm->recvHttpRespond(sock,argv->ch);
    string sh =(string)argv->ch;
    char pattern[] = {"http(s)?://[a-z]*.qq.com/.*?(?=[(\">)|(\" )|(\'>)|(\' )])"};
    reptile_regex(sh,pattern);
    sm->closeSocket(sock);
    event_del(argv->func);
}

void on_send(int sock,short event,void *arg){
    struct event* read_ev;
    Arg* argv = (Arg*)arg;
    SocketManager *sm = SocketManager::getInstance();
    sm->sendHttpRequest(sock,argv->url);
    event_del(argv->func);
    argv->func = read_ev;

    event_set(read_ev,sock,EV_READ|EV_PERSIST,on_read,(void*)argv);
    event_base_set(sm->base,read_ev);
    event_add(read_ev,NULL);
}


int SocketManager::createSocket(int port,Arg *arg){
    struct event listen_ev;
    struct hostent *host;
    struct sockaddr_in servAddr;
    int sockfd;

    host = gethostbyname(arg->url.host.c_str());
    if(host == NULL){
        cout<<"dns 解析失败"<<endl;
        return -1;
    }
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr = *((struct in_addr *)host->h_addr);
    servAddr.sin_port = htons(port);
    bzero(&(servAddr.sin_zero),8);

    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1){
        perror("socket 创建失败");
    }

    if(connect(sockfd,(struct sockaddr *)&servAddr,sizeof(struct sockaddr_in)) == -1){
        perror("connect 失败");
    }
    
    arg->func = &listen_ev;
    event_set(&listen_ev,sockfd, EV_WRITE|EV_PERSIST,on_send,(void*)arg);
    event_base_set(base,&listen_ev);
    event_add(&listen_ev,NULL);
    
    event_base_dispatch(base);
    return sockfd;
}

void SocketManager::sendHttpRequest(int sockfd,URL url){
    int sendSize;
    char sendBuf[BUF_SIZE];
    sprintf(sendBuf,"GET %s HTTP/1.1 \r\nHOST: %s\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.100 Safari/537.36 \r\nConnection: Close\r\n\r\n",url.pagepath.c_str(),url.host.c_str());    
    if((sendSize = send(sockfd,sendBuf,BUF_SIZE,0)) == -1){
        perror("send 失败");
    }
    return ;
}

void SocketManager::recvHttpRespond(int sockfd,char *ch){
    char recvBuf[BUF_SIZE];
    int recvSize;
    memset(recvBuf,0,sizeof(recvBuf));
    memset(ch,0,5000000);
    int rec = recv(sockfd,recvBuf,BUF_SIZE,0);    
    while(rec>0){
        strcat(ch,recvBuf);
        memset(recvBuf,0,sizeof(recvBuf));
        rec = recv(sockfd,recvBuf,BUF_SIZE,0);
    }
    return;
}

int SocketManager::closeSocket(int sockfd) {
    close(sockfd);
    return 0;
}