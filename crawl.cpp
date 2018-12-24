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
#include <regex>
#include <queue>
#include <unordered_map>
#include <set>
#include <fstream>
#include "bloomfilter.hpp"
#include "socketmanager.hpp"
#include "Message_Queue.cpp"
// #include "thpool.h"
using namespace std;

int page_count;
// unordered_map<string,size_t> url_id_pairs;
set<ele> matrix_eles;
threadpool thpool;
Message_Queue<URL> *q;
vector<string> visited_q;
unsigned char *dataHash;
unsigned char *nHostHash;
ofstream out;
BF host_bf;
BF url_bf;
pthread_mutex_t fmtx;
pthread_mutex_t smtx;
string tmp_links;
int ccount;
int servAddr;
int sockets_num;

int main(int argc, char* argv[]){
    Arg *arg = new Arg;
    arg->url.host = argv[1];
    arg->url.pagepath = "/";
    int PORT = std::atoi(argv[2]);
    page_count = 0;
    servAddr = 0;
    sockets_num = 0;

    host_bf = bf_create(VECTORSIZE);
    url_bf = bf_create(VECTORSIZE);

    ofstream result;
    result.open(argv[3]);
    if(!result.is_open())cout<<"url.txt not open"<<endl;
    out.open("tmp_link.txt");

    thpool = thpool_init(10);
    pthread_mutex_init(&fmtx,NULL);
    pthread_mutex_init(&smtx,NULL);
    tmp_links = "";
    ccount = 0;

    q = new Message_Queue<URL>;

    pthread_mutex_t mtx;
    pthread_mutex_init(&mtx,NULL);

    process_url(arg->url);

    int sockfd;
    int count = 0;
    int threads_num = 0;
    while(1){
        count += 1; 
        // if(count==1000)break;
        SocketManager *sm = SocketManager::getInstance();
        if(q->get_msg(arg->url)==0){
            sockfd = sm->createSocket(PORT,arg);
        }
        if(visited_q.size()>50000 && q->get_nready()==0)break;
    }
    cout<<"crawl end,processing links"<<endl;
    while(ccount<visited_q.size());
    out<<tmp_links;
    cout<<"process links end"<<endl;
    out.close();
    thpool_destroy(thpool);

    count = 0;
    unordered_map<string,size_t> url_id_pairs;
    for(string v_url:visited_q){
        result<<"http://"<<v_url<<" "<<++count<<endl;
        url_id_pairs["http://"+v_url] = count;
    }
    result<<endl;
    vector<ele> matrix_eles;
    ifstream in("tmp_link.txt");
    char *buffer = new char[10000];
    string buffer_string;
    while(in.getline(buffer,10000)){
        vector<string> sub_links;
        buffer_string = buffer;
        int pos1=0;
        int pos2=buffer_string.find("|",pos1);
        string main_url = "http://"+buffer_string.substr(pos1,pos2);
        string sub_url;
        int main_id = url_id_pairs[main_url];
        int sub_id;
        pos1 = pos2+1;
        while((pos2 = buffer_string.find("|",pos1))!=string::npos){
            sub_url = buffer_string.substr(pos1,pos2-pos1);
            sub_id = url_id_pairs[sub_url];
            // ele e;
            // e.row = main_id;
            // e.col = sub_id;
            // matrix_eles.push_back(e);
            if(sub_id!=0)result<<main_id<<" "<<sub_id<<endl;
            pos1 = pos2+1;
        }
        if(buffer_string.size()>pos1){
            sub_url = buffer_string.substr(pos1,buffer_string.size()-pos1);
            sub_id = url_id_pairs[sub_url];
            // ele e;
            // e.row = main_id;
            // e.col = sub_id;
            // matrix_eles.push_back(e)
            if(sub_id!=0)result<<main_id<<" "<<sub_id<<endl;
        }
    }
    result.close();
    // cout<<endl;
    // for(ele e:matrix_eles){
    //     cout<<e.row<<" "<<e.col<<endl;
    // }
    return 0;
}