#include "test.h"

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
#include <string.h>

bool sb_basic_append() {
    sb_t sb = {0};

    sb_append(&sb, "Hello");
    TEST_ASSERT(sb.len == 5);
    TEST_ASSERT(memcmp(sb.data, "Hello", 5) == 0);

    sb_append(&sb, " World");
    TEST_ASSERT(sb.len == 11);
    TEST_ASSERT(memcmp(sb.data, "Hello World", 11) == 0);

    sb_free(&sb);
    TEST_ASSERT(sb.data == NULL);

    return true;
}

bool sb_formatting() {
    sb_t sb = {0};

    int age = 25;
    const char* name = "John Doe";
    sb_appendf(&sb, "Name: %s, Age: %d", name, age);

    TEST_ASSERT(sb.len == 23); 
    TEST_ASSERT(strcmp(sb.data, "Name: John Doe, Age: 25") == 0);

    sb_free(&sb);

    return true;
}

bool sb_growth() {
    sb_t sb = {0};

    // Force multiple reallocations by appending a long string
    const char* long_str = "0123456789";
    for (int i = 0; i < 20; i++) {
        sb_append(&sb, long_str);
    }

    TEST_ASSERT(sb.len == 200);
    TEST_ASSERT(sb.cap >= 200);
    // Ensure the bit-manipulation in sb_reserve results in power-of-two
    TEST_ASSERT((sb.cap & (sb.cap - 1)) == 0);

    sb_free(&sb);

    return true;
}

bool sb_file_io() {
    sb_t out = {0};
    const char* filename = "test_sb_output.tmp";
    const char* content = "File System Test Content";

    sb_append(&out, content);
    int written = sb_write_to_file(filename, &out);
    TEST_ASSERT(written > 0);

    sb_t in = {0};
    int read = sb_read_file(filename, &in);
    TEST_ASSERT(read == (int)strlen(content));
    TEST_ASSERT(memcmp(in.data, content, read) == 0);

    // Cleanup
    sb_free(&out);
    sb_free(&in);
    remove(filename);

    return true;
}

bool sb_reset_and_concat() {
    sb_t a = {0};
    sb_t b = {0};

    sb_append(&a, "Part A");
    sb_append(&b, "Part B");

    sb_concat(&a, &b);
    TEST_ASSERT(a.len == 12);
    TEST_ASSERT(memcmp(a.data, "Part APart B", 12) == 0);

    sb_reset(&a);
    TEST_ASSERT(a.len == 0);
    // data should still be allocated
    TEST_ASSERT(a.data != NULL);

    sb_free(&a);
    sb_free(&b);

    return true;
}

bool test_sb_n_append() {
    sb_t sb = {0};
    sb_n_append(&sb, "Hello World", 5);
    TEST_ASSERT(sb.len == 5);
    TEST_ASSERT(memcmp(sb.data, "Hello", 5) == 0);
    sb_free(&sb);

    return true;
}

bool test_sb_appendln() {
    sb_t sb = {0};
    sb_appendln(&sb, "Line");
    TEST_ASSERT(sb.len == 5);
    TEST_ASSERT(sb.data[4] == '\n');
    TEST_ASSERT(memcmp(sb.data, "Line\n", 5) == 0);
    sb_free(&sb);

    return true;
}

bool test_sb_appendz() {
    sb_t sb = {0};
    sb_appendz(&sb, "Zero");
    TEST_ASSERT(sb.len == 5);
    TEST_ASSERT(sb.data[4] == '\0');
    TEST_ASSERT(memcmp(sb.data, "Zero\0", 5) == 0);
    sb_free(&sb);

    return true;
}

bool test_sb_append_to_file() {
    const char* filename = "test_sb_append_output.tmp";
    sb_t sb1 = {0};
    sb_append(&sb1, "First line\n");
    sb_write_to_file(filename, &sb1);

    sb_t sb2 = {0};
    sb_append(&sb2, "Second line");
    sb_append_to_file(filename, &sb2);

    sb_t in = {0};
    sb_read_file(filename, &in);
    TEST_ASSERT(in.len == 22);
    TEST_ASSERT(memcmp(in.data, "First line\nSecond line", 22) == 0);

    sb_free(&sb1);
    sb_free(&sb2);
    sb_free(&in);
    remove(filename);

    return true;
}

