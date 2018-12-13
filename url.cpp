#include<string.h>
#include<iostream>
#include<queue>
#include "url.hpp"
#ifndef LENGTH
#define LENGTH 50000
#endif
extern queue<URL> q;
extern queue<URL> visited_q;
extern unsigned char *dataHash;
int A[17] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59};

void bf_dataHash(const void *key,unsigned char *dataHash)
{
	const char *data = (const char *)key;
	for(int i = 0; i <= 16; i++)
	{
		unsigned int resultHash = MurmurHash(data,strlen(data),A[i]);
		*(dataHash + (i*LENGTH) + ((resultHash%(LENGTH<<3))>>3)) |= (1<<(resultHash%(LENGTH<<3)&0x7));
	}
	return ;
 }
 
 
int bf_dataCheck(const void *key, unsigned char *dataHash)
{
	const char *data = (const char *)key;
	for(int i = 0; i <= 16; i++)
	{
		unsigned int resultHash = MurmurHash(data,strlen(data),A[i]);
		if((*(dataHash + (i*LENGTH) + ((resultHash%(LENGTH<<3))>>3)) & (1<<(resultHash%(LENGTH<<3)&0x7)))==0)
		return 0;//wrong
	}
	return 1;//right 
}

unsigned int MurmurHash ( const void * key, int len, unsigned int seed )
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.

	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	// Initialize the hash to a 'random' value

	unsigned int h = seed ^ len;

	// Mix 4 bytes at a time into the hash

	const unsigned char * data = (const unsigned char *)key;

	while(len >= 4)
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

	switch(len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
	        h *= m;
	};

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
} 

void process_url(URL url){
	string full_url = url.host+url.pagepath;
	if(bf_dataCheck(full_url.c_str(),dataHash)!=1){
		bf_dataHash(full_url.c_str(),dataHash);
		q.push(url);
		cout<<q.size()<<endl;
	}
	else
		cout<<"duplicate"<<endl;
	return ;
}

