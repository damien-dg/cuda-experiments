#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <cuda_runtime.h>
#include "levenshtein.h"
#include <math.h>

#define HORIZONTAL 0
#define VERTICAL 1
#define DIAGONAL 2
#define UNDEFINED 255


using namespace std;

int           **dist;
int           *retiled;
FILE* outfileSequential;
typedef struct alignm_struct {
	char            seqelem[2];
	int             type;
	int             cost;
}               alignm_struct;



int min3(int a, int b, int c);
void alloc_dist_matrix(int m, int n);
void destroy_dist_matrix(int m, int n);
int determine_alignment(char *s, int m, char *t, int n);
//This function implements a heuristic to determine an alignment
// from the edit distance matrix
//
int __determine_alignment(int i, int j, alignm_struct * alignment);

int LevenshteinDistance(char *s, int m, char *t, int n);

int retile(char* &c, int n);
void parallelLevenshtein(char* s1, char* s2, int* &result, int size);

int tiledIndex(int i , int j, int n)
{
	int rval;
	if(!(i >= 0 && i <=n && j >= 0 && j <=n) ) {
		rval = 0;
	} else if((i+j) <= n) {
		rval = (((i+j)*(i+j+1))/2) + j;
	} else {
		rval = ((n+1)*(n+1)) - (((2*(n+1) - (i+j))*(2*(n+1) - (i+j+1)))/2) +
			(j - ((j+i) - (n+1))) - 1;
	}
	return rval;
}	

int main( int argc, char** argv ) {
    //cudaSetDevice(1);
    int             dist;
    char           *s;
    char           *t;
    int             m;
    int             n;
    int             i;
    char*           s1 = new char[1024];
    char*           s2 = new char[1024];
    int*           parallelDist;
    retiled = new int[(ARRSIZE +1) * (ARRSIZE + 1)];

    s = s1;
    t = s2;

    memset(s1, 0, sizeof(char) * ARRSIZE);
    memset(s2, 0, sizeof(char) * ARRSIZE);

    if( argc < 3) {
        fprintf(stderr, "Not enough arguments\nUsage: levenstein <infile1> <infile2>\n");
        exit(EXIT_FAILURE);
    }

    FILE* infile = fopen(argv[1], "r");
    FILE* modfile = fopen(argv[2], "r");
    FILE* outfileParallel = fopen("out.parallel", "w");
    outfileSequential = fopen("out.sequential", "w");

    size_t j = ARRSIZE;
    size_t k = ARRSIZE;

    getline(&s1, &j, (FILE*) infile);
    getline(&s2, &k, (FILE*) modfile);

    m = ARRSIZE;
    n = ARRSIZE;

#ifndef CUDA_NO_SM_11_ATOMIC_INTRINSICS
    printf("WARNING! Not using atomics!\n");
#endif

    parallelDist = new int[(ARRSIZE+1)*(ARRSIZE+1)];
    memset(parallelDist,0,sizeof(int)*(ARRSIZE+1)*(ARRSIZE+1));

    levenshteinCuda(s2,s1, parallelDist,ARRSIZE);

    for(int row = 0; row <= ARRSIZE;++row) {
        if( row > 5 )
            continue;
            printf("\nRow %.4d\n", row);
        //fprintf(outfileParallel, "Row %.4d\n", row);
        for(int col = 0; col <= ARRSIZE;++col) {
            printf("%.4d\t", parallelDist[tiledIndex(row,col,ARRSIZE)]);
            fprintf(outfileParallel,"[%d][%d] = %.4d\n", row, col, parallelDist[tiledIndex(row,col,ARRSIZE)]);
        }

            printf("\n");
        
    }
    printf("Parallel[%d][%d] = %d\n", ARRSIZE,ARRSIZE, parallelDist[tiledIndex(ARRSIZE,ARRSIZE,ARRSIZE)]);

    alloc_dist_matrix(m, n);
    dist = LevenshteinDistance(s, m, t, n);
   
    /*printf("Edit Distance of %s and %s = %d\n\n",
	       s, t, dist);

    determine_alignment(s, m, t, n);


    printf("\n\nSequence Similarity based on [ \"An Information-Theoretic Definition of Similarity\", Dekang Lin, Department of Computer Science, University of Manitoba]:\n\n");
    printf("Similariy = sim(x,y) = 1/( 1 + editDist(x,y)) = %f\n", 1.0/(1.0 + (float)dist));

    destroy_dist_matrix(m, n);*/

    
    return EXIT_SUCCESS;
}

