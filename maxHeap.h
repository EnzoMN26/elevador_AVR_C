#ifndef MAXHEAP_H
#define MAXHEAP_H

void swap(int *a, int *b);


// Function to print the Heap as array

// will print as - 'message array[]\n'

void printArray(char message[], int arr[], int n);

// To heapify a subtree with node i as root

// Size of heap is n

void heapify(int arr[], int n, int i);


// Function to build a Max-Heap from a given array
void buildHeap(int arr[], int n);

#endif
