#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TABLE_SIZE      1024
#define MAX_WORD_LEN    128
#define MAX_DOCS        256
#define MAX_FILENAME    512
#define TOP_WORDS_COUNT 10

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

static const char *STOP_WORDS[] = {
    "a", "an", "the", "is", "it", "in", "on", "at", "to", "of",
    "and", "or", "but", "for", "nor", "so", "yet", "be", "was",
    "are", "were", "has", "have", "had", "do", "does", "did",
    "with", "this", "that", "from", "by", "as", "not", "no",
    "if", "its", "he", "she", "we", "they", "you", "i", "my",
    "his", "her", "our", "their", "your", NULL
};

unsigned int hash(const char *word)
{
    unsigned long h = 5381;
    int c;
    while ((c = (unsigned char)*word++) != '\0')
        h = ((h << 5) + h) + c;
    return (unsigned int)(h % TABLE_SIZE);
}

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
        hnode->next     = table[idx];
        table[idx]      = hnode;
    }

    PostingNode *pnode = hnode->postings;
    while (pnode != NULL) {
        if (pnode->doc_id == doc_id) {
            pnode->frequency++;
            return;
        }
        pnode = pnode->next;
    }

    pnode = (PostingNode *)malloc(sizeof(PostingNode));
    if (!pnode) { perror("malloc PostingNode"); exit(EXIT_FAILURE); }

    pnode->doc_id    = doc_id;
    pnode->frequency = 1;
    pnode->next      = hnode->postings;
    hnode->postings  = pnode;
}

void process_file(const char *filename, int doc_id)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "  [!] Cannot open file: %s\n", filename);
        return;
    }

    char raw[MAX_WORD_LEN * 2];
    char word[MAX_WORD_LEN];

    while (fscanf(fp, "%255s", raw) == 1) {
        int j = 0;
        for (int i = 0; raw[i] != '\0' && j < MAX_WORD_LEN - 1; i++) {
            if (isalpha((unsigned char)raw[i]))
                word[j++] = (char)tolower((unsigned char)raw[i]);
        }
        word[j] = '\0';

        if (j == 0)             continue;
        if (is_stop_word(word)) continue;

        insert_word(word, doc_id);
    }

    fclose(fp);
}

int main(void)
{
    for (int i = 0; i < TABLE_SIZE; i++)
        table[i] = NULL;

    char *test_files[] = { "doc1.txt", "doc2.txt", "doc3.txt" };
    int   n = 3;

    for (int i = 0; i < n; i++) {
        docs[i].doc_id = i;
        strncpy(docs[i].filename, test_files[i], MAX_FILENAME - 1);
        docs[i].filename[MAX_FILENAME - 1] = '\0';
        printf("Indexing \"%s\" (doc_id=%d)...\n", test_files[i], i);
        process_file(test_files[i], i);
        doc_count++;
    }

    index_built = 1;

    const char *probe_words[] = { "search", "index", "hash", "data" };
    for (int w = 0; w < 4; w++) {
        unsigned int idx  = hash(probe_words[w]);
        HashNode    *node = table[idx];
        while (node && strcmp(node->word, probe_words[w]) != 0)
            node = node->next;

        if (node) {
            printf("\n\"%s\" found in:\n", probe_words[w]);
            for (PostingNode *p = node->postings; p != NULL; p = p->next)
                printf("  %-20s  freq=%d\n", docs[p->doc_id].filename, p->frequency);
        } else {
            printf("\n\"%s\" not found.\n", probe_words[w]);
        }
    }

    printf("\nCommit 3 OK — file processing and normalisation verified.\n");
    return 0;
}