#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    struct Node* next;
} Node;

// אתחול - ה-last מצביע ל-NULL
void Init_CLLL(Node **last) {
    *last = NULL;
}

int Isempty_CLLL(Node *last) {
    return (last == NULL);
}

// Push ברשימה מעגלית: מוסיף איבר שהופך להיות ה"ראש" (הבא אחרי ה-last)
void Push_CLLL(Node **last, int value) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (!newNode) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    newNode->data = value;

    if (*last == NULL) {
        // מקרה של רשימה ריקה
        newNode->next = newNode;
        *last = newNode;
    } else {
        // האיבר החדש מצביע לראש הנוכחי
        newNode->next = (*last)->next;
        // האחרון מצביע לחדש (שהופך להיות הראש החדש)
        (*last)->next = newNode;
    }
}

// הוספה אחרי צומת ספציפי
void Insert_after_CLLL(Node **last, Node *prevNode, int value) {
    if (prevNode == NULL) return;

    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->data = value;
    
    newNode->next = prevNode->next;
    prevNode->next = newNode;

    // תיקון חשוב: אם הוספנו איבר אחרי ה-last הנוכחי, 
    // האיבר החדש הוא עכשיו ה-last החדש!
    if (prevNode == *last) {
        *last = newNode;
    }
}

// מחיקת האיבר הראשון (זה שנמצא ב-last->next)
int PopCLLL(Node **last) {
    if (*last == NULL) return -1; // רשימה ריקה

    Node *head = (*last)->next;
    int poppedValue = head->data;

    if (head == *last) {
        // מקרה של איבר אחרון ברשימה
        free(head);
        *last = NULL;
    } else {
        // ניתוק הראש והעברת הצבעת ה-last לאיבר הבא
        (*last)->next = head->next;
        free(head);
    }

    return poppedValue;
}

// מחיקה אחרי צומת מסוים
int Delete_afterCLLL(Node **last, Node *prevNode) {
    if (prevNode == NULL || prevNode->next == prevNode && prevNode != *last) 
        return -1;

    Node *nodeToDelete = prevNode->next;
    int deletedValue = nodeToDelete->data;

    if (nodeToDelete == *last) {
        *last = prevNode;
    }

    if (nodeToDelete == prevNode) {
        *last = NULL;
    } else {
        prevNode->next = nodeToDelete->next;
    }

    free(nodeToDelete);
    return deletedValue;
}

int main(void) {
    Node *last;
    Init_CLLL(&last);

    Push_CLLL(&last, 10);
    Push_CLLL(&last, 20);
    
    printf("Popped: %d\n", PopCLLL(&last));
    
    return 0;
}
