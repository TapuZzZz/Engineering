/*
 * Wikipedia Bidirectional BFS — ncurses live visualization
 * ==========================================================
 *
 * Algorithms & Data Structures:
 *  - Bidirectional BFS:  O(B^(D/2)) vs O(B^D) — exponentially faster
 *  - Hash Map (FNV-1a):  O(1) average node lookup vs O(N) linear scan
 *  - Dynamic arrays:     amortized O(1) append, no fixed limits
 *  - ncurses:            live terminal graph visualization
 *
 * Compile (Linux/MSYS2):
 *   gcc wiki_bidir.c -o wiki_bidir -lcurl -lncurses -lm
 *
 * Run:
 *   ./wiki_bidir Dog Internet
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ncurses.h>
#include <curl/curl.h>

/* ═══════════════════════════════════════
   CONSTANTS
   ═══════════════════════════════════════ */
#define MAX_TITLE      256
#define HASH_SIZE      16381   /* prime — hash table buckets              */
#define INIT_NODES     128     /* initial node array capacity             */
#define INIT_ADJ       16      /* initial adjacency list capacity per node */
#define MAX_LINKS      200     /* links fetched per Wikipedia page        */
#define SIDE_W         52      /* width of left info panel                */
#define LOG_LINES      14      /* visible log lines                       */

/* ═══════════════════════════════════════
   DATA STRUCTURES
   ═══════════════════════════════════════ */

/* ── Hash map entry ── */
typedef struct HashEntry {
    char             key[MAX_TITLE];
    int              node_id;
    struct HashEntry *next;    /* chaining for collisions */
} HashEntry;

/* ── Dynamic adjacency list ── */
typedef struct {
    int  *neighbors;   /* heap array */
    int   count;
    int   capacity;
} AdjList;

/* ── Node states (for visualization) ── */
typedef enum {
    ST_UNKNOWN = 0,
    ST_QUEUED_FWD,
    ST_VISITED_FWD,
    ST_QUEUED_BWD,
    ST_VISITED_BWD,
    ST_PATH,
    ST_START,
    ST_END,
    ST_CURRENT
} NodeState;

/* ── Graph node ── */
typedef struct {
    char       title[MAX_TITLE];
    int        parent_fwd;   /* parent in forward  BFS tree */
    int        parent_bwd;   /* parent in backward BFS tree */
    int        dist_fwd;     /* distance from source        */
    int        dist_bwd;     /* distance from target        */
    NodeState  state;
    int        fetched;      /* links already loaded?       */
    /* Screen position (for visualization) */
    float      vx, vy;       /* visual x,y in [0,1] space   */
} GraphNode;

/* ── Dynamic graph ── */
typedef struct {
    GraphNode  *nodes;
    AdjList    *adj;
    int         count;
    int         capacity;
    /* Hash map for O(1) title→id lookup */
    HashEntry  *htable[HASH_SIZE];
} Graph;

/* ── Dynamic queue ── */
typedef struct {
    int  *data;
    int   head, tail, size, cap;
} Queue;

/* ── HTTP buffer ── */
typedef struct { char *buf; size_t len; } HttpBuf;

/* ── Log ring buffer ── */
typedef struct {
    char  lines[LOG_LINES][128];
    int   head;
    int   count;
} Log;

/* ═══════════════════════════════════════
   GLOBALS (kept minimal)
   ═══════════════════════════════════════ */
static Log     g_log;
static WINDOW *g_win_info  = NULL;
static WINDOW *g_win_graph = NULL;
static WINDOW *g_win_log   = NULL;
static int     g_screen_h, g_screen_w;

/* ═══════════════════════════════════════
   FNV-1a HASH  —  O(|key|), very fast
   ═══════════════════════════════════════ */
static unsigned int fnv1a(const char *s) {
    unsigned int h = 2166136261u;
    while (*s) { h ^= (unsigned char)*s++; h *= 16777619u; }
    return h % HASH_SIZE;
}

