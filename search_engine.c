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

static void sort_postings_by_frequency(PostingNode *head)
{
    if (!head || !head->next) return;

    for (PostingNode *i = head; i != NULL; i = i->next) {
        for (PostingNode *j = i->next; j != NULL; j = j->next) {
            if (j->frequency > i->frequency) {
                int tmp_id   = i->doc_id;
                int tmp_freq = i->frequency;
                i->doc_id    = j->doc_id;
                i->frequency = j->frequency;
                j->doc_id    = tmp_id;
                j->frequency = tmp_freq;
            }
        }
    }
}

static const char *get_filename_by_id(int doc_id)
{
    for (int i = 0; i < doc_count; i++)
        if (docs[i].doc_id == doc_id)
            return docs[i].filename;
    return "(unknown)";
}

void search_word(const char *word)
{
    char norm[MAX_WORD_LEN];
    int j = 0;
    for (int i = 0; word[i] != '\0' && j < MAX_WORD_LEN - 1; i++) {
        if (isalpha((unsigned char)word[i]))
            norm[j++] = (char)tolower((unsigned char)word[i]);
    }
    norm[j] = '\0';

    if (j == 0) {
        printf("  [!] Empty or invalid query.\n");
        return;
    }

    HashNode *hnode = table[hash(norm)];
    while (hnode && strcmp(hnode->word, norm) != 0)
        hnode = hnode->next;

    if (!hnode) {
        printf("  No results found for \"%s\".\n", norm);
        return;
    }

    sort_postings_by_frequency(hnode->postings);

    int count = 0;
    for (PostingNode *p = hnode->postings; p != NULL; p = p->next)
        count++;

    printf("\n  Results for \"%s\"  (%d document%s found)\n",
           norm, count, count == 1 ? "" : "s");
    printf("  %-6s  %-8s  %s\n", "Rank", "Freq", "Document");
    printf("  %-6s  %-8s  %s\n", "------", "--------",
           "--------------------------------------------");

    int rank = 1;
    for (PostingNode *p = hnode->postings; p != NULL; p = p->next, rank++)
        printf("  %-6d  %-8d  %s\n", rank, p->frequency,
               get_filename_by_id(p->doc_id));
    printf("\n");
}

static HashNode *lookup(const char *raw)
{
    char norm[MAX_WORD_LEN];
    int j = 0;
    for (int i = 0; raw[i] != '\0' && j < MAX_WORD_LEN - 1; i++)
        if (isalpha((unsigned char)raw[i]))
            norm[j++] = (char)tolower((unsigned char)raw[i]);
    norm[j] = '\0';

    HashNode *h = table[hash(norm)];
    while (h && strcmp(h->word, norm) != 0)
        h = h->next;
    return h;
}

void search_and(const char *w1, const char *w2)
{
    HashNode *h1 = lookup(w1);
    HashNode *h2 = lookup(w2);

    if (!h1 || !h2) {
        printf("  No results: one or both words not in index.\n");
        return;
    }

    int res_id[MAX_DOCS], res_freq[MAX_DOCS], res_count = 0;

    for (PostingNode *p1 = h1->postings; p1 != NULL; p1 = p1->next)
        for (PostingNode *p2 = h2->postings; p2 != NULL; p2 = p2->next)
            if (p1->doc_id == p2->doc_id) {
                res_id[res_count]   = p1->doc_id;
                res_freq[res_count] = p1->frequency + p2->frequency;
                res_count++;
            }

    if (res_count == 0) {
        printf("  No documents contain both \"%s\" AND \"%s\".\n",
               h1->word, h2->word);
        return;
    }

    for (int i = 1; i < res_count; i++) {
        int kid = res_id[i], kf = res_freq[i], k = i - 1;
        while (k >= 0 && res_freq[k] < kf) {
            res_id[k+1]   = res_id[k];
            res_freq[k+1] = res_freq[k];
            k--;
        }
        res_id[k+1]   = kid;
        res_freq[k+1] = kf;
    }

    printf("\n  AND search: \"%s\" + \"%s\"  (%d document%s found)\n",
           h1->word, h2->word, res_count, res_count == 1 ? "" : "s");
    printf("  %-6s  %-8s  %s\n", "Rank", "CombFreq", "Document");
    printf("  %-6s  %-8s  %s\n", "------", "--------",
           "--------------------------------------------");

    for (int i = 0; i < res_count; i++)
        printf("  %-6d  %-8d  %s\n", i + 1, res_freq[i],
               get_filename_by_id(res_id[i]));
    printf("\n");
}

