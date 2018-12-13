#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<string>
#include<queue>

#ifndef URL_HPP_
#define URL_HPP_
using namespace std;
typedef struct URL{
    string host;
    string pagepath;
}URL;

void bf_dataHash(const void *key,unsigned char *dataHash);
int bf_dataCheck(const void *key, unsigned char *dataHash);
unsigned int MurmurHash ( const void *key, int len, unsigned int seed);
void process_url(URL url);
#endif