/* ═══════════════════════════════════════
   GRAPH  —  dynamic + hash map
   ═══════════════════════════════════════ */
Graph *graph_new(void) {
    Graph *g = calloc(1, sizeof(Graph));
    g->capacity = INIT_NODES;
    g->nodes    = calloc(g->capacity, sizeof(GraphNode));
    g->adj      = calloc(g->capacity, sizeof(AdjList));
    return g;
}

static void graph_grow(Graph *g) {
    int nc = g->capacity * 2;
    g->nodes = realloc(g->nodes, nc * sizeof(GraphNode));
    g->adj   = realloc(g->adj,   nc * sizeof(AdjList));
    memset(g->nodes + g->capacity, 0, g->capacity * sizeof(GraphNode));
    memset(g->adj   + g->capacity, 0, g->capacity * sizeof(AdjList));
    g->capacity = nc;
}

/* O(1) average lookup via hash map */
int graph_find(Graph *g, const char *title) {
    HashEntry *e = g->htable[fnv1a(title)];
    for (; e; e = e->next)
        if (strcmp(e->key, title) == 0) return e->node_id;
    return -1;
}

int graph_add(Graph *g, const char *title) {
    int existing = graph_find(g, title);
    if (existing >= 0) return existing;

    if (g->count >= g->capacity) graph_grow(g);
    int id = g->count++;
    strncpy(g->nodes[id].title, title, MAX_TITLE-1);
    g->nodes[id].parent_fwd = -1;
    g->nodes[id].parent_bwd = -1;
    g->nodes[id].dist_fwd   = -1;
    g->nodes[id].dist_bwd   = -1;
    g->nodes[id].state      = ST_UNKNOWN;
    /* Spread nodes visually in a spiral */
    float t = id * 2.399963f;  /* golden angle */
    float r = sqrtf((float)id / (g->capacity > 0 ? g->capacity : 1));
    g->nodes[id].vx = 0.5f + r * cosf(t) * 0.45f;
    g->nodes[id].vy = 0.5f + r * sinf(t) * 0.45f;

    /* Insert into hash map */
    HashEntry *e = malloc(sizeof(HashEntry));
    strncpy(e->key, title, MAX_TITLE-1);
    e->node_id = id;
    unsigned int h = fnv1a(title);
    e->next = g->htable[h];
    g->htable[h] = e;
    return id;
}

void graph_add_edge(Graph *g, int from, int to) {
    AdjList *a = &g->adj[from];
    /* check duplicate */
    for (int i = 0; i < a->count; i++)
        if (a->neighbors[i] == to) return;
    if (a->count >= a->capacity) {
        a->capacity = a->capacity ? a->capacity * 2 : INIT_ADJ;
        a->neighbors = realloc(a->neighbors, a->capacity * sizeof(int));
    }
    a->neighbors[a->count++] = to;
}

void graph_free(Graph *g) {
    for (int i = 0; i < HASH_SIZE; i++) {
        HashEntry *e = g->htable[i];
        while (e) { HashEntry *t = e->next; free(e); e = t; }
    }
    for (int i = 0; i < g->count; i++) free(g->adj[i].neighbors);
    free(g->nodes); free(g->adj); free(g);
}

/* ═══════════════════════════════════════
   DYNAMIC QUEUE
   ═══════════════════════════════════════ */
Queue *queue_new(void) {
    Queue *q = malloc(sizeof(Queue));
    q->cap  = 256; q->head = q->tail = q->size = 0;
    q->data = malloc(q->cap * sizeof(int));
    return q;
}
static void queue_grow(Queue *q) {
    int nc = q->cap * 2;
    int *nd = malloc(nc * sizeof(int));
    for (int i = 0; i < q->size; i++)
        nd[i] = q->data[(q->head + i) % q->cap];
    free(q->data); q->data = nd;
    q->head = 0; q->tail = q->size; q->cap = nc;
}
void  queue_push(Queue *q, int v) {
    if (q->size >= q->cap) queue_grow(q);
    q->data[q->tail] = v;
    q->tail = (q->tail+1) % q->cap;
    q->size++;
}
int   queue_pop (Queue *q) {
    int v = q->data[q->head];
    q->head = (q->head+1) % q->cap;
    q->size--;
    return v;
}
int   queue_empty(Queue *q) { return q->size == 0; }
void  queue_free(Queue *q)  { free(q->data); free(q); }

