#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned int productId;
    char *productName;
    float productPrice;
    unsigned int productQuantity;
} product, *productPtr;

typedef struct {
    unsigned int categoryId;
    char *categoryName;
    unsigned int categoryQuantity;
    productPtr next;
} category, *categoryPtr;

typedef struct {
    unsigned int stockId;
    unsigned int stockQuantity;
    categoryPtr next;
}Stock, *StockPtr;

typedef enum {
    Fail,
    Sucsess
} statusType;

StockPtr CompanyStock = NULL; 
// ------------------------------------
statusType initCategory(char *categoryName){
    categoryPtr newCategory = (categoryPtr)malloc(sizeof(category));
    newCategory->categoryId = ++CompanyStock->stockQuantity;
    newCategory->categoryName = (char *)malloc(strlen(categoryName) + 1);
    strcpy(newCategory->categoryName, categoryName);
    return Sucsess;
}

int main() {
    return 0;
}