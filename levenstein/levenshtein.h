#ifndef LEVENSHTEIN_H
#define LEVEHSHTEIN_H

#define TILE_SIZE 16
#define TILES_PER_BLOCK 2
#define MAX_BLOCKS 8
#define WARP_MAX 32
//#define TESTCUDA
#define TESTING
#define TESTLENGTH 10000

#define MIN(a,b) (a-((a-b)&(b-a)>>31))
#define MAX(a,b) (a-((a-b)&(a-b)>>31))
#define ARRSIZE 1024
#define BLOCKSIZE 512
//#define INDEX(a,b)  (((((a<ARRSIZE)&&(a>=0))&&((b<ARRSIZE)&&(b>=0)))*0xffffffff)&((((a*(a+1))/2)+b)))
//#define INDEX(a,b)  (((((a<ARRSIZE)&&(a>=0))&&((b<ARRSIZE)&&(b>=0)))*0xffffffff)&(((((a+b)*(a+b+1))/2)+b+1)))

void levenshteinCuda(char* s1, char* s2, int* &result, size_t n);

int getIndex(int i,int j);

int getMin(int a, int b);

#endif