/* ═══════════════════════════════════════
   LOG
   ═══════════════════════════════════════ */
void log_push(const char *msg) {
    strncpy(g_log.lines[g_log.head], msg, 127);
    g_log.head = (g_log.head + 1) % LOG_LINES;
    if (g_log.count < LOG_LINES) g_log.count++;
}

/* ═══════════════════════════════════════
   HTTP + WIKIPEDIA API
   ═══════════════════════════════════════ */
static size_t http_cb(void *d, size_t s, size_t n, void *u) {
    size_t t = s*n; HttpBuf *b = u;
    b->buf = realloc(b->buf, b->len + t + 1);
    memcpy(b->buf + b->len, d, t);
    b->len += t; b->buf[b->len] = '\0';
    return t;
}

char *fetch_links(const char *title) {
    CURL *c = curl_easy_init(); if (!c) return NULL;
    char *enc = curl_easy_escape(c, title, 0);
    char url[1024];
    snprintf(url, sizeof(url),
        "https://en.wikipedia.org/w/api.php"
        "?action=query&titles=%s&prop=links"
        "&pllimit=%d&format=json&plnamespace=0",
        enc, MAX_LINKS);
    curl_free(enc);
    HttpBuf b = { malloc(1), 0 }; b.buf[0] = '\0';
    curl_easy_setopt(c, CURLOPT_URL,           url);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, http_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA,     &b);
    curl_easy_setopt(c, CURLOPT_USERAGENT,     "WikiBidir/1.0");
    curl_easy_setopt(c, CURLOPT_TIMEOUT,       8L);
    CURLcode rc = curl_easy_perform(c);
    curl_easy_cleanup(c);
    if (rc != CURLE_OK) { free(b.buf); return NULL; }
    return b.buf;
}

/* Load neighbors of node `id` into graph. Returns count. */
int load_neighbors(Graph *g, int id) {
    if (g->nodes[id].fetched) return 0;
    g->nodes[id].fetched = 1;
    char *json = fetch_links(g->nodes[id].title);
    if (!json) return 0;
    int count = 0;
    const char *p = json;
    while ((p = strstr(p, "\"title\":\"")) != NULL) {
        p += 9;
        char title[MAX_TITLE]; int i = 0;
        while (*p && *p != '"' && i < MAX_TITLE-1) {
            if (p[0]=='\\' && p[1]) { p+=2; continue; }
            title[i++] = *p++;
        }
        title[i] = '\0';
        if (*p == '"') p++;
        if (strchr(title, ':')) continue;
        int nb = graph_add(g, title);
        graph_add_edge(g, id, nb);
        if (++count >= MAX_LINKS) break;
    }
    free(json);
    return count;
}

/* ═══════════════════════════════════════
   NCURSES VISUALIZATION
   ═══════════════════════════════════════ */

