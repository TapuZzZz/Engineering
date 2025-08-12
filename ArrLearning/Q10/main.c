// 10. Define an integer array with 10 elements and initialize it as you wish. 
// Write a program that sorts the array from smallest to largest.
#include <stdio.h>

int arr[10] = {5, 2, 9, 1, 7, 4, 6, 3, 8, 0};

void print_array(int a[], int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", a[i]);
    }
    printf("\n");
}

void bubble_sort(int a[], int size) {
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (a[j] > a[j + 1]) {
                int temp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = temp;
            }
        }
    }
}

void selection_sort(int a[], int size) {
    for (int i = 0; i < size - 1; i++) {
        int min_idx = i;
        for (int j = i + 1; j < size; j++) {
            if (a[j] < a[min_idx]) {
                min_idx = j;
            }
        }
        int temp = a[i];
        a[i] = a[min_idx];
        a[min_idx] = temp;
    }
}

void insertion_sort(int a[], int size) {
    for (int i = 1; i < size; i++) {
        int key = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > key) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

int main() {
    // Efficiency comparison:
    // Bubble sort: O(n^2), slowest in practice.
    // Selection sort: O(n^2), usually a bit faster than bubble sort.
    // Insertion sort: O(n^2), but much faster for nearly sorted arrays and small arrays.
    // For small arrays like this, insertion sort is usually the fastest.

    int arr_bubble[10], arr_selection[10], arr_insertion[10];
    for (int i = 0; i < 10; i++) {
        arr_bubble[i] = arr[i];
        arr_selection[i] = arr[i];
        arr_insertion[i] = arr[i];
    }

    bubble_sort(arr_bubble, 10);
    printf("Bubble sort: ");
    print_array(arr_bubble, 10);

    selection_sort(arr_selection, 10);
    printf("Selection sort: ");
    print_array(arr_selection, 10);

    insertion_sort(arr_insertion, 10);
    printf("Insertion sort: ");
    print_array(arr_insertion, 10);

    return 0;
}