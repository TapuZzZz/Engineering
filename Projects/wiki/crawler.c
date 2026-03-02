#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <curl/curl.h>

// ─── Constants ───────────────────────────────────────────────
#define MAX_NODES   500000
#define MAX_EDGES   5000000
#define MAX_URL_LEN 256
#define DB_PATH     "wiki.db"
#define SAVE_EVERY  500        // Save checkpoint every 500 pages

// ─── Global flag — set by Ctrl+C ─────────────────────────────
volatile int stop_requested = 0;

void handle_sigint(int sig) {
    (void)sig;
    stop_requested = 1;
    printf("\n[!] Stop requested — finishing current page and saving...\n");
}

// ─── Structures ──────────────────────────────────────────────
typedef struct {
    char url[MAX_URL_LEN];
} Node;

typedef struct {
    int from;
    int to;
} Edge;

// ─── AVL Tree (visited set) ──────────────────────────────────
typedef struct AVLNode {
    char url[MAX_URL_LEN];
    int  index;
    int  height;
    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

int avl_height(AVLNode *n) { return n ? n->height : 0; }

void avl_update(AVLNode *n) {
    int l = avl_height(n->left), r = avl_height(n->right);
    n->height = 1 + (l > r ? l : r);
}

AVLNode *avl_rotate_right(AVLNode *y) {
    AVLNode *x = y->left;
    y->left = x->right; x->right = y;
    avl_update(y); avl_update(x);
    return x;
}

AVLNode *avl_rotate_left(AVLNode *x) {
    AVLNode *y = x->right;
    x->right = y->left; y->left = x;
    avl_update(x); avl_update(y);
    return y;
}

AVLNode *avl_balance(AVLNode *n) {
    avl_update(n);
    int bf = avl_height(n->left) - avl_height(n->right);
    if (bf > 1) {
        if (avl_height(n->left->left) < avl_height(n->left->right))
            n->left = avl_rotate_left(n->left);
        return avl_rotate_right(n);
    }
    if (bf < -1) {
        if (avl_height(n->right->right) < avl_height(n->right->left))
            n->right = avl_rotate_right(n->right);
        return avl_rotate_left(n);
    }
    return n;
}

int avl_find(AVLNode *root, const char *url) {
    while (root) {
        int cmp = strcmp(url, root->url);
        if      (cmp < 0) root = root->left;
        else if (cmp > 0) root = root->right;
        else              return root->index;
    }
    return -1;
}

AVLNode *avl_insert(AVLNode *root, const char *url, int idx, int *inserted) {
    if (!root) {
        AVLNode *n = calloc(1, sizeof(AVLNode));
        strncpy(n->url, url, MAX_URL_LEN - 1);
        n->index = idx; n->height = 1;
        *inserted = 1;
        return n;
    }
    int cmp = strcmp(url, root->url);
    if      (cmp < 0) root->left  = avl_insert(root->left,  url, idx, inserted);
    else if (cmp > 0) root->right = avl_insert(root->right, url, idx, inserted);
    else              { *inserted = 0; return root; }
    return avl_balance(root);
}

void avl_free(AVLNode *root) {
    if (!root) return;
    avl_free(root->left);
    avl_free(root->right);
    free(root);
}

// ─── BFS Queue ───────────────────────────────────────────────
typedef struct {
    int *data;
    int  head, tail, capacity;
} Queue;

Queue *queue_create(int cap) {
    Queue *q = malloc(sizeof(Queue));
    q->data  = malloc(cap * sizeof(int));
    q->head  = q->tail = 0;
    q->capacity = cap;
    return q;
}
void queue_push(Queue *q, int v) { q->data[q->tail++ % q->capacity] = v; }
int  queue_pop (Queue *q)        { return q->data[q->head++ % q->capacity]; }
int  queue_empty(Queue *q)       { return q->head == q->tail; }
void queue_free(Queue *q)        { free(q->data); free(q); }

// ─── HTTP ────────────────────────────────────────────────────
struct Memory { char *data; size_t size; };

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;
    mem->data = realloc(mem->data, mem->size + total + 1);
    memcpy(mem->data + mem->size, contents, total);
    mem->size += total;
    mem->data[mem->size] = '\0';
    return total;
}