void ui_init(void) {
    initscr();
    start_color();
    use_default_colors();
    cbreak(); noecho(); curs_set(0);
    getmaxyx(stdscr, g_screen_h, g_screen_w);

    /* Color pairs */
    init_pair(1,  COLOR_GREEN,   -1);  /* normal green    */
    init_pair(2,  COLOR_CYAN,    -1);  /* fwd visited     */
    init_pair(3,  COLOR_BLUE,    -1);  /* bwd visited     */
    init_pair(4,  COLOR_YELLOW,  -1);  /* path            */
    init_pair(5,  COLOR_RED,     -1);  /* current/target  */
    init_pair(6,  COLOR_WHITE,   -1);  /* labels          */
    init_pair(7,  COLOR_MAGENTA, -1);  /* queued          */
    init_pair(8,  COLOR_BLACK,   COLOR_GREEN);   /* start */
    init_pair(9,  COLOR_BLACK,   COLOR_RED);     /* end   */

    int gh = g_screen_h - 3;   /* graph window height     */
    int gw = g_screen_w - SIDE_W - 1;

    g_win_graph = newwin(gh,         gw,       0,      SIDE_W+1);
    g_win_info  = newwin(gh - LOG_LINES - 1, SIDE_W, 0, 0);
    g_win_log   = newwin(LOG_LINES+1, SIDE_W,  gh - LOG_LINES - 1, 0);

    /* Status bar */
    mvhline(g_screen_h-3, 0, ACS_HLINE, g_screen_w);
    mvhline(gh,            0, ACS_HLINE, g_screen_w);
    mvvline(0, SIDE_W,  ACS_VLINE, gh);
}

void ui_draw_graph(Graph *g, int path[], int plen) {
    int h, w;
    getmaxyx(g_win_graph, h, w);
    werase(g_win_graph);
    box(g_win_graph, 0, 0);

    /* Draw a tiny ASCII-art representation of nodes */
    for (int i = 0; i < g->count && i < 800; i++) {
        GraphNode *n = &g->nodes[i];
        int cx = 1 + (int)(n->vx * (w - 2));
        int cy = 1 + (int)(n->vy * (h - 2));
        cx = cx < 1 ? 1 : cx >= w-1 ? w-2 : cx;
        cy = cy < 1 ? 1 : cy >= h-1 ? h-2 : cy;

        /* Draw edges to neighbors (dim) */
        AdjList *a = &g->adj[i];
        for (int j = 0; j < a->count && j < 6; j++) {
            int nb = a->neighbors[j];
            if (nb >= g->count) continue;
            int nx = 1 + (int)(g->nodes[nb].vx * (w-2));
            int ny = 1 + (int)(g->nodes[nb].vy * (h-2));
            /* Draw one char midpoint as edge hint */
            int mx = (cx + nx) / 2, my = (cy + ny) / 2;
            if (mx > 0 && mx < w-1 && my > 0 && my < h-1) {
                wattron(g_win_graph, COLOR_PAIR(1) | A_DIM);
                mvwaddch(g_win_graph, my, mx, '.');
                wattroff(g_win_graph, COLOR_PAIR(1) | A_DIM);
            }
        }
    }

    /* Draw path edges (bright) */
    for (int i = 0; i < plen-1; i++) {
        GraphNode *a = &g->nodes[path[i]];
        GraphNode *b = &g->nodes[path[i+1]];
        int ax = 1+(int)(a->vx*(w-2)), ay = 1+(int)(a->vy*(h-2));
        int bx = 1+(int)(b->vx*(w-2)), by = 1+(int)(b->vy*(h-2));
        /* Draw dashes between them */
        int steps = abs(bx-ax) > abs(by-ay) ? abs(bx-ax) : abs(by-ay);
        steps = steps ? steps : 1;
        for (int s = 0; s <= steps; s++) {
            int px = ax + (bx-ax)*s/steps;
            int py = ay + (by-ay)*s/steps;
            if (px>0&&px<w-1&&py>0&&py<h-1) {
                wattron(g_win_graph, COLOR_PAIR(4)|A_BOLD);
                mvwaddch(g_win_graph, py, px, '-');
                wattroff(g_win_graph, COLOR_PAIR(4)|A_BOLD);
            }
        }
    }

    /* Draw nodes on top */
    for (int i = 0; i < g->count && i < 800; i++) {
        GraphNode *n = &g->nodes[i];
        int cx = 1 + (int)(n->vx * (w - 2));
        int cy = 1 + (int)(n->vy * (h - 2));
        cx = cx<1?1:cx>=w-1?w-2:cx;
        cy = cy<1?1:cy>=h-1?h-2:cy;

        int pair; chtype ch;
        switch (n->state) {
            case ST_START:       pair=8; ch='S'; break;
            case ST_END:         pair=9; ch='E'; break;
            case ST_PATH:        pair=4; ch='*'; break;
            case ST_CURRENT:     pair=5; ch='@'; break;
            case ST_VISITED_FWD: pair=2; ch='o'; break;
            case ST_VISITED_BWD: pair=3; ch='o'; break;
            case ST_QUEUED_FWD:  pair=7; ch='+'; break;
            case ST_QUEUED_BWD:  pair=7; ch='+'; break;
            default:             pair=1; ch='.'; break;
        }
        wattron(g_win_graph,  COLOR_PAIR(pair) | (pair<=3?A_BOLD:0));
        mvwaddch(g_win_graph, cy, cx, ch);
        wattroff(g_win_graph, COLOR_PAIR(pair) | A_BOLD);

        /* Label for important nodes */
        if (n->state==ST_START || n->state==ST_END || n->state==ST_PATH
            || n->state==ST_CURRENT) {
            char lbl[20];
            snprintf(lbl, sizeof(lbl), "%.12s", n->title);
            int lx = cx + 1;
            if (lx + (int)strlen(lbl) >= w-1) lx = cx - (int)strlen(lbl) - 1;
            if (lx < 1) lx = 1;
            wattron(g_win_graph, COLOR_PAIR(pair)|A_BOLD);
            mvwprintw(g_win_graph, cy, lx, "%s", lbl);
            wattroff(g_win_graph, COLOR_PAIR(pair)|A_BOLD);
        }
    }
    wrefresh(g_win_graph);
}