int
min3(int a, int b, int c)
{
	int             min, temp;

	min = MIN(MIN(a, b), MIN(b, c));

	return min;
}

void
alloc_dist_matrix(int m, int n)
{
	int             i;
	dist = (int **) malloc(sizeof(int *) * (m + 1));

	for (i = 0; i < m + 1; i++)
		dist[i] = (int *) malloc(sizeof(int) * (n + 1));
}

void
destroy_dist_matrix(int m, int n)
{
	int             i;
	for (i = 0; i < m + 1; i++)
		free(dist[i]);
	free(dist);
}

int
determine_alignment(char *s, int m, char *t, int n)
{
	alignm_struct   alignment[m + n];
	int             i, j, k;

	for (i = 0; i < m + n; i++)
		alignment[i].type = UNDEFINED;

	__determine_alignment(m, n, alignment);



	i = 0;
	j = 0;
	for (k = 0; k < m + n; k++) {
		if (alignment[k].type == UNDEFINED)
			break;

		if (alignment[k].type == HORIZONTAL) {
			alignment[k].seqelem[0] = '-';
			alignment[k].seqelem[1] = t[j++];
		} else if (alignment[k].type == VERTICAL) {
			alignment[k].seqelem[0] = s[i++];
			alignment[k].seqelem[1] = '-';
		} else if (alignment[k].type == DIAGONAL) {
			alignment[k].seqelem[0] = s[i++];
			alignment[k].seqelem[1] = t[j++];
		}
	}

	/*printf("Alignment Matrix: \n");

	for(k = 0; alignment[k].type != UNDEFINED && k < m + n; k++){
		printf("%c\t", alignment[k].seqelem[0]);
	}
	printf("\n");
	for(k = 0; alignment[k].type != UNDEFINED && k < m + n; k++){
		printf("%c\t", alignment[k].seqelem[1]);
	}
	printf("\n");*/
}
//This function implements a heuristic to determine an alignment
// from the edit distance matrix
//
int
__determine_alignment(int i, int j, alignm_struct * alignment)
{
	int             vert, horiz, diag;
	int             min, cost, which;
	int             alignm_elem;


	if ((i <= 0) && (j <= 0)) {
		return 0;
	}
	if ((i > 0) && (j > 0)) {
		horiz = dist[i][j - 1];
		vert = dist[i - 1][j];
		diag = dist[i - 1][j - 1];
		min = min3(horiz, vert, diag);
	} else if (i > 0) { // Boundary of the Edit Distance Matrix
		horiz = -1;
		vert = dist[i - 1][j];
		diag = -1;
		min = vert;
	} else if (j > 0) { // Boundary of the Edit Distance Matrix
		horiz = dist[i][j - 1];
		vert = -1;
		diag = -1;
		min = horiz;
	}


	if ( (min == diag ) && (i > 0) && (j > 0)) {
		if (dist[i - 1][j - 1] != dist[i][j]) {
                        cost = 1;
                        which = DIAGONAL;
                        j--;
                        i--;
		} else {
			cost = 0;
			which = DIAGONAL;
			j--;
			i--;
		}
	} else if (min == horiz) {
		which = HORIZONTAL;
		if (dist[i][j - 1] != dist[i][j])
			cost = 1;
		else
			cost = 0;
		j--;
	} else if (min == vert) {
		which = VERTICAL;
		if (dist[i - 1][j] != dist[i][j])
			cost = 1;
		else
			cost = 0;
		i--;
	}

	alignm_elem = __determine_alignment(i, j, alignment);

	alignment[alignm_elem].type = which;
	alignment[alignm_elem].cost = cost;

// Some Debug Output ... Can be deleted
/*	if( alignment[alignm_elem].type == HORIZONTAL)
		printf("HORIZONTAL \t Cost: %d i: %d  j: %d\n", alignment[alignm_elem].cost, i,j);
	if( alignment[alignm_elem].type == VERTICAL)
		printf("VERTICAL  \t Cost: %d  i: %d  j: %d\n", alignment[alignm_elem].cost, i,j);
	if( alignment[alignm_elem].type == DIAGONAL)
		printf("DIAGONAL  \t Cost: %d  i: %d  j: %d\n", alignment[alignm_elem].cost, i,j); */

	return (++alignm_elem);
}

