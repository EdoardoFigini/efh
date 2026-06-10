#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#endif

int g_realloc_count = 0;
static void* my_realloc(void* ptr, size_t count, size_t size) {
    g_realloc_count++;
#ifdef _WIN32
      return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, count * size);
#else
      return realloc(ptr, count * size);
#endif
}

#define SB_REALLOC(ptr, count, size) my_realloc(ptr, count, size)

#define SB_IMPLEMENTATION
#define SB_DEF static
#include "sb.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_sb_basic_append() {
    printf("Testing basic append...\n");
    sb_t sb = {0};

    sb_append(&sb, "Hello");
    assert(sb.len == 5);
    assert(memcmp(sb.data, "Hello", 5) == 0);

    sb_append(&sb, " World");
    assert(sb.len == 11);
    assert(memcmp(sb.data, "Hello World", 11) == 0);

    sb_free(&sb);
    assert(sb.data == NULL);
}

void test_sb_formatting() {
    printf("Testing formatted append...\n");
    sb_t sb = {0};

    int age = 25;
    const char* name = "John Doe";
    sb_appendf(&sb, "Name: %s, Age: %d", name, age);

    assert(sb.len == 23); 
    assert(strcmp(sb.data, "Name: John Doe, Age: 25") == 0);

    sb_free(&sb);
}

void test_sb_growth() {
    printf("Testing capacity growth...\n");
    sb_t sb = {0};

    // Force multiple reallocations by appending a long string
    const char* long_str = "0123456789";
    for (int i = 0; i < 20; i++) {
        sb_append(&sb, long_str);
    }

    assert(sb.len == 200);
    assert(sb.cap >= 200);
    // Ensure the bit-manipulation in sb_reserve results in power-of-two
    assert((sb.cap & (sb.cap - 1)) == 0);

    sb_free(&sb);
}

void test_sb_file_io() {
    printf("Testing file I/O...\n");
    sb_t out = {0};
    const char* filename = "test_sb_output.tmp";
    const char* content = "File System Test Content";

    sb_append(&out, content);
    int written = sb_write_to_file(filename, &out);
    assert(written > 0);

    sb_t in = {0};
    int read = sb_read_file(filename, &in);
    assert(read == (int)strlen(content));
    assert(memcmp(in.data, content, read) == 0);

    // Cleanup
    sb_free(&out);
    sb_free(&in);
    remove(filename);
}

void test_sb_reset_and_concat() {
    printf("Testing reset and concat...\n");
    sb_t a = {0};
    sb_t b = {0};

    sb_append(&a, "Part A");
    sb_append(&b, "Part B");

    sb_concat(&a, &b);
    assert(a.len == 12);
    assert(memcmp(a.data, "Part APart B", 12) == 0);

    sb_reset(&a);
    assert(a.len == 0);
    // data should still be allocated
    assert(a.data != NULL);

    sb_free(&a);
    sb_free(&b);
}

void test_sb_n_append() {
    printf("Testing sb_n_append...\n");
    sb_t sb = {0};
    sb_n_append(&sb, "Hello World", 5);
    assert(sb.len == 5);
    assert(memcmp(sb.data, "Hello", 5) == 0);
    sb_free(&sb);
}

void test_sb_appendln() {
    printf("Testing sb_appendln...\n");
    sb_t sb = {0};
    sb_appendln(&sb, "Line");
    assert(sb.len == 5);
    assert(sb.data[4] == '\n');
    assert(memcmp(sb.data, "Line\n", 5) == 0);
    sb_free(&sb);
}

void test_sb_appendz() {
    printf("Testing sb_appendz...\n");
    sb_t sb = {0};
    sb_appendz(&sb, "Zero");
    assert(sb.len == 5);
    assert(sb.data[4] == '\0');
    assert(memcmp(sb.data, "Zero\0", 5) == 0);
    sb_free(&sb);
}