void ui_draw_info(Graph *g, int level_fwd, int level_bwd,
                  int queue_fwd, int queue_bwd,
                  const char *src, const char *tgt) {
    werase(g_win_info);
    box(g_win_info, 0, 0);
    int row = 1;

    wattron(g_win_info, COLOR_PAIR(4)|A_BOLD);
    mvwprintw(g_win_info, row++, 2, "BIDIRECTIONAL BFS");
    wattroff(g_win_info, COLOR_PAIR(4)|A_BOLD);
    row++;

    wattron(g_win_info, COLOR_PAIR(2));
    mvwprintw(g_win_info, row++, 2, "SRC: %.40s", src);
    wattroff(g_win_info, COLOR_PAIR(2));
    wattron(g_win_info, COLOR_PAIR(5));
    mvwprintw(g_win_info, row++, 2, "TGT: %.40s", tgt);
    wattroff(g_win_info, COLOR_PAIR(5));
    row++;

    mvwprintw(g_win_info, row++, 2, "Nodes   : %d", g->count);
    int edges = 0;
    for (int i=0;i<g->count;i++) edges += g->adj[i].count;
    mvwprintw(g_win_info, row++, 2, "Edges   : %d", edges);
    row++;

    wattron(g_win_info, COLOR_PAIR(2));
    mvwprintw(g_win_info, row++, 2, "FWD lvl : %d  queue: %d", level_fwd, queue_fwd);
    wattroff(g_win_info, COLOR_PAIR(2));
    wattron(g_win_info, COLOR_PAIR(3));
    mvwprintw(g_win_info, row++, 2, "BWD lvl : %d  queue: %d", level_bwd, queue_bwd);
    wattroff(g_win_info, COLOR_PAIR(3));
    row++;

    /* Legend */
    wattron(g_win_info, A_BOLD);
    mvwprintw(g_win_info, row++, 2, "LEGEND:");
    wattroff(g_win_info, A_BOLD);
    wattron(g_win_info, COLOR_PAIR(8)|A_BOLD);
    mvwprintw(g_win_info, row,   2, "S"); wattroff(g_win_info, COLOR_PAIR(8)|A_BOLD);
    mvwprintw(g_win_info, row++, 4, " Start");
    wattron(g_win_info, COLOR_PAIR(9)|A_BOLD);
    mvwprintw(g_win_info, row,   2, "E"); wattroff(g_win_info, COLOR_PAIR(9)|A_BOLD);
    mvwprintw(g_win_info, row++, 4, " End");
    wattron(g_win_info, COLOR_PAIR(5)|A_BOLD);
    mvwprintw(g_win_info, row,   2, "@"); wattroff(g_win_info, COLOR_PAIR(5)|A_BOLD);
    mvwprintw(g_win_info, row++, 4, " Current scan");
    wattron(g_win_info, COLOR_PAIR(2)|A_BOLD);
    mvwprintw(g_win_info, row,   2, "o"); wattroff(g_win_info, COLOR_PAIR(2)|A_BOLD);
    mvwprintw(g_win_info, row++, 4, " Visited (fwd)");
    wattron(g_win_info, COLOR_PAIR(3)|A_BOLD);
    mvwprintw(g_win_info, row,   2, "o"); wattroff(g_win_info, COLOR_PAIR(3)|A_BOLD);
    mvwprintw(g_win_info, row++, 4, " Visited (bwd)");
    wattron(g_win_info, COLOR_PAIR(4)|A_BOLD);
    mvwprintw(g_win_info, row,   2, "*"); wattroff(g_win_info, COLOR_PAIR(4)|A_BOLD);
    mvwprintw(g_win_info, row++, 4, " Path");
    wattron(g_win_info, COLOR_PAIR(7)|A_BOLD);
    mvwprintw(g_win_info, row,   2, "+"); wattroff(g_win_info, COLOR_PAIR(7)|A_BOLD);
    mvwprintw(g_win_info, row++, 4, " In queue");

    wrefresh(g_win_info);
}

