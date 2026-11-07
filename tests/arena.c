#include "test.h"

#define ARENA_IMPLEMENTATION
#include "arena.h"

#include <stdio.h>
#include <string.h>

bool basic_alloc() {
    arena_t arena = {0};

    void* ptr1 = arena_alloc(&arena, 100);
    TEST_ASSERT(ptr1 != NULL);
    TEST_ASSERT(arena.head != NULL);
    TEST_ASSERT(arena.block_count == 1);

    void* ptr2 = arena_alloc(&arena, 200);
    TEST_ASSERT(ptr2 != NULL);
    TEST_ASSERT(arena.block_count == 1); // Should fit in the same block

    // Fill the memory to ensure it's writable
    memset(ptr1, 0xAA, 100);
    memset(ptr2, 0xBB, 200);

    arena_free(&arena);
    TEST_ASSERT(arena.head == NULL);
    TEST_ASSERT(arena.block_count == 0);

    return true;
}

bool multiple_blocks() {
    arena_t arena = {0};

    // Assuming PAGE_SIZE is 4096 and ARENA_PAGES_PER_BLOCK is 1
    // Allocating chunks of 1024 bytes should eventually trigger new blocks.
    for (int i = 0; i < 10; i++) {
        void* ptr = arena_alloc(&arena, 1024);
        TEST_ASSERT(ptr != NULL);
        memset(ptr, (char)i, 1024);
    }

    TEST_ASSERT(arena.block_count > 1);

    arena_free(&arena);

    return true;
}

bool test_arena_strdup() {
    arena_t arena = {0};

    const char* original = "Hello, Arena!";
    char* copy = arena_strdup(&arena, original);
    
    TEST_ASSERT(copy != NULL);
    TEST_ASSERT(strcmp(copy, original) == 0);
    TEST_ASSERT(copy != original); // Should be a deep copy

    arena_free(&arena);

    return true;
}

bool test_arena_sprintf() {
    arena_t arena = {0};

    char* formatted = arena_sprintf(&arena, "The number is %d and the word is %s.", 42, "test");
    
    TEST_ASSERT(formatted != NULL);
    TEST_ASSERT(strcmp(formatted, "The number is 42 and the word is test.") == 0);

    // Test a long format that might require more memory
    char long_word[1024];
    memset(long_word, 'A', 1023);
    long_word[1023] = '\0';

    char* long_formatted = arena_sprintf(&arena, "Long word: %s", long_word);
    TEST_ASSERT(long_formatted != NULL);
    TEST_ASSERT(strlen(long_formatted) == 1023 + 11); // "Long word: " + 1023 A's

    arena_free(&arena);

    return true;
}

bool zero_alloc() {
    arena_t arena = {0};

    void* ptr = arena_alloc(&arena, 0);
    TEST_ASSERT(ptr == NULL);
    
    arena_free(&arena);

    return true;
}

typedef struct {
    int x;
    float y;
} push_struct_t;

bool test_arena_push() {
    arena_t arena = {0};

    // 1. Variable
    int my_var = 123;
    int* p_var = arena_push(&arena, my_var);
    TEST_ASSERT(*p_var == 123);

    // 2. Pointer
    const char* str = "hello";
    const char** p_str = arena_push(&arena, str);
    TEST_ASSERT(strcmp(*p_str, "hello") == 0);

    // 3. Struct literal
    push_struct_t* p_struct = arena_push(&arena, ((push_struct_t){10, 3.14f}));
    TEST_ASSERT(p_struct->x == 10);
    TEST_ASSERT(p_struct->y == 3.14f);

    // 4. Number literal
    int* p_num = arena_push(&arena, 42);
    TEST_ASSERT(*p_num == 42);

    arena_free(&arena);

    return true;
}

int main() {
    TEST(basic_alloc);
    TEST(multiple_blocks);
    TEST(test_arena_strdup);
    TEST(test_arena_sprintf);
    TEST(zero_alloc);
    TEST(test_arena_push);

    REPORT("arena.h");
    return 0;
}
