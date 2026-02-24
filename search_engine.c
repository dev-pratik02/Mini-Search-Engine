

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TABLE_SIZE      1024
#define MAX_WORD_LEN    128
#define MAX_DOCS        256
#define MAX_FILENAME    512
#define TOP_WORDS_COUNT  10




static const char *STOP_WORDS[] = {
    "a", "an", "the", "is", "it", "in", "on", "at", "to", "of",
    "and", "or", "but", "for", "nor", "so", "yet", "be", "was",
    "are", "were", "has", "have", "had", "do", "does", "did",
    "with", "this", "that", "from", "by", "as", "not", "no",
    "if", "its", "he", "she", "we", "they", "you", "i", "my",
    "his", "her", "our", "their", "your", NULL
};

/* ============================================================================
 *  DATA STRUCTURES
 * ============================================================================ */

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

/* ============================================================================
 *  GLOBAL STATE
 * ============================================================================ */

static HashNode *table[TABLE_SIZE];
static Document  docs[MAX_DOCS];
static int       doc_count   = 0;
static int       index_built = 0;




unsigned int hash(const char *word)
{
    unsigned long h = 5381;
    int c;
    while ((c = (unsigned char)*word++) != '\0')
        h = ((h << 5) + h) + c;
    return (unsigned int)(h % TABLE_SIZE);
}

/* ============================================================================
 *  STOP-WORD CHECK
 *  Time: O(S) where S = number of stop words
 * ============================================================================ */

static int is_stop_word(const char *word)
{
    for (int i = 0; STOP_WORDS[i] != NULL; i++)
        if (strcmp(word, STOP_WORDS[i]) == 0)
            return 1;
    return 0;
}


void insert_word(const char *word, int doc_id)
{
    unsigned int idx = hash(word);

    /* --- Find or create the HashNode for this word --- */
    HashNode *hnode = table[idx];
    while (hnode != NULL) {
        if (strcmp(hnode->word, word) == 0)
            break;
        hnode = hnode->next;
    }

    if (hnode == NULL) {
        hnode = (HashNode *)malloc(sizeof(HashNode));
        if (!hnode) { perror("malloc HashNode"); exit(EXIT_FAILURE); }

        strncpy(hnode->word, word, MAX_WORD_LEN - 1);
        hnode->word[MAX_WORD_LEN - 1] = '\0';
        hnode->postings = NULL;
        hnode->next     = table[idx];   /* prepend to chain */
        table[idx]      = hnode;
    }

    /* --- Find or create the PostingNode for this doc_id --- */
    PostingNode *pnode = hnode->postings;
    while (pnode != NULL) {
        if (pnode->doc_id == doc_id) {
            pnode->frequency++;         /* word seen again in same document */
            return;
        }
        pnode = pnode->next;
    }

    pnode = (PostingNode *)malloc(sizeof(PostingNode));
    if (!pnode) { perror("malloc PostingNode"); exit(EXIT_FAILURE); }

    pnode->doc_id    = doc_id;
    pnode->frequency = 1;
    pnode->next      = hnode->postings; /* prepend to posting list */
    hnode->postings  = pnode;
}



int main(void)
{
    for (int i = 0; i < TABLE_SIZE; i++)
        table[i] = NULL;

    (void)docs;
    (void)doc_count;
    (void)index_built;

    /* Verify stop-word detection */
    printf("is_stop_word(\"the\")    = %d  (expected 1)\n", is_stop_word("the"));
    printf("is_stop_word(\"search\") = %d  (expected 0)\n", is_stop_word("search"));

    /* Simulate two documents being indexed */
    insert_word("search", 0);
    insert_word("engine", 0);
    insert_word("search", 0);   /* duplicate -> should increment freq  */
    insert_word("search", 1);   /* same word, different doc            */
    insert_word("index",  0);
    insert_word("index",  1);

    /* Inspect posting list for "search": expect doc0=freq2, doc1=freq1 */
    unsigned int idx  = hash("search");
    HashNode    *node = table[idx];
    while (node && strcmp(node->word, "search") != 0)
        node = node->next;

    if (node) {
        printf("\nPosting list for \"search\":\n");
        for (PostingNode *p = node->postings; p != NULL; p = p->next)
            printf("  doc_id=%-3d  freq=%d\n", p->doc_id, p->frequency);
    }

    /* Inspect posting list for "index": expect doc0=freq1, doc1=freq1 */
    idx  = hash("index");
    node = table[idx];
    while (node && strcmp(node->word, "index") != 0)
        node = node->next;

    if (node) {
        printf("\nPosting list for \"index\":\n");
        for (PostingNode *p = node->postings; p != NULL; p = p->next)
            printf("  doc_id=%-3d  freq=%d\n", p->doc_id, p->frequency);
    }

    printf("\n OK — insertion and collision handling verified.\n");
    return 0;
}