void ui_draw_log(void) {
    werase(g_win_log);
    box(g_win_log, 0, 0);
    wattron(g_win_log, A_BOLD);
    mvwprintw(g_win_log, 0, 2, " LOG ");
    wattroff(g_win_log, A_BOLD);
    int start = (g_log.head - g_log.count + LOG_LINES) % LOG_LINES;
    for (int i = 0; i < g_log.count; i++) {
        int idx = (start + i) % LOG_LINES;
        mvwprintw(g_win_log, i+1, 1, "%-*.*s",
                  SIDE_W-2, SIDE_W-2, g_log.lines[idx]);
    }
    wrefresh(g_win_log);
}

void ui_status(const char *msg) {
    int y = g_screen_h - 2;
    move(y, 0);
    clrtoeol();
    attron(COLOR_PAIR(6)|A_BOLD);
    mvprintw(y, 1, "%s", msg);
    attroff(COLOR_PAIR(6)|A_BOLD);
    refresh();
}

void ui_close(void) {
    delwin(g_win_graph);
    delwin(g_win_info);
    delwin(g_win_log);
    endwin();
}

/* ═══════════════════════════════════════
   BIDIRECTIONAL BFS
   O(B^(D/2)) — meets in the middle
   ═══════════════════════════════════════ */

/* Reconstruct path after the two frontiers meet at `mid` */
int *reconstruct_path(Graph *g, int mid, int *out_len) {
    /* Forward chain: mid → ... → src */
    int fwd[4096], fb = 0;
    for (int c = mid; c != -1; c = g->nodes[c].parent_fwd)
        fwd[fb++] = c;

    /* Backward chain: mid → ... → tgt */
    int bwd[4096], bb = 0;
    for (int c = mid; c != -1; c = g->nodes[c].parent_bwd)
        bwd[bb++] = c;

    /* total: fwd reversed + bwd (skip duplicate mid) */
    *out_len = fb + bb - 1;
    int *path = (*out_len > 0) ? malloc((size_t)*out_len * sizeof(int)) : malloc(sizeof(int));
    for (int i = 0; i < fb; i++)       path[i]      = fwd[fb-1-i];
    for (int i = 1; i < bb; i++)       path[fb-1+i] = bwd[i];
    return path;
}

/* Delay in milliseconds — lets ncurses update visible */
static void ms_sleep(int ms) {
    struct timespec ts = { 0, ms * 1000000L };
    nanosleep(&ts, NULL);
}