void test_sb_append_to_file() {
    printf("Testing sb_append_to_file...\n");
    const char* filename = "test_sb_append_output.tmp";
    sb_t sb1 = {0};
    sb_append(&sb1, "First line\n");
    sb_write_to_file(filename, &sb1);

    sb_t sb2 = {0};
    sb_append(&sb2, "Second line");
    sb_append_to_file(filename, &sb2);

    sb_t in = {0};
    sb_read_file(filename, &in);
    assert(in.len == 22);
    assert(memcmp(in.data, "First line\nSecond line", 22) == 0);

    sb_free(&sb1);
    sb_free(&sb2);
    sb_free(&in);
    remove(filename);
}

void test_sb_reserve_power_of_two() {
    printf("Testing sb_reserve power of 2 behavior...\n");
    sb_t sb = {0};
    
    sb_reserve(&sb, 3);
    assert(sb.cap == 4);

    sb_reserve(&sb, 17);
    assert(sb.cap == 32);

    sb_reserve(&sb, 100);
    assert(sb.cap == 128);

    sb_reserve(&sb, 1025);
    assert(sb.cap == 2048);

    sb_free(&sb);
}

void test_sb_n_append_state_corruption() {
    printf("Testing sb_n_append state corruption on failure...\n");
    sb_t sb = {0};
    sb_append(&sb, "A"); 
    
    size_t old_cap = sb.cap;
    
    // Request a gigantic append that will fail allocation
    int res = sb_n_append(&sb, "B", ((size_t)-1) / 4);
    assert(res == -1); // Allocation should fail
    
    assert(sb.data != NULL);
    assert(sb.cap == old_cap); 
    
    sb_free(&sb);
}

void test_sb_appendf_preallocated() {
    printf("Testing sb_appendf on preallocated buffer...\n");
    sb_t sb = {0};
    sb_reserve(&sb, 100);
    
    int res = sb_appendf(&sb, "Hello %s", "World");
    assert(res == 11);
    assert(sb.len == 11);
    assert(strcmp(sb.data, "Hello World") == 0);

    sb_reset(&sb);

    res = sb_appendln(&sb, "Hello World");
    assert(res == 12);
    assert(sb.len == 12);
    assert(strcmp(sb.data, "Hello World\n") == 0);
    
    sb_free(&sb);
}

void test_sb_n_append_realloc_bug() {
    printf("Testing sb_n_append does not realloc unnecessarily...\n");
    sb_t sb = {0};
    
    sb_append(&sb, "A");
    int initial_reallocs = g_realloc_count;
    
    // Capacity is 128, so it shouldn't realloc.
    sb_append(&sb,  "B");
    sb_append(&sb, "C");
    sb_append(&sb,  "D");
    
    assert(g_realloc_count == initial_reallocs);
    
    sb_free(&sb);
}

void test_sb_oom_crashes() {
    printf("Testing sb_appendln and sb_appendf OOM...\n");
    sb_t sb = {0};
    sb_reserve(&sb, 16);
    
    // Artificially inflate len to force re-allocations to fail
    sb.len = ((size_t)-1) - 10;
    void* data = sb.data;
    size_t cap = sb.cap;
    
    assert(sb_appendln(&sb, "0123456789") == -1);
    // sb should remain unchanged
    assert(sb.len == ((size_t)-1) - 10);
    assert(data == sb.data);
    assert(cap == sb.cap);
    
    assert(sb_appendf(&sb, "012345678%d", 9) == -1);
    // sb should remain unchanged
    assert(sb.len == ((size_t)-1) - 10);
    assert(data == sb.data);
    assert(cap == sb.cap);
    
    sb.len = 0;
    sb_free(&sb);
}

int main() {
    test_sb_basic_append();
    test_sb_formatting();
    test_sb_growth();
    test_sb_file_io();
    test_sb_reset_and_concat();
    test_sb_n_append();
    test_sb_appendln();
    test_sb_appendz();
    test_sb_append_to_file();
    test_sb_reserve_power_of_two();
    test_sb_n_append_state_corruption();
    test_sb_appendf_preallocated();
    test_sb_n_append_realloc_bug();
    test_sb_oom_crashes();

    printf("\nAll sb.h tests passed successfully!\n");

    return 0;
}
