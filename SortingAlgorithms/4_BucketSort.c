#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ─── Node for linked-list buckets ───────────────────────────────────────────
typedef struct Node {
    float val;
    struct Node *next;
} Node;

// ─── Insertion sort on a linked list (O(k²) but k is tiny per bucket) ───────
static Node *insertSorted(Node *head, float val) {
    Node *n = malloc(sizeof(Node));
    n->val = val;
    if (!head || val < head->val) { n->next = head; return n; }
    Node *cur = head;
    while (cur->next && cur->next->val < val) cur = cur->next;
    n->next = cur->next;
    cur->next = n;
    return head;
}

// ─── Bucket Sort  (expects floats in [0, 1)) ─────────────────────────────────
void bucketSort(float *arr, int n) {
    if (n <= 1) return;

    Node **buckets = calloc(n, sizeof(Node *));

    // Distribute
    for (int i = 0; i < n; i++) {
        int idx = (int)(arr[i] * n);
        if (idx >= n) idx = n - 1;          // clamp edge case (val == 1.0)
        buckets[idx] = insertSorted(buckets[idx], arr[i]);
    }

    // Concatenate & free
    int pos = 0;
    for (int i = 0; i < n; i++) {
        Node *cur = buckets[i];
        while (cur) {
            arr[pos++] = cur->val;
            Node *tmp = cur;
            cur = cur->next;
            free(tmp);
        }
    }

    free(buckets);
}

// ─── Demo ────────────────────────────────────────────────────────────────────
int main(void) {
    float arr[] = {0.78f, 0.17f, 0.39f, 0.26f, 0.72f, 0.94f, 0.21f, 0.12f, 0.68f, 0.55f};
    int n = sizeof arr / sizeof *arr;

    printf("Before: ");
    for (int i = 0; i < n; i++) printf("%.2f ", arr[i]);

    bucketSort(arr, n);

    printf("\nAfter:  ");
    for (int i = 0; i < n; i++) printf("%.2f ", arr[i]);
    printf("\n");
    return 0;
}