int bidir_bfs(Graph *g, int src_id, int tgt_id,
              int **path_out, int *path_len) {
    Queue *qF = queue_new();   /* forward  frontier */
    Queue *qB = queue_new();   /* backward frontier */

    g->nodes[src_id].dist_fwd = 0;
    g->nodes[src_id].state    = ST_START;
    g->nodes[tgt_id].dist_bwd = 0;
    g->nodes[tgt_id].state    = ST_END;

    queue_push(qF, src_id);
    queue_push(qB, tgt_id);

    int best = INT_MAX, mid = -1;
    int lvl_fwd = 0, lvl_bwd = 0;
    int nxt_fwd = 1, nxt_bwd = 1;   /* nodes left in current level */
    int new_fwd = 0, new_bwd = 0;

    char msg[128];

    while (!queue_empty(qF) || !queue_empty(qB)) {

        /* ── Step forward frontier ── */
        if (!queue_empty(qF)) {
            int cur = queue_pop(qF);
            nxt_fwd--;

            if (g->nodes[cur].state != ST_START)
                g->nodes[cur].state = ST_CURRENT;

            snprintf(msg, sizeof(msg), "[FWD L%d] %.80s", lvl_fwd,
                     g->nodes[cur].title);
            log_push(msg);

            int links = load_neighbors(g, cur);
            snprintf(msg, sizeof(msg), "   +-- %d links | nodes:%d",
                     links, g->count);
            log_push(msg);

            AdjList *a = &g->adj[cur];
            for (int i = 0; i < a->count; i++) {
                int nb = a->neighbors[i];
                if (g->nodes[nb].dist_fwd == -1) {
                    g->nodes[nb].dist_fwd   = lvl_fwd + 1;
                    g->nodes[nb].parent_fwd = cur;
                    if (g->nodes[nb].state == ST_UNKNOWN)
                        g->nodes[nb].state = ST_QUEUED_FWD;
                    queue_push(qF, nb);
                    new_fwd++;

                    /* Check if backward BFS already visited this node */
                    if (g->nodes[nb].dist_bwd != -1) {
                        int total = g->nodes[nb].dist_fwd + g->nodes[nb].dist_bwd;
                        if (total < best) { best = total; mid = nb; }
                    }
                }
            }

            if (g->nodes[cur].state == ST_CURRENT)
                g->nodes[cur].state = ST_VISITED_FWD;

            if (nxt_fwd == 0) {
                lvl_fwd++;
                nxt_fwd = new_fwd;
                new_fwd = 0;
            }
        }

        /* ── Step backward frontier ── */
        if (!queue_empty(qB)) {
            int cur = queue_pop(qB);
            nxt_bwd--;

            if (g->nodes[cur].state != ST_END)
                g->nodes[cur].state = ST_CURRENT;

            AdjList *a = &g->adj[cur];
            /* backward: also load neighbors (undirected treat) */
            load_neighbors(g, cur);
            a = &g->adj[cur]; /* reload pointer after possible realloc */

            for (int i = 0; i < a->count; i++) {
                int nb = a->neighbors[i];
                if (g->nodes[nb].dist_bwd == -1) {
                    g->nodes[nb].dist_bwd   = lvl_bwd + 1;
                    g->nodes[nb].parent_bwd = cur;
                    if (g->nodes[nb].state == ST_UNKNOWN ||
                        g->nodes[nb].state == ST_QUEUED_FWD)
                        g->nodes[nb].state = ST_QUEUED_BWD;
                    queue_push(qB, nb);
                    new_bwd++;

                    if (g->nodes[nb].dist_fwd != -1) {
                        int total = g->nodes[nb].dist_fwd + g->nodes[nb].dist_bwd;
                        if (total < best) { best = total; mid = nb; }
                    }
                }
            }

            if (g->nodes[cur].state == ST_CURRENT)
                g->nodes[cur].state = ST_VISITED_BWD;

            if (nxt_bwd == 0) {
                lvl_bwd++;
                nxt_bwd = new_bwd;
                new_bwd = 0;
            }
        }

        /* ── Draw ── */
        ui_draw_graph(g, NULL, 0);
        ui_draw_info(g, lvl_fwd, lvl_bwd,
                     qF->size, qB->size,
                     g->nodes[src_id].title,
                     g->nodes[tgt_id].title);
        ui_draw_log();
        snprintf(msg, sizeof(msg),
                 "  RUNNING  nodes:%d  best:%s  [q] quit",
                 g->count, best==INT_MAX?"none":"-");
        ui_status(msg);
        ms_sleep(80);

        /* early exit if path found and both frontiers advanced past it */
        if (mid != -1 &&
            lvl_fwd + lvl_bwd >= best) break;

        /* quit key */
        nodelay(stdscr, TRUE);
        int ch = getch();
        if (ch == 'q' || ch == 'Q') { mid = -1; break; }
    }

    queue_free(qF);
    queue_free(qB);

    if (mid == -1) { *path_out = NULL; *path_len = 0; return 0; }

    *path_out = reconstruct_path(g, mid, path_len);
    return 1;
}

