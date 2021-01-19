# Dynamic Storage Allocator

Task: Create a dynamic storage allocator for C programs, i.e., your own version of the malloc, free and realloc routines. This should be done without invoking any memory-management related library calls or system calls. This excludes the use of malloc, calloc, free, realloc, sbrk, brk, or any variants of these calls in your code. The use if any global or static compound data structures such as arrays, structs, trees, or lists are not allowed.

Tech Stack Summary: This program was written entirely in C. The memlib.c package simulates the memory system for the dynamic memory allocator. It allows the following functions to be invoked: mem_sbrk(int), mem_heap_lo(), mem_heap_hi(), mem_heapsize(), mem_pagesize(). The allocator always returns pointers that are 8-byte aligned as well.
