#pragma once

#include <stddef.h>


void *myMemcpy(void *dest, const void *src, size_t n);
void *myMalloc(size_t size);
void *myRealloc(void *ptr, size_t size);
void *myCalloc(size_t nmemb, size_t size);
void myFree(void *ptr);
int myPosix_memalign(void **memptr, size_t alignment, size_t size);