/* ═══════════════════════════════════════
   MAIN
   ═══════════════════════════════════════ */
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <source> <target>\n", argv[0]);
        printf("  e.g. %s Dog Internet\n", argv[0]);
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    Graph *g   = graph_new();
    int src_id = graph_add(g, argv[1]);
    int tgt_id = graph_add(g, argv[2]);

    /* Pre-load source and target so both frontiers have edges */
    load_neighbors(g, src_id);
    load_neighbors(g, tgt_id);

    ui_init();

    int *path = NULL, plen = 0;
    int found = bidir_bfs(g, src_id, tgt_id, &path, &plen);

    /* Mark path nodes */
    if (found) {
        for (int i = 0; i < plen; i++) {
            int id = path[i];
            if      (id == src_id) g->nodes[id].state = ST_START;
            else if (id == tgt_id) g->nodes[id].state = ST_END;
            else                   g->nodes[id].state = ST_PATH;
        }
    }

    /* Final draw */
    ui_draw_graph(g, path, plen);
    ui_draw_info(g, 0, 0, 0, 0, argv[1], argv[2]);

    /* Print path to log */
    if (found) {
        log_push(">>> PATH FOUND <<<");
        char line[128] = "";
        for (int i = 0; i < plen; i++) {
            char seg[32];
            snprintf(seg, sizeof(seg), "%.14s%s",
                     g->nodes[path[i]].title,
                     i<plen-1?" -> ":"");
            if (strlen(line)+strlen(seg) < 126) strcat(line, seg);
            else { log_push(line); strcpy(line, seg); }
        }
        if (line[0]) log_push(line);
        char hops[64];
        snprintf(hops, sizeof(hops), "Hops: %d", plen-1);
        log_push(hops);
        ui_status("  PATH FOUND — press any key to exit");
    } else {
        log_push("NOT FOUND");
        ui_status("  NOT FOUND — press any key to exit");
    }

    ui_draw_log();
    nodelay(stdscr, FALSE);
    getch();

    ui_close();

    /* Print path to stdout after ncurses exits */
    if (found) {
        printf("\n+--------------------------------------------------+\n");
        printf("|  PATH FOUND  (%d hops)                          |\n", plen-1);
        printf("+--------------------------------------------------+\n");
        for (int i = 0; i < plen; i++) {
            if      (i == 0)      printf("  [START] %s\n", g->nodes[path[i]].title);
            else if (i == plen-1) printf("  [ END ] %s\n", g->nodes[path[i]].title);
            else                  printf("  [  %2d ] %s\n", i, g->nodes[path[i]].title);
            if (i < plen-1) printf("          |\n          v\n");
        }
        printf("\n  Graph: %d nodes\n", g->count);
    }

    free(path);
    graph_free(g);
    curl_global_cleanup();
    return found ? 0 : 1;
}