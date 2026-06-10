#define ARENA_IMPLEMENTATION
#include "arena.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_basic_alloc() {
    printf("Testing basic allocation...\n");
    arena_t arena = {0};

    void* ptr1 = arena_alloc(&arena, 100);
    assert(ptr1 != NULL);
    assert(arena.head != NULL);
    assert(arena.block_count == 1);

    void* ptr2 = arena_alloc(&arena, 200);
    assert(ptr2 != NULL);
    assert(arena.block_count == 1); // Should fit in the same block

    // Fill the memory to ensure it's writable
    memset(ptr1, 0xAA, 100);
    memset(ptr2, 0xBB, 200);

    arena_free(&arena);
    assert(arena.head == NULL);
    assert(arena.block_count == 0);
}

void test_multiple_blocks() {
    printf("Testing multiple block allocation...\n");
    arena_t arena = {0};

    // Assuming PAGE_SIZE is 4096 and ARENA_PAGES_PER_BLOCK is 1
    // Allocating chunks of 1024 bytes should eventually trigger new blocks.
    for (int i = 0; i < 10; i++) {
        void* ptr = arena_alloc(&arena, 1024);
        assert(ptr != NULL);
        memset(ptr, (char)i, 1024);
    }

    assert(arena.block_count > 1);

    arena_free(&arena);
}

void test_arena_strdup() {
    printf("Testing arena_strdup...\n");
    arena_t arena = {0};

    const char* original = "Hello, Arena!";
    char* copy = arena_strdup(&arena, original);
    
    assert(copy != NULL);
    assert(strcmp(copy, original) == 0);
    assert(copy != original); // Should be a deep copy

    arena_free(&arena);
}

void test_arena_sprintf() {
    printf("Testing arena_sprintf...\n");
    arena_t arena = {0};

    char* formatted = arena_sprintf(&arena, "The number is %d and the word is %s.", 42, "test");
    
    assert(formatted != NULL);
    assert(strcmp(formatted, "The number is 42 and the word is test.") == 0);

    // Test a long format that might require more memory
    char long_word[1024];
    memset(long_word, 'A', 1023);
    long_word[1023] = '\0';

    char* long_formatted = arena_sprintf(&arena, "Long word: %s", long_word);
    assert(long_formatted != NULL);
    assert(strlen(long_formatted) == 1023 + 11); // "Long word: " + 1023 A's

    arena_free(&arena);
}

void test_zero_alloc() {
    printf("Testing zero-byte allocation...\n");
    arena_t arena = {0};

    void* ptr = arena_alloc(&arena, 0);
    assert(ptr == NULL);
    
    arena_free(&arena);
}

typedef struct {
    int x;
    float y;
} push_test_struct_t;

void test_arena_push() {
    printf("Testing arena_push...\n");
    arena_t arena = {0};

    // 1. Variable
    int my_var = 123;
    int* p_var = arena_push(&arena, my_var);
    assert(*p_var == 123);

    // 2. Pointer
    const char* str = "hello";
    const char** p_str = arena_push(&arena, str);
    assert(strcmp(*p_str, "hello") == 0);

    // 3. Struct literal
    push_test_struct_t* p_struct = arena_push(&arena, ((push_test_struct_t){10, 3.14f}));
    assert(p_struct->x == 10);
    assert(p_struct->y == 3.14f);

    // 4. Number literal
    int* p_num = arena_push(&arena, 42);
    assert(*p_num == 42);

    arena_free(&arena);
}

int main() {
    test_basic_alloc();
    test_multiple_blocks();
    test_arena_strdup();
    test_arena_sprintf();
    test_zero_alloc();
    test_arena_push();

    printf("\nAll arena.h tests passed successfully!\n");
    return 0;
}
