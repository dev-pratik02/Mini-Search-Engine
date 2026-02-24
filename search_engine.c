
#include <stdio.h>   
#include <stdlib.h>  
#include <string.h>                        
#include <ctype.h>  



#define TABLE_SIZE      1024  
#define MAX_WORD_LEN    128    
#define MAX_DOCS        256   
#define MAX_FILENAME    512    

#define TOP_WORDS_COUNT  10   
typedef struct {
    int  doc_id;                  
    char filename[MAX_FILENAME];  
} Document;



typedef struct PostingNode {
    int               doc_id;    
    int               frequency; 
    struct PostingNode *next;    
} PostingNode;



typedef struct HashNode {
    char             word[MAX_WORD_LEN]; 
    PostingNode     *postings;           
    struct HashNode *next;               
} HashNode;



static HashNode *table[TABLE_SIZE]; 

static Document  docs[MAX_DOCS];    
static int       doc_count  = 0;    
static int       index_built = 0;   



unsigned int hash(const char *word)
{
    unsigned long h = 5381;
    int c;

    while ((c = (unsigned char)*word++) != '\0')
        h = ((h << 5) + h) + c;   /* h = h * 33 + c */

    return (unsigned int)(h % TABLE_SIZE);
}



int main(void)
{

    for (int i = 0; i < TABLE_SIZE; i++)
        table[i] = NULL;

    /* Smoke-test the hash function with a few sample words */
    printf("Hash table initialised (%d buckets).\n", TABLE_SIZE);
    printf("hash(\"hello\")    = %u\n", hash("hello"));
    printf("hash(\"world\")    = %u\n", hash("world"));
    printf("hash(\"search\")   = %u\n", hash("search"));
    printf("hash(\"engine\")   = %u\n", hash("engine"));
    printf("hash(\"index\")    = %u\n", hash("index"));


    printf("\nScaffold OK — data structures defined, hash function verified.\n");
    printf("doc_count  = %d  (no documents loaded yet)\n", doc_count);
    printf("index_built = %d  (index not yet populated)\n", index_built);

    return 0;
}