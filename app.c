#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int n;           
    int* Vertices;   
    int* ADJ;        
} graph;

// פונקציה לביצוע כפל מטריצות בינארי: C = A * B
void multiply(int* A, int* B, int* C, int n) {
    // זמנית נשמור את התוצאה במערך עזר כדי לא לדרוס נתונים תוך כדי חישוב
    int* temp = (int*)calloc(n * n, sizeof(int));
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                // אם יש מסלול מ-i ל-k ומ-k ל-j
                if (A[i * n + k] == 1 && B[k * n + j] == 1) {
                    temp[i * n + j] = 1;
                    break; // מספיק מסלול אחד כדי לקבוע שיש קשר
                }
            }
        }
    }
    
    // העתקת התוצאה למטריצה C
    for (int i = 0; i < n * n; i++) C[i] = temp[i];
    free(temp);
}

// הפונקציה המבוקשת: מקבלת גרף ומספר צעדים k
int* PATH_K(graph g, int k) {
    int n = g.n;
    if (k <= 0) return NULL;

    // הקצאת זיכרון למטריצת התוצאה
    int* result = (int*)malloc(n * n * sizeof(int));
    
    // התחלה: העתקת ADJ המקורי (אורך 1)
    for (int i = 0; i < n * n; i++) {
        result[i] = g.ADJ[i];
    }

    // ביצוע כפל k-1 פעמים כדי להגיע לאורך k
    for (int step = 1; step < k; step++) {
        multiply(result, g.ADJ, result, n);
    }

    return result;
}

void printMatrix(int* mat, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", mat[i * n + j]);
        }
        printf("\n");
    }
}

void main() {
    int nodes[] = {1, 2, 3, 4};
    int adjMatrix[] = {
        0, 1, 1, 0,
        1, 0, 0, 1,
        0, 0, 0, 1,
        0, 0, 1, 0,
    };

    graph g = {4, nodes, adjMatrix};
    
    int k = 2; // שנה כאן ל-3 או לכל מספר אחר
    int* pathK = PATH_K(g, k);

    printf("Matrix for path length %d:\n", k);
    printMatrix(pathK, g.n);

    free(pathK);
}