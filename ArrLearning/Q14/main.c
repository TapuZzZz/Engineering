// .14 Write a program with functions.
//  The main program will contain: 
//  A. A function to receive an array of 10 elements. 
//  B. A function that prints the following menu: 
//      .1 Finding the largest number and how many times it appears. 
//      .2 Finding the smallest number and how many times it appears. 
//      .3 Sum of the elements of the array. 
//      .4 Checking whether the number the user has typed is found and if so, how many times. 
//      .5 Sorting the array. 
//      .6 Please type your choice. 
//  C. A switch statement that checks which key the user has typed and, based on the key, 
//  calls the function that fulfills the user's request.
#include <stdio.h>

// Function declarations
void inputArray(int arr[], int size);
void printMenu();
void findLargest(int arr[], int size);
void findSmallest(int arr[], int size);
void sumArray(int arr[], int size);
void searchNumber(int arr[], int size);
void sortArray(int arr[], int size);
void printArray(int arr[], int size);

int main(){
    int arr[10];
    int choice;
    
    // A. Function to receive an array of 10 elements
    printf("Please enter 10 numbers:\n");
    inputArray(arr, 10);
    
    do {
        // B. Function that prints the menu
        printMenu();
        scanf("%d", &choice);
        
        // C. Switch statement based on user choice
        switch(choice) {
            case 1:
                findLargest(arr, 10);
                break;
            case 2:
                findSmallest(arr, 10);
                break;
            case 3:
                sumArray(arr, 10);
                break;
            case 4:
                searchNumber(arr, 10);
                break;
            case 5:
                sortArray(arr, 10);
                break;
            case 6:
                printf("Goodbye!\n");
                break;
            default:
                printf("Invalid choice! Please try again.\n");
        }
        printf("\n");
    } while(choice != 6);

    return 0;
}

// Function to receive an array of 10 elements
void inputArray(int arr[], int size) {
    for(int i = 0; i < size; i++) {
        printf("Enter element %d: ", i + 1);
        scanf("%d", &arr[i]);
    }
    printf("\n");
}

// Function that prints the menu
void printMenu() {
    printf("================= MENU =================\n");
    printf("1. Finding the largest number and how many times it appears\n");
    printf("2. Finding the smallest number and how many times it appears\n");
    printf("3. Sum of the elements of the array\n");
    printf("4. Checking whether the number the user has typed is found and if so, how many times\n");
    printf("5. Sorting the array\n");
    printf("6. Exit\n");
    printf("Please type your choice: ");
}

// Function to find largest number and its frequency
void findLargest(int arr[], int size) {
    int largest = arr[0];
    int count = 0;
    
    // Find the largest number
    for(int i = 1; i < size; i++) {
        if(arr[i] > largest) {
            largest = arr[i];
        }
    }
    
    // Count how many times it appears
    for(int i = 0; i < size; i++) {
        if(arr[i] == largest) {
            count++;
        }
    }
    
    printf("Largest number: %d\n", largest);
    printf("It appears %d time(s)\n", count);
}

// Function to find smallest number and its frequency
void findSmallest(int arr[], int size) {
    int smallest = arr[0];
    int count = 0;
    
    // Find the smallest number
    for(int i = 1; i < size; i++) {
        if(arr[i] < smallest) {
            smallest = arr[i];
        }
    }
    
    // Count how many times it appears
    for(int i = 0; i < size; i++) {
        if(arr[i] == smallest) {
            count++;
        }
    }
    
    printf("Smallest number: %d\n", smallest);
    printf("It appears %d time(s)\n", count);
}

// Function to calculate sum of array elements
void sumArray(int arr[], int size) {
    int sum = 0;
    
    for(int i = 0; i < size; i++) {
        sum += arr[i];
    }
    
    printf("Sum of all elements: %d\n", sum);
}

// Function to search for a specific number
void searchNumber(int arr[], int size) {
    int searchNum, count = 0;
    
    printf("Enter the number to search for: ");
    scanf("%d", &searchNum);
    
    for(int i = 0; i < size; i++) {
        if(arr[i] == searchNum) {
            count++;
        }
    }
    
    if(count > 0) {
        printf("Number %d was found %d time(s)\n", searchNum, count);
    } else {
        printf("Number %d was not found in the array\n", searchNum);
    }
}

// Function to sort the array (bubble sort)
void sortArray(int arr[], int size) {
    int temp;
    
    printf("Array before sorting: ");
    printArray(arr, size);
    
    // Bubble sort algorithm
    for(int i = 0; i < size - 1; i++) {
        for(int j = 0; j < size - i - 1; j++) {
            if(arr[j] > arr[j + 1]) {
                // Swap elements
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
    
    printf("Array after sorting: ");
    printArray(arr, size);
}

// Helper function to print the array
void printArray(int arr[], int size) {
    for(int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}