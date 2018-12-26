#include "socketmanager.hpp"
#include "Message_Queue.cpp"
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
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <fstream>
#include <vector>
#include <fcntl.h>
#define BUF_SIZE 1024

extern Message_Queue<URL> *q;
extern vector<string> visited_q;
// extern unordered_map<string,size_t> url_id_pairs;
// extern set<ele> matrix_eles;
extern ofstream out;
extern threadpool thpool;
extern pthread_mutex_t fmtx;
extern pthread_mutex_t cmtx;
extern string tmp_links;
extern int ccount;
extern long int fixedAddr;
extern int sockets_num;
extern pthread_mutex_t smtx;

SocketManager::SocketManager() {
    base = event_base_new();
    event_base_dispatch(base);      
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

SocketManager* SocketManager::sc = NULL;
SocketManager::Garbo SocketManager::garbo;

string combine_url(string pre,string cur){
    int last_splash_pos,first_double_dot_pos;
    if((last_splash_pos = pre.find_last_of("/"))==-1)return cur;
    while(cur.substr(0,1) =="/")cur = cur.substr(1,cur.size());
    pre = pre.substr(0,last_splash_pos);

    while((first_double_dot_pos=cur.find(".."))==0){
        cur = cur.substr(2,cur.size());
        while(cur.substr(0,1)=="/"){
            cur = cur.substr(1,cur.size());
        }
        if((last_splash_pos=pre.find_last_of("/"))==-1){
            pre = "";
            break;
        }
        else pre = pre.substr(0,last_splash_pos);
    }
    string splash = "/";
    return pre+splash+cur;
}

void reptile_regex(regexPara* reg){
    string buf = reg->sh;
    string chost = reg->url.host;
    string cpagepath = reg->url.pagepath;
    string tmp_link=chost+cpagepath;

    regex img_regex(reg->pattern);
    struct hostent *host;

    auto words_begin = sregex_iterator(buf.begin(),buf.end(),img_regex);
    auto words_end = sregex_iterator();
    regex p1("[a-z0-9]{1,10}(\\.[a-z0-9]{1,10}){1,4}(?=/)");
    match_results<string::const_iterator> result;
    regex p2("http(s)?://[a-z0-9]{1,10}(\\.[a-z0-9]{1,10}){1,4}/");
    string t("");
    string t_splash("/");
    URL extracted_url;
    string match_str;
    unordered_set<string> sub_links;
    
    for(sregex_iterator i = words_begin;i!=words_end;++i){
        smatch match = *i;
        match_str = match.str();
        match_str = match_str.substr(6,match_str.size()-6);
        string mao = ":";

        regex filter("(javascript)|(\\.css)|(@)|(\\+)|(#)|(\\.jpg)|(\\?)");
        smatch m;
        if(regex_search(match_str,m,filter))continue;

        string::const_iterator start = match_str.begin();
        string::const_iterator end = match_str.end();
        regex_search(start,end,result,p1);    
        if(result.empty()){
            if(match_str.substr(0,4)=="http")continue;
            extracted_url.host = chost;
            extracted_url.pagepath = combine_url(cpagepath,match_str);
        }else{
            extracted_url.host = result[0].str();
            extracted_url.pagepath = regex_replace(match_str,p2,t_splash);
            if(extracted_url.pagepath == "")extracted_url.pagepath="/";
        }
        if(extracted_url.pagepath.find(mao)!=string::npos)continue;
        if(valid_host(extracted_url.host)==false)
            continue;

        //判断host是否有效
        host = gethostbyname(extracted_url.host.c_str());
        if(host!=NULL){
            struct in_addr serv_addr = *((struct in_addr *)host->h_addr);
            if(serv_addr.s_addr==fixedAddr){
                process_url(extracted_url);
                sub_links.insert("http://"+extracted_url.host+extracted_url.pagepath);
            }else{
                process_nhost(extracted_url.host);
            }
        }else{
            process_nhost(extracted_url.host);
        }
    }
    // pthread_mutex_lock(&fmtx);
    for(string sub_link:sub_links){
        tmp_link+=("|"+sub_link);
    }
    tmp_link+="\n";
    tmp_links += tmp_link;
    ccount++;
    if(ccount%3000==0){
        pthread_mutex_lock(&fmtx);
        out<<tmp_links;
        tmp_links = "";
        pthread_mutex_unlock(&fmtx);
    }
    pthread_mutex_lock(&smtx);
    sockets_num--;
    pthread_mutex_unlock(&smtx);
    // pthread_mutex_unlock(&fmtx);
    return ;
}

void on_read(int sock,short event,void *arg){
    Arg *argv = (Arg*)arg;
    SocketManager *sm = SocketManager::getInstance();
    int ret = sm->recvHttpRespond(sock,argv);
    if(ret == -1){
        process_nhost(argv->url.host);
        cout<<argv->url.host<<argv->url.pagepath<<endl;
    }else{
        string sh =(string)argv->ch;
        string state = sh.substr(9,3);
        if(state=="200"){
            visited_q.push_back(argv->url.host+argv->url.pagepath);
            regexPara *reg = new regexPara;
            reg->sh = sh;
            reg->pattern = "href=\"(http(s)?://[a-z0-9]{1,10}(\\.[a-z0-9]{1,10}){1,4}/)?.*?(?=\")";
            reg->url = argv->url;
            thpool_add_work(thpool, (void (*)(void*))reptile_regex,(void *)reg);
            sm->closeSocket(sock);
        }else{
        }
    }
    event_del(argv->func);
}

void on_send(int sock,short event,void *arg){
    struct event *read_ev = new struct event;
    Arg* argv = (Arg*)arg;
    SocketManager *sm = SocketManager::getInstance();
    int ret = sm->sendHttpRequest(sock,argv->url);
    event_del(argv->func);
    if(ret == -1)return;
    argv->func = read_ev;

    sm->pageCount(argv);
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
        return -1;
    }
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr = *((struct in_addr *)host->h_addr);
    servAddr.sin_port = htons(port);
    bzero(&(servAddr.sin_zero),8);
    if(servAddr.sin_addr.s_addr!=fixedAddr){
		cout<<servAddr.sin_addr.s_addr<<" "<<fixedAddr<<endl;
		return -1;
	}

    while(sockets_num>10000);

    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1){
        perror("socket 创建失败");
    }

    if(connect(sockfd,(struct sockaddr *)&servAddr,sizeof(struct sockaddr_in)) == -1){
        perror("connect 失败");
    }
    pthread_mutex_lock(&smtx);
    sockets_num++;
    pthread_mutex_unlock(&smtx);
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
        return -1;
    }
    sprintf(sendBuf,"GET %s HTTP/1.1 \r\nHOST: %s\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.100 Safari/537.36 \r\nConnection: Close\r\n\r\n",url.pagepath.c_str(),url.host.c_str());    
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
    if(pages_count%1000==0){
        cout<<("c p "+to_string(visited_q.size())+"/"+to_string(pages_count)+" "+to_string(q->get_nready()))<<" "<<sockets_num<<"\n";
    }
    return;
}
