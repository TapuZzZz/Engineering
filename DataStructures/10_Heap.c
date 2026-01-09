#include <stdio.h>
#include <stdlib.h>

typedef struct Heap {
    int* vec;
    int heapsize;
    int vecsize;
} Heap;

void Init_Heap(Heap* heap) {
    heap->vec = NULL;
    heap->heapsize = 0;
    heap->vecsize = 0;
}

int findLeftSon(unsigned short currOffset){
	return (currOffset*2);
}
	
int findRightSon(unsigned short currOffset){
	return ((currOffset*2)+1);
}

int findParent(unsigned short currOffset){
	return (currOffset/2);
}

void Heapify(Heap* heap, int offset) {
    int largest = offset;
    int left = findLeftSon(offset);
    int right = findRightSon(offset);

    if (left < heap->heapsize && heap->vec[left] > heap->vec[largest]) {
        largest = left;
    }

    if (right < heap->heapsize && heap->vec[right] > heap->vec[largest]) {
        largest = right;
    }

    if (largest != offset) {
        Swap(heap, offset, largest);
        Heapify(heap, largest);
    }
}

void Swap(Heap* heap, int offset1, int offset2){
    int temp = heap->vec[offset2];
    heap->vec[offset2] = heap->vec[offset1];
    heap->vec[offset1] = temp;
}