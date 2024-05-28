#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function declarations
void printMessage(const char *message);
int add(int a, int b);
double multiply(double a, double b);
char* concatenateStrings(const char *str1, const char *str2);
void printArray(int arr[], int size);
int findMax(int arr[], int size);

int main() {
    // Demonstrating printMessage function
    printMessage("Hello, world!");

    // Demonstrating add function
    int sum = add(5, 7);
    printf("Sum: %d\n", sum);

    // Demonstrating multiply function
    double product = multiply(3.5, 2.0);
    printf("Product: %.2f\n", product);

    // Demonstrating concatenateStrings function
    char *combined = concatenateStrings("Hello, ", "world!");
    printf("Concatenated String: %s\n", combined);
    free(combined);

    // Demonstrating printArray and findMax functions
    int numbers[] = {4, 7, 1, 8, 5, 9};
    int size = sizeof(numbers) / sizeof(numbers[0]);

    printArray(numbers, size);
    int max = findMax(numbers, size);
    printf("Maximum Value: %d\n", max);

    return 0;
}

// Function definitions

void printMessage(const char *message) {
    printf("%s\n", message);
}

int add(int a, int b) {
    return a + b;
}

double multiply(double a, double b) {
    return a * b;
}

char* concatenateStrings(const char *str1, const char *str2) {
    // Allocate memory for the concatenated string
    char *result = (char*)malloc(strlen(str1) + strlen(str2) + 1);
    if (result == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    // Concatenate the strings
    strcpy(result, str1);
    strcat(result, str2);

    return result;
}

void printArray(int arr[], int size) {
    printf("Array: ");
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int findMax(int arr[], int size) {
    int max = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}