void display_top_words(void)
{
    char top_word[TOP_WORDS_COUNT][MAX_WORD_LEN];
    int  top_freq[TOP_WORDS_COUNT];
    int  top_count = 0;

    for (int i = 0; i < TOP_WORDS_COUNT; i++) {
        top_word[i][0] = '\0';
        top_freq[i]    = 0;
    }

    for (int i = 0; i < TABLE_SIZE; i++) {
        for (HashNode *h = table[i]; h != NULL; h = h->next) {
            int total = 0;
            for (PostingNode *p = h->postings; p != NULL; p = p->next)
                total += p->frequency;

            if (top_count < TOP_WORDS_COUNT || total > top_freq[top_count - 1]) {
                int ins = 0;
                while (ins < top_count && top_freq[ins] >= total)
                    ins++;

                if (ins < TOP_WORDS_COUNT) {
                    int end = top_count < TOP_WORDS_COUNT ? top_count : TOP_WORDS_COUNT - 1;
                    for (int s = end; s > ins; s--) {
                        top_freq[s] = top_freq[s - 1];
                        strncpy(top_word[s], top_word[s - 1], MAX_WORD_LEN - 1);
                    }
                    top_freq[ins] = total;
                    strncpy(top_word[ins], h->word, MAX_WORD_LEN - 1);
                    top_word[ins][MAX_WORD_LEN - 1] = '\0';
                    if (top_count < TOP_WORDS_COUNT) top_count++;
                }
            }
        }
    }

    printf("\n  Top %d most frequent words:\n", TOP_WORDS_COUNT);
    printf("  %-6s  %-9s  %s\n", "Rank", "TotalFreq", "Word");
    printf("  %-6s  %-9s  %s\n", "------", "---------",
           "---------------------------");
    for (int i = 0; i < top_count; i++)
        printf("  %-6d  %-9d  %s\n", i + 1, top_freq[i], top_word[i]);
    printf("\n");
}

void print_stats(void)
{
    int used = 0, words = 0, pairs = 0, max_chain = 0;

    for (int i = 0; i < TABLE_SIZE; i++) {
        int len = 0;
        for (HashNode *h = table[i]; h != NULL; h = h->next) {
            len++;
            words++;
            for (PostingNode *p = h->postings; p != NULL; p = p->next)
                pairs++;
        }
        if (len > 0) used++;
        if (len > max_chain) max_chain = len;
    }

    printf("\n  ── Index Statistics ──────────────────────────────\n");
    printf("  Table size          : %d buckets\n",  TABLE_SIZE);
    printf("  Used buckets        : %d  (load %.2f)\n",
           used, (double)used / TABLE_SIZE);
    printf("  Unique words        : %d\n",  words);
    printf("  Total (word,doc)    : %d\n",  pairs);
    printf("  Longest chain       : %d\n",  max_chain);
    printf("  Documents loaded    : %d\n",  doc_count);
    printf("  ──────────────────────────────────────────────────\n\n");
}

void free_index(void)
{
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashNode *hnode = table[i];
        while (hnode != NULL) {
            PostingNode *pnode = hnode->postings;
            while (pnode != NULL) {
                PostingNode *ptmp = pnode;
                pnode = pnode->next;
                free(ptmp);
            }
            HashNode *htmp = hnode;
            hnode = hnode->next;
            free(htmp);
        }
        table[i] = NULL;
    }
}

static void print_banner(void)
{
    printf("\n");
    printf("  ╔══════════════════════════════════════════╗\n");
    printf("  ║      Mini Search Engine  v1.0            ║\n");
    printf("  ║  Inverted Index · Hash Table · C99       ║\n");
    printf("  ╚══════════════════════════════════════════╝\n\n");
}

