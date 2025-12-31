#include <stdio.h>
#include <stdlib.h>

#define NAME_LEN 30
#define GRADES_COUNT 5

typedef struct {
    int id;
    char name[NAME_LEN];
    int grades[GRADES_COUNT];
} Student;

void write_students_to_file(const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("Error opening file for writing!\n");
        return;
    }

}

Student* read_students_from_file(const char* filename, int* out_count) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        printf("Error opening file for reading!\n");
        *out_count = 0;
        return NULL;
    }
    
}

float calcAverage(const Student* s) {
    int sum = 0;
    for (int i = 0; i < GRADES_COUNT; i++)
        sum += s->grades[i];
    return sum / (float)GRADES_COUNT;
}


void print_students(const Student* arr, int count) {
    for (int i = 0; i < count; i++) {
        printf("ID: %d, Name: %s, Grades:", arr[i].id, arr[i].name);
        for (int j = 0; j < GRADES_COUNT; j++)
            printf(" %d", arr[i].grades[j]);
        printf(", Average: %.2f\n", calcAverage(&arr[i]));
    }
}


int main(void) {
    const char* filename = "students.txt";

    

    return 0;
}