bool sb_reserve_power_of_two() {
    sb_t sb = {0};
    
    sb_reserve(&sb, 3);
    TEST_ASSERT(sb.cap == 4);

    sb_reserve(&sb, 17);
    TEST_ASSERT(sb.cap == 32);

    sb_reserve(&sb, 100);
    TEST_ASSERT(sb.cap == 128);

    sb_reserve(&sb, 1025);
    TEST_ASSERT(sb.cap == 2048);

    sb_free(&sb);

    return true;
}

bool sb_n_append_state_corruption() {
    sb_t sb = {0};
    sb_append(&sb, "A"); 
    
    size_t old_cap = sb.cap;
    
    // Request a gigantic append that will fail allocation
    int res = sb_n_append(&sb, "B", ((size_t)-1) / 4);
    TEST_ASSERT(res == -1); // Allocation should fail
    
    TEST_ASSERT(sb.data != NULL);
    TEST_ASSERT(sb.cap == old_cap); 
    
    sb_free(&sb);

    return true;
}

bool sb_appendf_preallocated() {
    sb_t sb = {0};
    sb_reserve(&sb, 100);
    
    int res = sb_appendf(&sb, "Hello %s", "World");
    TEST_ASSERT(res == 11);
    TEST_ASSERT(sb.len == 11);
    TEST_ASSERT(strcmp(sb.data, "Hello World") == 0);

    sb_reset(&sb);

    res = sb_appendln(&sb, "Hello World");
    TEST_ASSERT(res == 12);
    TEST_ASSERT(sb.len == 12);
    TEST_ASSERT(strcmp(sb.data, "Hello World\n") == 0);
    
    sb_free(&sb);

    return true;
}

bool sb_n_append_realloc_bug() {
    sb_t sb = {0};
    
    sb_append(&sb, "A");
    int initial_reallocs = g_realloc_count;
    
    // Capacity is 128, so it shouldn't realloc.
    sb_append(&sb,  "B");
    sb_append(&sb, "C");
    sb_append(&sb,  "D");
    
    TEST_ASSERT(g_realloc_count == initial_reallocs);
    
    sb_free(&sb);

    return true;
}

bool sb_oom_crashes() {
    sb_t sb = {0};
    sb_reserve(&sb, 16);
    
    // Artificially inflate len to force re-allocations to fail
    sb.len = ((size_t)-1) - 10;
    void* data = sb.data;
    size_t cap = sb.cap;
    
    TEST_ASSERT(sb_appendln(&sb, "0123456789") == -1);
    // sb should remain unchanged
    TEST_ASSERT(sb.len == ((size_t)-1) - 10);
    TEST_ASSERT(data == sb.data);
    TEST_ASSERT(cap == sb.cap);
    
    TEST_ASSERT(sb_appendf(&sb, "012345678%d", 9) == -1);
    // sb should remain unchanged
    TEST_ASSERT(sb.len == ((size_t)-1) - 10);
    TEST_ASSERT(data == sb.data);
    TEST_ASSERT(cap == sb.cap);
    
    sb.len = 0;
    sb_free(&sb);

    return true;
}

int main() {
    TEST(sb_basic_append);
    TEST(sb_formatting);
    TEST(sb_growth);
    TEST(sb_file_io);
    TEST(sb_reset_and_concat);
    TEST(test_sb_n_append);
    TEST(test_sb_appendln);
    TEST(test_sb_appendz);
    TEST(test_sb_append_to_file);
    TEST(sb_reserve_power_of_two);
    TEST(sb_n_append_state_corruption);
    TEST(sb_appendf_preallocated);
    TEST(sb_n_append_realloc_bug);
    TEST(sb_oom_crashes);

    REPORT("sb.h");
    return 0;
}
