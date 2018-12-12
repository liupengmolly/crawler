#include <iostream>
#include <string.h>
#include <string>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <event.h>
#include <regex>
#include <unistd.h>
#include <queue>
using namespace std;
#define PORT 80
#define BUF_SIZE 512


typedef struct Url{
    string host;
    string pagepath;
}Url;

queue<Url> q;
queue<Url> visited_q;

int createSocket(Url url, int port){
    struct sockaddr_in servAddr;
    struct hostent *host;
    int sockfd;
    const char *hostname = url.host.c_str();
    host = gethostbyname(hostname);
    if(host == NULL){
        perror("dns 解析失败");
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
    return sockfd;
}

void sendHttpRequest(int sockfd,Url url){
    char sendBuf[BUF_SIZE];
    int sendSize;
    sprintf(sendBuf,"GET %s HTTP/1.1 \r\nHOST: %s\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/61.0.3163.100 Safari/537.36 \r\nConnection: Close\r\n\r\n",url.pagepath.c_str(),url.host.c_str());    
    if((sendSize = send(sockfd,sendBuf,BUF_SIZE,0)) == -1){
        perror("send 失败");
    }
    return ;
}

void recvHttpRespond(int sockfd,char *ch){
    char recvBuf[BUF_SIZE];
    int recvSize;
    memset(recvBuf,0,sizeof(recvBuf));
    memset(ch,0,1000000);
    int count = 0;
    // cout<<"start receiving...\n";
    int rec = recv(sockfd,recvBuf,BUF_SIZE,0);    
    while(rec>0){
        count++;
        strcat(ch,recvBuf);
        memset(recvBuf,0,sizeof(recvBuf));
        rec = recv(sockfd,recvBuf,BUF_SIZE,0);
        // printf("in loop");x
    }
    // printf("end");
    return;
}

void reptile_regex(string buf,char *pattern){
    regex img_regex(pattern);

    cout << " = = = = = = = = = = = = = = = = = = = = = = = = ="<<endl;
    auto words_begin = sregex_iterator(buf.begin(),buf.end(),img_regex);
    auto words_end = sregex_iterator();

    for(sregex_iterator i = words_begin;i!=words_end;++i){
        smatch match = *i;
        string match_str = match.str();
        match_str = match_str.substr(2,match_str.size()-1);

        string::const_iterator start = match_str.begin();
        string::const_iterator end = match_str.end();
        regex p1("[a-zA-Z]{1,10}\.qq\.com");
        match_results<string::const_iterator> result;
        regex_search(start,end,result,p1);    
        if(result.empty()){
            printf("host not match");
            return;
        }

        regex p2("http(s)?:\/\/[a-zA-Z]{1,10}\.qq\.com\/");
        string t("\/");
        match_str = regex_replace(match_str,p2,t);

        Url url;
        url.host = result[0];
        url.pagepath = match_str;
        cout<<result[0]<<endl;
        cout<<match_str<<endl;
        q.push(url);
    }
    return ;
}

int main(){
    Url url;
    url.host = "sports.qq.com";
    url.pagepath = "/nbavideo/";
    char ch[1000000];
    int sockfd = createSocket(url,PORT);
    sendHttpRequest(sockfd,url);
    recvHttpRespond(sockfd,ch);
    string sh = string(ch);
    char pattern[] = {"http(s)?://[a-z]*.qq.com/.*?(?=[(\">)|(\" )|(\'>)|(\' )])"};
    reptile_regex(sh,pattern);
    close(sockfd);

    while(!q.empty()){
        url = q.front();
        q.pop();
        sockfd = createSocket(url,PORT);
        sendHttpRequest(sockfd,url);
        recvHttpRespond(sockfd,ch);
        string sh = string(ch);
        reptile_regex(sh,pattern);
        close(sockfd); 
    }
    // string s = "http://www.qq.com/TR/xhtml1/DTD/xhtml1-transitional.dtd";
    // string::const_iterator start = s.begin();
    // string::const_iterator end = s.end();
    // regex p("http(s)?:\/\/[a-zA-Z]{1,10}\.qq\.com\/");
    // match_results<string::const_iterator> result;
    // regex_search(start,end,result,p);    
    // cout<<result[0]<<endl;
    return 0;
}