int
LevenshteinDistance(char *s, int m, char *t, int n)
{
	//d is a table with m + 1 rows and n + 1 columns
	int             i, j, cost, min;

        //retiled[0] = 0;
	for (i = 0; i < m + 1; i++)
		dist[i][0] = i;
            //retiled[tiledIndex(i,0,ARRSIZE)] = i;

	for (i = 0; i < n + 1; i++)
		dist[0][i] = i;
            //retiled[tiledIndex(0,i,ARRSIZE)] = i;


	for (i = 1; i < m + 1; i++)
		for (j = 1; j < n + 1; j++) {
			if (s[i - 1] == t[j - 1])
				cost = 0;
			else
				cost = 1;

			dist[i][j] = min3(dist[i - 1][j] + 1, dist[i][j - 1] + 1, dist[i - 1][j - 1] + cost);
                        //retiled[tiledIndex(i,j,ARRSIZE)] = min3(retiled[tiledIndex(i - 1,j,ARRSIZE)] + 1,
                        //                                        retiled[tiledIndex(i,j - 1,ARRSIZE)] + 1,
                        //                                        retiled[tiledIndex(i - 1,j - 1,ARRSIZE)] + cost);

		}

	//printf("Edit Distance Matrix: \n\n");
	//for (i = 0; i < m + 1; i++) {
            //fprintf(outfileSequential, "Row %.4d\n", i);
        //    for (j = 0; j < n + 1; j++) {
	//	fprintf(outfileSequential,"[%d][%d] = %.4d\n", i, j, dist[i][j]);
        //    }
            //printf("\n");
//	}
	//printf("\n\n");
        printf("Value at [%.4d][%.4d] = %.4d\n",m,n,dist[m][n]);
                
	cost = dist[m][n];
        //cost = retiled[tiledIndex(m,n,ARRSIZE)];
	return cost;
}

void parallelLevenshtein(char* s1, char* s2, int* &result, int size) {
    //Not really parallel

    for(int i = 0; i <= ARRSIZE; ++i) //for each element in the first column
        result[getIndex(i,0)] = i;

    for (int i = 0; i <= ARRSIZE; i++)
        result[getIndex(0,i)] = i;

    //int i = threadIdx.x + 1;  //column
    int Rprev[ARRSIZE];
    int Rs[ARRSIZE];
    int* Rd = result;
    for(int i = 0; i < ARRSIZE; ++i)
        Rprev[i] = Rs[i] = Rd[i];

    int i;                    //column
    int j;                    //row

    for(int k = 2; k < (2 * size) - 1; ++k) {
        for(int x = 0; x < 5; ++x) { //x serves as threadIdx.x
            i = x + 1;
            //j = k - threadIdx.x;
            j = k - x - 1;

            if( j > 0 && j < size)
            {
                //Rs[threadIdx.x] = MIN( (Rd[index(j, i - 1)] + 1),
                //                       (Rprev[threadIdx.x] + 1) );


                Rs[x]           = MIN( (Rd[getIndex(i  ,j-1)] + 1),
                                       (Rd[getIndex(i-1,  j)] + 1)   );
                Rd[getIndex(i,j)]  = MIN( (Rs[x]),
                                       (Rd[getIndex(i-1,j-1)] + ((s1[i-1]!=s2[j-1])&1)) );
                Rs[x] = Rd[getIndex(i,j)];
                    
                //Rd[index(j,i)]  = MIN( (Rs[threadIdx.x]),
                //                       (Rd[index(j-1,i-1)] + ((Mds[i-1]==Nds[j-1])&1)) );
            } else {
                //printf("Thread %i, %i %i\n", x, i, j);
            }

            //__syncthreads();
            Rprev[x] = Rs[x];
            //Rprev[threadIdx.x] = Rs[threadIdx.x];
            //__syncthreads();
        }
    }
}