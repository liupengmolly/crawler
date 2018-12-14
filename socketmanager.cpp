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
#include <time.h>
#include <fcntl.h>
#define BUF_SIZE 1024

SocketManager::SocketManager() {
    base = event_base_new();
    event_base_dispatch(base);       //初始dispatch,没有这个就会出问题，没太懂，以后再看吧
    pages_count = 0;
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
extern string log;

SocketManager* SocketManager::sc = NULL;
SocketManager::Garbo SocketManager::garbo;

string combine_url(string pre,string cur){
    int last_splash_pos,first_double_dot_pos;
    if((last_splash_pos = pre.find_last_of('/'))==-1)return cur;
    if(cur.substr(0,1) =="/")cur = cur.substr(1,cur.size());
    pre = pre.substr(0,last_splash_pos);

    while((first_double_dot_pos=cur.find('..'))==0){
        cur = cur.substr(2,cur.size());
        while(cur.substr(0,1)=="/"){
            cur = cur.substr(1,cur.size());
        }
        if((last_splash_pos=pre.find_last_of('/'))==-1){
            pre = "";
            break;
        }
        else pre = pre.substr(0,last_splash_pos);
    }
    string splash = "/";
    return pre+splash+cur;
}

void reptile_regex(string buf,char *pattern,URL url){
    regex img_regex(pattern);
    struct hostent *host;

    auto words_begin = sregex_iterator(buf.begin(),buf.end(),img_regex);
    auto words_end = sregex_iterator();
    // regex p1("[a-z0-9]{1,10}(\.([a-z0-9]{1,10}))(\.([a-z0-9]{1,10}))(\.([a-z0-9]{1,10}))?");
    regex p1("[a-z0-9]{1,10}(\\.[a-z0-9]{1,10}){1,4}(?=/)");
    match_results<string::const_iterator> result;
    regex p2("http://[a-z0-9]{1,10}(\\.[a-z0-9]{1,10}){1,4}/");
    // regex p2("http://[a-z0-9]{1,10}(\.([a-z0-9]{1,10}))(\.([a-z0-9]{1,10}))(\.([a-z0-9]{1,10}))?/");
    string t("");
    string match_str;
    URL extracted_url;

    for(sregex_iterator i = words_begin;i!=words_end;++i){
        smatch match = *i;
        match_str = match.str().substr(6,match.str().size());
        // cout<<"match_str "<<match_str<<endl;
        string::const_iterator start = match_str.begin();
        string::const_iterator end = match_str.end();
        regex_search(start,end,result,p1);    
        if(result.empty()){
            if(match_str.substr(0,7)=="http://" || match_str.substr(0,8)=="https://")continue;
            extracted_url.host = url.host;
            extracted_url.pagepath = combine_url(url.pagepath,match_str);
        }else{
            // cout<<"not empty"<<endl;
            extracted_url.host = result[0].str();
            extracted_url.host = extracted_url.host.substr(0,extracted_url.host.size());
            extracted_url.pagepath = regex_replace(match_str,p2,t);
            if(extracted_url.pagepath == "")extracted_url.pagepath="/";
        }
        if(valid_host(extracted_url.host)==false){
            // string tmp = "host not valid, not add in queue\n";
            // log+=tmp;
            continue;
        }

        //判断host是否有效
        host = gethostbyname(extracted_url.host.c_str());
        if(host!=NULL){
            struct in_addr serv_addr = *((struct in_addr *)host->h_addr);
            if(serv_addr.s_addr==3528813578){
                process_url(extracted_url);
            }else{
                process_nhost(extracted_url.host);
                // log+="\n";
            }
        }else{
            process_nhost(extracted_url.host);
            // log+="\n";
        }
    }
    return ;
}

void on_read(int sock,short event,void *arg){
    Arg *argv = (Arg*)arg;
    SocketManager *sm = SocketManager::getInstance();
    int ret = sm->recvHttpRespond(sock,argv);
    if(ret == -1){
        process_nhost(argv->url.host);
        // string tmp = " can't get data,processed\n";
        // tmp = argv->url.host+tmp;
        // log+=tmp;
    }else{
        string sh =(string)argv->ch;
        char pattern[] = {"href=\"(http://[a-z0-9]{1,10}(\\.[a-z0-9]{1,10}){1,4}/)?(?!https)(?!css/).*?(?=\")"};
        reptile_regex(sh,pattern,argv->url);
        sm->closeSocket(sock);
    }
    event_del(argv->func);
}

void on_send(int sock,short event,void *arg){
    struct event *read_ev = new struct event;
    Arg* argv = (Arg*)arg;
    SocketManager *sm = SocketManager::getInstance();
    sm->pageCount(argv);
    int ret = sm->sendHttpRequest(sock,argv->url);
    event_del(argv->func);
    if(ret == -1)return;
    argv->func = read_ev;

    // struct event *time_ev = new struct event;

    // event_set(time_ev,sock,EV_TIMEOUT,on)

    event_set(read_ev,sock,EV_TIMEOUT|EV_READ|EV_PERSIST,on_read,(void*)argv);
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
        // string tmp = "dns 解析失败\n";
        // log+=tmp;
        return -1;
    }
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr = *((struct in_addr *)host->h_addr);
    servAddr.sin_port = htons(port);
    bzero(&(servAddr.sin_zero),8);
    if(servAddr.sin_addr.s_addr!=3528813578)return -1;
    // log+=(to_string(servAddr.sin_addr.s_addr)+" ");

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

int SocketManager::sendHttpRequest(int sockfd,URL url){
    int sendSize;
    char sendBuf[BUF_SIZE];
    if(valid_host(url.host)==false){
        // string tmp = "host not valid, not send request\n";
        // log+=tmp;
        return -1;
    }
    sprintf(sendBuf,"GET %s HTTP/1.1 \r\nHOST: %s\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.100 Safari/537.36 \r\nConnection: Close\r\n\r\n",url.pagepath.c_str(),url.host.c_str());    
    // struct timeval timeout={1,0};
    // int ret=setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
    // if(ret==-1){
        // cout<<"404"<<endl;
        // process_nhost(url.host);
        // return -1;
    // }
    if((sendSize = send(sockfd,sendBuf,BUF_SIZE,0)) == -1){
        perror("send 失败");
    }
    return 0;
}

int SocketManager::recvHttpRespond(int sockfd,Arg *arg){
    char recvBuf[BUF_SIZE];
    int recvSize;
    memset(recvBuf,0,sizeof(recvBuf));
    memset(arg->ch,0,5000000);
    int rec = recv(sockfd,recvBuf,BUF_SIZE,0);    
    if(rec==-1){
        return rec;
    }
    while(rec>0){
        strcat(arg->ch,recvBuf);
        memset(recvBuf,0,sizeof(recvBuf));
        rec = recv(sockfd,recvBuf,BUF_SIZE,0);
    }
    return 0;
}

int SocketManager::closeSocket(int sockfd) {
    close(sockfd);
    return 0;
}

void SocketManager::pageCount(Arg *arg){
    pages_count += 1;
    // string tmp = " = = = = = =  = = = = = = = = = = = = = = = = = = = \ncrawl ";
    // tmp+=to_string(pages_count);
    if(pages_count==4){
        ;
    }
    cout<<("c p "+to_string(pages_count)+" ");
    cout<<(arg->url.host+arg->url.pagepath+"\n");
    return;
}