static void print_menu(void)
{
    printf("  ┌─────────────────────────────────┐\n");
    printf("  │  1. Load & index documents      │\n");
    printf("  │  2. Search — single keyword     │\n");
    printf("  │  3. Search — AND (two keywords) │\n");
    printf("  │  4. Show top %2d frequent words  │\n", TOP_WORDS_COUNT);
    printf("  │  5. Index statistics            │\n");
    printf("  │  6. Exit                        │\n");
    printf("  └─────────────────────────────────┘\n");
    printf("  Choice: ");
}

int main(void)
{
    for (int i = 0; i < TABLE_SIZE; i++)
        table[i] = NULL;

    print_banner();

    int running = 1;
    while (running) {
        print_menu();

        int choice;
        if (scanf("%d", &choice) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            printf("  [!] Please enter a number.\n\n");
            continue;
        }
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF);

        switch (choice) {

        case 1: {
            if (doc_count >= MAX_DOCS) {
                printf("  [!] Maximum document limit (%d) reached.\n\n", MAX_DOCS);
                break;
            }
            printf("  How many files to load? ");
            int n;
            if (scanf("%d", &n) != 1 || n <= 0) {
                while ((ch = getchar()) != '\n' && ch != EOF);
                printf("  [!] Invalid number.\n\n");
                break;
            }
            while ((ch = getchar()) != '\n' && ch != EOF);

            for (int i = 0; i < n; i++) {
                if (doc_count >= MAX_DOCS) {
                    printf("  [!] Reached max document limit.\n");
                    break;
                }
                printf("  Enter filename %d: ", i + 1);
                char fname[MAX_FILENAME];
                if (!fgets(fname, MAX_FILENAME, stdin)) break;

                size_t len = strlen(fname);
                if (len > 0 && fname[len - 1] == '\n')
                    fname[len - 1] = '\0';

                int id = doc_count;
                docs[id].doc_id = id;
                strncpy(docs[id].filename, fname, MAX_FILENAME - 1);
                docs[id].filename[MAX_FILENAME - 1] = '\0';

                printf("  Indexing \"%s\"...\n", fname);
                process_file(fname, id);
                doc_count++;
                printf("  Done.\n");
            }
            index_built = 1;
            printf("  All files indexed successfully.\n\n");
            break;
        }

        case 2: {
            if (!index_built || doc_count == 0) {
                printf("  [!] No documents loaded. Use option 1 first.\n\n");
                break;
            }
            printf("  Enter search word: ");
            char qword[MAX_WORD_LEN];
            if (!fgets(qword, MAX_WORD_LEN, stdin)) break;
            size_t ql = strlen(qword);
            if (ql > 0 && qword[ql - 1] == '\n') qword[ql - 1] = '\0';
            search_word(qword);
            break;
        }

        case 3: {
            if (!index_built || doc_count == 0) {
                printf("  [!] No documents loaded. Use option 1 first.\n\n");
                break;
            }
            char w1[MAX_WORD_LEN], w2[MAX_WORD_LEN];
            printf("  Enter first word : ");
            if (!fgets(w1, MAX_WORD_LEN, stdin)) break;
            size_t l1 = strlen(w1);
            if (l1 > 0 && w1[l1 - 1] == '\n') w1[l1 - 1] = '\0';

            printf("  Enter second word: ");
            if (!fgets(w2, MAX_WORD_LEN, stdin)) break;
            size_t l2 = strlen(w2);
            if (l2 > 0 && w2[l2 - 1] == '\n') w2[l2 - 1] = '\0';

            search_and(w1, w2);
            break;
        }

        case 4: {
            if (!index_built || doc_count == 0) {
                printf("  [!] No documents indexed yet.\n\n");
                break;
            }
            display_top_words();
            break;
        }

        case 5: {
            if (!index_built) {
                printf("  [!] No documents indexed yet.\n\n");
                break;
            }
            print_stats();
            break;
        }

        case 6: {
            printf("  Freeing memory...\n");
            free_index();
            printf("  Goodbye!\n\n");
            running = 0;
            break;
        }

        default:
            printf("  [!] Invalid option. Choose 1-6.\n\n");
        }
    }

    return 0;
}