char *fetch_url(const char *url) {
    CURL *curl = curl_easy_init();
    struct Memory response = {NULL, 0};
    if (!curl) return NULL;
    curl_easy_setopt(curl, CURLOPT_URL,            url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        15L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,      "wiki-crawler/1.0");
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) { free(response.data); return NULL; }
    return response.data;
}

// ─── Link Validation ─────────────────────────────────────────
int is_valid_wiki_link(const char *link) {
    if (strncmp(link, "/wiki/", 6) != 0) return 0;
    const char *skip[] = {
        "/wiki/Special:", "/wiki/Help:",       "/wiki/Wikipedia:",
        "/wiki/Talk:",    "/wiki/User:",        "/wiki/File:",
        "/wiki/Category:","/wiki/Template:",    "/wiki/Portal:",
        NULL
    };
    for (int i = 0; skip[i]; i++)
        if (strncmp(link, skip[i], strlen(skip[i])) == 0) return 0;
    return 1;
}

// ─── Save DB ─────────────────────────────────────────────────
//
//  FORMAT:
//  [4]  magic "WIKI"
//  [4]  node_count
//  [4]  edge_count
//  [4]  queue_head     ← so we can resume!
//  [4]  queue_tail
//  per node: [1] url_len + [url_len] url
//  per edge: [4] from + [4] to
//  queue:    [4 * capacity] all queue indices
//
void save_db(const char *path,
             Node *nodes, int node_count,
             Edge *edges, int edge_count,
             Queue *queue) {
    // Write to temp file first — then rename
    // This way a crash mid-write never corrupts the DB!
    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE *f = fopen(tmp_path, "wb");
    if (!f) { perror("fopen"); return; }

    // Header
    fwrite("WIKI",       1,            4, f);
    fwrite(&node_count,  sizeof(int),  1, f);
    fwrite(&edge_count,  sizeof(int),  1, f);
    fwrite(&queue->head, sizeof(int),  1, f);
    fwrite(&queue->tail, sizeof(int),  1, f);

    // Nodes
    for (int i = 0; i < node_count; i++) {
        unsigned char len = (unsigned char)strlen(nodes[i].url);
        fwrite(&len,         1,   1, f);
        fwrite(nodes[i].url, 1, len, f);
    }

    // Edges
    fwrite(edges, sizeof(Edge), edge_count, f);

    // Queue data (so we know where to continue from)
    fwrite(queue->data, sizeof(int), queue->capacity, f);

    fclose(f);

    // Atomic replace — rename is atomic on Linux!
    rename(tmp_path, path);

    printf("[SAVED] %d nodes | %d edges → %s\n",
           node_count, edge_count, path);
}

// ─── Load DB ─────────────────────────────────────────────────
int load_db(const char *path,
            Node *nodes, int *node_count,
            Edge *edges, int *edge_count,
            Queue *queue, AVLNode **visited) {

    FILE *f = fopen(path, "rb");
    if (!f) return 0; // No DB yet — start fresh

    char magic[5] = {0};
    fread(magic, 1, 4, f);
    if (strcmp(magic, "WIKI") != 0) {
        fprintf(stderr, "Corrupted DB!\n");
        fclose(f);
        return 0;
    }

    fread(node_count,    sizeof(int), 1, f);
    fread(edge_count,    sizeof(int), 1, f);
    fread(&queue->head,  sizeof(int), 1, f);
    fread(&queue->tail,  sizeof(int), 1, f);

    // Nodes
    for (int i = 0; i < *node_count; i++) {
        unsigned char len;
        fread(&len, 1, 1, f);
        fread(nodes[i].url, 1, len, f);
        nodes[i].url[len] = '\0';

        // Rebuild AVL visited set
        int ins = 0;
        *visited = avl_insert(*visited, nodes[i].url, i, &ins);
    }

    // Edges
    fread(edges, sizeof(Edge), *edge_count, f);

    // Queue
    fread(queue->data, sizeof(int), queue->capacity, f);

    fclose(f);
    printf("[LOADED] %d nodes | %d edges from %s\n",
           *node_count, *edge_count, path);
    printf("[RESUME] Queue position: %d → %d\n\n",
           queue->head, queue->tail);
    return 1;
}

