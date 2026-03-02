#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> // ספרייה למדידת זמן

#define MAX_URL_LEN 256

typedef struct {
    char  url[MAX_URL_LEN];
    int  *neighbors;
    int   neighbor_count;
} Node;

typedef struct {
    Node *nodes;
    int   node_count;
} Graph;

Graph *load_db(const char *path) {
    clock_t start = clock(); // התחלת מדידת טעינה
    FILE *f = fopen(path, "rb");
    if (!f) { perror("fopen"); return NULL; }

    char magic[5] = {0};
    fread(magic, 1, 4, f);
    if (strcmp(magic, "WIKI") != 0) {
        fprintf(stderr, "Invalid DB file\n");
        fclose(f); return NULL;
    }

    int node_count, edge_count, queue_head, queue_tail;
    fread(&node_count,  sizeof(int), 1, f);
    fread(&edge_count,  sizeof(int), 1, f);
    fread(&queue_head,  sizeof(int), 1, f);
    fread(&queue_tail,  sizeof(int), 1, f);

    printf("Loading %d nodes, %d edges...\n", node_count, edge_count);

    Graph *g      = malloc(sizeof(Graph));
    g->nodes      = calloc(node_count, sizeof(Node));
    g->node_count = node_count;

    for (int i = 0; i < node_count; i++) {
        unsigned char len;
        fread(&len, 1, 1, f);
        fread(g->nodes[i].url, 1, len, f);
        g->nodes[i].url[len] = '\0';
    }

    for (int i = 0; i < edge_count; i++) {
        int from, to;
        fread(&from, sizeof(int), 1, f);
        fread(&to,   sizeof(int), 1, f);
        if (from < 0 || from >= node_count) continue;
        if (to   < 0 || to   >= node_count) continue;
        Node *n      = &g->nodes[from];
        n->neighbors = realloc(n->neighbors, (n->neighbor_count + 1) * sizeof(int));
        n->neighbors[n->neighbor_count++] = to;
    }

    fclose(f);
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("DB loaded in %.3f seconds!\n\n", time_spent);
    return g;
}

int find_node(Graph *g, const char *url) {
    for (int i = 0; i < g->node_count; i++)
        if (strcmp(g->nodes[i].url, url) == 0)
            return i;
    return -1;
}

void bfs(Graph *g, int src, int dst) {
    clock_t start = clock(); // התחלת מדידת זמן החיפוש
    
    int  *parent  = malloc(g->node_count * sizeof(int));
    char *visited = calloc(g->node_count, 1);
    int  *queue   = malloc(g->node_count * sizeof(int));
    int   head = 0, tail = 0, found = 0;

    for (int i = 0; i < g->node_count; i++) parent[i] = -1;
    visited[src]  = 1;
    queue[tail++] = src;

    while (head < tail && !found) {
        int cur = queue[head++];
        for (int i = 0; i < g->nodes[cur].neighbor_count; i++) {
            int nb = g->nodes[cur].neighbors[i];
            if (visited[nb]) continue;
            visited[nb] = 1;
            parent[nb]  = cur;
            if (nb == dst) { found = 1; break; }
            queue[tail++] = nb;
        }
    }

    clock_t end = clock(); // סיום מדידת זמן החיפוש
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    if (!found) {
        printf("No path found. Search took %.3f seconds.\n", time_spent);
    } else {
        int *path = malloc(g->node_count * sizeof(int));
        int len = 0, cur = dst;
        while (cur != -1) { path[len++] = cur; cur = parent[cur]; }

        printf("\n======================================\n");
        printf("  Shortest Path Found in %.3f seconds!\n", time_spent);
        printf("  Distance: %d steps\n", len - 1);
        printf("======================================\n\n");

        for (int i = len - 1; i >= 0; i--) {
            if      (i == len - 1) printf("Start : https://en.wikipedia.org%s\n", g->nodes[path[i]].url);
            else if (i == 0)       printf("End   : https://en.wikipedia.org%s\n", g->nodes[path[i]].url);
            else                   printf("Step %d: https://en.wikipedia.org%s\n", len-1-i, g->nodes[path[i]].url);
        }
        printf("\n======================================\n");
        free(path);
    }

    free(parent); free(visited); free(queue);
}

int main() {
    Graph *g = load_db("wiki.db");
    if (!g) return 1;

    char input[512], src_url[MAX_URL_LEN], dst_url[MAX_URL_LEN];

    printf("Enter source Wikipedia URL:\n> ");
    fgets(input, sizeof(input), stdin);
    char *s = strstr(input, "/wiki/");
    if (!s) { fprintf(stderr, "Invalid URL\n"); return 1; }
    strncpy(src_url, s, MAX_URL_LEN - 1);
    src_url[strcspn(src_url, "\n\r ")] = '\0';

    printf("Enter target Wikipedia URL:\n> ");
    fgets(input, sizeof(input), stdin);
    char *t = strstr(input, "/wiki/");
    if (!t) { fprintf(stderr, "Invalid URL\n"); return 1; }
    strncpy(dst_url, t, MAX_URL_LEN - 1);
    dst_url[strcspn(dst_url, "\n\r ")] = '\0';

    int src = find_node(g, src_url);
    int dst = find_node(g, dst_url);

    if (src == -1) { printf("Source not in DB: %s\n", src_url); return 1; }
    if (dst == -1) { printf("Target not in DB: %s\n", dst_url); return 1; }

    bfs(g, src, dst);

    for (int i = 0; i < g->node_count; i++) free(g->nodes[i].neighbors);
    free(g->nodes);
    free(g);
    return 0;
}