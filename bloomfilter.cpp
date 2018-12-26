#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <queue>
#include <iostream>
#include <netdb.h>
#include <unordered_map>
#include "bloomfilter.hpp"
#include "Message_Queue.cpp"
#define BUF_SIZE 1024
using namespace std;
extern int page_count;
extern BF host_bf;
extern BF url_bf;
extern Message_Queue<URL> *q;
extern pthread_mutex_t tmtx;
bool connect_url(URL url){
    struct hostent *host;
    int sockfd;
    struct sockaddr_in servAddr;
    host = gethostbyname(url.host.c_str());
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr = *((struct in_addr *)host->h_addr);
    servAddr.sin_port = htons(80);
    bzero(&(servAddr.sin_zero),8);

    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(connect(sockfd,(struct sockaddr *)&servAddr,sizeof(struct sockaddr_in)) == -1) {
        perror("connect 失败");
    }
    char sendBuf[BUF_SIZE];
    int sendSize;
    //构建一个http请求
    sprintf(sendBuf,"GET %s HTTP/1.1 \r\nHost: %s \r\nConnection: Close \r\n\r\n",url.pagepath.c_str(),url.host.c_str());
    if((sendSize = send(sockfd,sendBuf,BUF_SIZE,0)) == -1) {
        perror("send 失败");
    }
    char recvBuf[BUF_SIZE];
    int recvSize;
    memset(recvBuf,0,sizeof(recvBuf));
    int rec = recv(sockfd,recvBuf,BUF_SIZE,0);    
    if(rec==-1){
        return rec;
    }
    close(sockfd);
    string sh = (string)recvBuf;
    string state = sh.substr(9,3);
    if(state=="200")return true;
    return false;

}

// extern unordered_map<string,size_t> url_id_pairs;
// extern ofstream *out;
//创建一个大小为size的空间，返回携带空间指针的bf
BF bf_create(unsigned int size)   //size=VECTORSIZE=479240000
{
	unsigned int i;
	unsigned int vsize;
	BF table;  //有一个长度为17类型为pbit的数组hash[GROUPNUM=17]
	vsize = size >> 3;  //无符号数右移3位 补零
	table.hash[0] = (pbit)malloc((sizeof(bit)*(vsize)));

	table.size = size;

	for (i = 0; i < vsize; i++)
	{
		(table.hash[0] + i)->a = 0;
		(table.hash[0] + i)->b = 0;
		(table.hash[0] + i)->c = 0;
		(table.hash[0] + i)->d = 0;
		(table.hash[0] + i)->e = 0;
		(table.hash[0] + i)->f = 0;
		(table.hash[0] + i)->g = 0;
		(table.hash[0] + i)->h = 0;
	}

	for (i = 1; i<GROUPNUM; i++)
		table.hash[i] = table.hash[i - 1] + (vsize / GROUPNUM);

	return table;
}

//将字符串line加入哈希表对应关键码位置中
void bf_add(BF bf, char * line)
{
	unsigned int i;
	unsigned int hashResult;
	for (i = 0; i < GROUPNUM; i++)
	{
		hashResult = MurmurHash2(line, strlen(line), i + 1) % (bf.size / GROUPNUM);
		*(bf.hash[i] + (hashResult >> 3)) = setBitNumber(*(bf.hash[i] + (hashResult >> 3)), 1, hashResult % 8 + 1);  //将bit中pos位的值设为num  bit setBitNumber(bit b, int num, int pos)
		
	}
}

//在bf中查找字符串line，存在返回1，否则返回0
int bf_search(BF bf, char line[])
{
	int i;
	unsigned int hashResult;
	int result = 1;

	for (i = 0; i < GROUPNUM; i++)
	{
		hashResult = MurmurHash2(line, strlen(line), i + 1) % (bf.size / GROUPNUM);

		if (getBitNumber(*(bf.hash[i] + (hashResult >> 3)), hashResult % 8 + 1) == 0)
		{
			//如果不匹配，退出
			result = 0;
			i = GROUPNUM;
		}
	}
	return result;
}

//超级哈希from google，参数seed确定具体哈希函数  hashResult = MurmurHash2(line, strlen(line), i + 1)
unsigned int MurmurHash2(const void * key, int len, unsigned int seed)
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.

	const unsigned int m = 0x5bd1e995; // 0101 1011 1101 0001 1110 1001 1001 0101
	const int r = 24;

	// Initialize the hash to a 'random' value

	unsigned int h = seed ^ len;  //二进制按位异或

								  // Mix 4 bytes at a time into the hash

	const unsigned char * data = (const unsigned char *)key;

	while (len >= 4)
	{
		unsigned int k = *(unsigned int *)data;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;
		len -= 4;
	}

	// Handle the last few bytes of the input array

	switch (len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
		h *= m;
	}

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

//获取bit结构中的pos位的值
int getBitNumber(bit b, int pos)
{
	int result;
	switch (pos)
	{
	case 1:result = b.a; break;
	case 2:result = b.b; break;
	case 3:result = b.c; break;
	case 4:result = b.d; break;
	case 5:result = b.e; break;
	case 6:result = b.f; break;
	case 7:result = b.g; break;
	case 8:result = b.h; break;
	default:result = 0;
	}
	return result;
}
//将bit中pos位的值设为num  bit setBitNumber(bit b, int num, int pos)
bit setBitNumber(bit b, int num, int pos)
{
	switch (pos)
	{
	case 1: b.a = num; break;
	case 2: b.b = num; break;
	case 3: b.c = num; break;
	case 4: b.d = num; break;
	case 5: b.e = num; break;
	case 6: b.f = num; break;
	case 7: b.g = num; break;
	case 8: b.h = num; break;
	default:break;
	}
	return b;
}

void process_url(URL url){
	char buf[1000];
	string full_url = url.host+url.pagepath;
	//处理index.html或者index.shtml的情况
	if(full_url.find("index.html")!=string::npos)full_url=full_url.substr(0,full_url.size()-11);
	if(full_url.find("index.shtml")!=string::npos)full_url=full_url.substr(0,full_url.size()-12);
	//处理结尾文件夹slash
	if(full_url.substr(full_url.size()-1,1)=="/")full_url = full_url.substr(0,full_url.size()-1);
	int len = full_url.copy(buf,full_url.length());
	buf[len]='\0';
	pthread_mutex_lock(&tmtx);
	if(bf_search(url_bf,buf)!=1){
		if(connect_url(url)){
			bf_add(url_bf,buf);
			q->push_msg(url);
			page_count++;
		}
	}
	pthread_mutex_unlock(&tmtx);
	return ;
}

bool valid_host(string host){
	char buf[1000];
	int len = host.copy(buf,host.length());
	buf[len]='\0';
	if(bf_search(host_bf,buf)!=1)return true;
	return false;
}

void process_nhost(string host){
	char buf[1000];
	int len = host.copy(buf,host.length());
	if(bf_search(host_bf,buf)!=1){
		bf_add(host_bf,buf);
	}
}