// ─── Main ────────────────────────────────────────────────────
int main(int argc, char *argv[]) {
    const char *seed      = "/wiki/Science";
    int         max_pages = 100000;

    if (argc >= 2) seed      = argv[1];
    if (argc >= 3) max_pages = atoi(argv[2]);

    // Register Ctrl+C handler
    signal(SIGINT, handle_sigint);

    // Allocate
    Node  *nodes = calloc(MAX_NODES, sizeof(Node));
    Edge  *edges = calloc(MAX_EDGES, sizeof(Edge));
    Queue *queue = queue_create(MAX_NODES);
    AVLNode *visited = NULL;
    int node_count = 0, edge_count = 0;

    if (!nodes || !edges) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }

    // Try to load existing DB and resume
    int resumed = load_db(DB_PATH, nodes, &node_count,
                          edges, &edge_count, queue, &visited);

    if (!resumed) {
        // Fresh start
        printf("Starting fresh from: %s\n\n", seed);
        strncpy(nodes[0].url, seed, MAX_URL_LEN - 1);
        node_count = 1;
        int ins = 0;
        visited = avl_insert(visited, seed, 0, &ins);
        queue_push(queue, 0);
    }

    int pages_crawled = 0;

    // ── Crawl loop ────────────────────────────────────────────
    while (!queue_empty(queue) &&
           pages_crawled < max_pages &&
           !stop_requested) {

        int cur_idx = queue_pop(queue);

        char full_url[512];
        snprintf(full_url, sizeof(full_url),
                 "https://en.wikipedia.org%s", nodes[cur_idx].url);

        char *html = fetch_url(full_url);
        if (!html) continue;

        pages_crawled++;

        // Progress every 100 pages
        if (pages_crawled % 100 == 0)
            printf("[%d pages] [%d nodes] [%d edges]\n",
                   pages_crawled, node_count, edge_count);

        // Parse links
        const char *ptr = html;
        while ((ptr = strstr(ptr, "href=\"")) != NULL) {
            ptr += 6;
            const char *end = strchr(ptr, '"');
            if (!end) break;

            size_t len = end - ptr;
            char link[MAX_URL_LEN];
            if (len >= MAX_URL_LEN) { ptr = end + 1; continue; }
            strncpy(link, ptr, len);
            link[len] = '\0';

            if (!is_valid_wiki_link(link)) { ptr = end + 1; continue; }

            int ins = 0;
            int to_idx = node_count;
            visited = avl_insert(visited, link, to_idx, &ins);

            if (ins && node_count < MAX_NODES) {
                strncpy(nodes[node_count].url, link, MAX_URL_LEN - 1);
                node_count++;
                queue_push(queue, to_idx);
            } else if (!ins) {
                to_idx = avl_find(visited, link);
            }

            if (edge_count < MAX_EDGES) {
                edges[edge_count].from = cur_idx;
                edges[edge_count].to   = to_idx;
                edge_count++;
            }

            ptr = end + 1;
        }

        free(html);

        // Periodic save
        if (pages_crawled % SAVE_EVERY == 0)
            save_db(DB_PATH, nodes, node_count,
                    edges, edge_count, queue);
    }

    // Final save — always happens, even after Ctrl+C
    save_db(DB_PATH, nodes, node_count, edges, edge_count, queue);

    printf("\nDone. Crawled %d pages this session.\n", pages_crawled);

    free(nodes);
    free(edges);
    queue_free(queue);
    avl_free(visited);
    return 0;
}