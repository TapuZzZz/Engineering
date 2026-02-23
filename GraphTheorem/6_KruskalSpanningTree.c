#include <stdio.h>
#include <stdlib.h>

struct Edge { int src, dest, weight; };
struct Graph { int V, E; struct Edge* edge; };

int find(int parent[], int i) {
    if (parent[i] == -1) return i;
    return find(parent, parent[i]);
}

void Union(int parent[], int x, int y) {
    parent[x] = y;
}

int compareEdges(const void* a, const void* b) {
    return ((struct Edge*)a)->weight - ((struct Edge*)b)->weight;
}

void KruskalMST(struct Graph* graph) {
    int V = graph->V;
    struct Edge result[V]; 
    int e = 0, i = 0;

    qsort(graph->edge, graph->E, sizeof(graph->edge[0]), compareEdges);

    int *parent = (int*) malloc(V * sizeof(int));
    for (int v = 0; v < V; v++) parent[v] = -1;

    while (e < V - 1 && i < graph->E) {
        struct Edge next_edge = graph->edge[i++];
        int x = find(parent, next_edge.src);
        int y = find(parent, next_edge.dest);

        if (x != y) {
            result[e++] = next_edge;
            Union(parent, x, y);
        }
    }

    for (i = 0; i < e; ++i)
        printf("%d -- %d == %d\n", result[i].src, result[i].dest, result[i].weight);
}