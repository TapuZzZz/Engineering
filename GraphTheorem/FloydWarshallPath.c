#include <stdio.h>

#define INF 999
#define V 4

void printPath(int P[V][V], int i, int j) {
    if (P[i][j] == -1) return;
    printPath(P, i, P[i][j]);
    printf("%d ", P[i][j]);
}

void floydWarshall(int graph[V][V]) {
    int dist[V][V], P[V][V], i, j, k;

    for (i = 0; i < V; i++) {
        for (j = 0; j < V; j++) {
            dist[i][j] = graph[i][j];
            P[i][j] = -1;
        }
    }

    for (k = 0; k < V; k++) {
        for (i = 0; i < V; i++) {
            for (j = 0; j < V; j++) {
                if (dist[i][k] != INF && dist[k][j] != INF && dist[i][k] + dist[k][j] < dist[i][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    P[i][j] = k;
                }
            }
        }
    }

    int start = 0, end = 2;
    printf("Shortest distance: %d\n", dist[start][end]);
    printf("Path: %d ", start);
    printPath(P, start, end);
    printf("%d\n", end);
}

int main() {
    int graph[V][V] = { {0,   5,  INF, 10},
                        {INF, 0,   3,  INF},
                        {INF, INF, 0,   1},
                        {INF, INF, INF, 0} };
    floydWarshall(graph);
    return 0;
}