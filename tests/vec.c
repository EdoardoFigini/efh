#define VEC_IMPLEMENTATION
#include "vec.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// --- Helper Types ---

typedef struct {
    VEC_HEADER(const char*);
} cstrs_t;

typedef VEC(int) ints_t;

// --- Comparators ---

int compare_ints(const void* a, const void* b) {
    int arg1 = *(const int*)a;
    int arg2 = *(const int*)b;
    return (arg1 > arg2) - (arg1 < arg2);
}

int compare_cstrs(const void* a, const void* b) {
    // Note: vec_sort passes pointers to the items. 
    // Since items are const char*, we are receiving const char**
    return strcmp(*(const char**)a, *(const char**)b);
}

// --- Test Cases ---

void test_vec_basic_push_pop() {
    printf("Testing basic push and get...\n");
    ints_t nums = { 0 };
    
    vec_push(&nums, 10);
    vec_push(&nums, 20);
    vec_push(&nums, 30);

    assert(vec_len(&nums) == 3);
    assert(vec_get(&nums, 0) == 10);
    assert(vec_get(&nums, 1) == 20);
    assert(vec_get(&nums, 2) == 30);

    vec_clear(&nums);
    assert(vec_len(&nums) == 0);
}

void test_vec_sorting() {
    printf("Testing vector sorting...\n");
    ints_t nums = { 0 };
    vec_push(&nums, 67);
    vec_push(&nums, 80085);
    vec_push(&nums, 420);

    vec_sort(&nums, compare_ints);

    assert(vec_get(&nums, 0) == 67);
    assert(vec_get(&nums, 1) == 420);
    assert(vec_get(&nums, 2) == 80085);

    vec_clear(&nums);
}

void test_vec_removal() {
    printf("Testing removal (ordered vs unordered)...\n");
    ints_t nums = { 0 };
    vec_push(&nums, 1);
    vec_push(&nums, 2);
    vec_push(&nums, 3);
    vec_push(&nums, 4);

    // Ordered remove of '2' (index 1)
    // Result should be [1, 3, 4]
    vec_remove(&nums, 1);
    assert(vec_len(&nums) == 3);
    assert(vec_get(&nums, 1) == 3); 

    // Unordered remove of '1' (index 0)
    // Should swap last element (4) into index 0
    // Result should be [4, 3]
    vec_remove_unordered(&nums, 0);
    assert(vec_len(&nums) == 2);
    assert(vec_get(&nums, 0) == 4);

    vec_clear(&nums);
}

void test_vec_string_manipulation() {
    printf("Testing string vector operations...\n");
    cstrs_t strings = { 0 };
    
    vec_push(&strings, "Zebra");
    vec_push(&strings, "Apple");
    vec_push(&strings, "Mango");

    vec_sort(&strings, compare_cstrs);

    assert(strcmp(vec_get(&strings, 0), "Apple") == 0);
    assert(strcmp(vec_get(&strings, 1), "Mango") == 0);
    assert(strcmp(vec_get(&strings, 2), "Zebra") == 0);

    // Test modification via vec_get
    vec_get(&strings, 1) = "Banana";
    assert(strcmp(vec_get(&strings, 1), "Banana") == 0);

    vec_clear(&strings);
}

void test_vec_copy() {
  printf("Testing vec copy...\n");
  ints_t nums = { 0 };
  vec_push(&nums, 67);
  vec_push(&nums, 80085);
  vec_push(&nums, 420);

  ints_t copy = { 0 };

  vec_copy(&copy, &nums);


  assert(vec_len(&nums) == vec_len(&copy));
  assert(vec_len(&nums) == vec_cap(&copy));
  assert(vec_get(&nums, 0) == vec_get(&copy, 0));
  assert(vec_get(&nums, 1) == vec_get(&copy, 1));
  assert(vec_get(&nums, 2) == vec_get(&copy, 2));

  VEC(int) src = {0};
  vec_reserve(&src, 100); // Create large artificial capacity
  vec_push(&src, 1);
  vec_push(&src, 2);
  vec_push(&src, 3);
  
  VEC(int) dst = {0};
  vec_copy(&dst, &src);
  
  assert(dst.len == src.len);
  assert(dst.cap == src.len);
  assert(dst.cap == dst.len);
  
  vec_clear(&src);
  vec_clear(&dst);
}

void test_vec_indexof() {
    printf("Testing vec_indexof...\n");
    ints_t nums = { 0 };
    vec_push(&nums, 10);
    vec_push(&nums, 20);
    vec_push(&nums, 30);
    
    int* ptr = &vec_get(&nums, 1);
    // This should return 1, since ptr points to the 2nd element
    size_t idx = vec_indexof(&nums, ptr);
    assert(idx == 1);
    
    vec_clear(&nums);
}

void test_vec_extra_ops() {
    printf("Testing extra operations (pop, first, last, is_empty, set, swap, etc)...\n");
    ints_t nums = { 0 };
    
    assert(vec_is_empty(&nums));
    
    vec_push(&nums, 10);
    vec_push(&nums, 20);
    vec_push(&nums, 30);
    
    assert(!vec_is_empty(&nums));
    
    assert(vec_first(&nums) == 10);
    assert(vec_last(&nums) == 30);
    
    int popped = vec_pop(&nums);
    assert(popped == 30);
    assert(vec_len(&nums) == 2);
    assert(vec_last(&nums) == 20);
    
    vec_set(&nums, 0, 15);
    assert(vec_get(&nums, 0) == 15);
    
    vec_push(&nums, 25);
    // [15, 20, 25]
    vec_swap(&nums, 0, 2);
    // [25, 20, 15]
    assert(vec_get(&nums, 0) == 25);
    assert(vec_get(&nums, 2) == 15);
    
    vec_reset(&nums);
    assert(vec_is_empty(&nums));
    // reset leaves cap intact
    assert(vec_cap(&nums) > 0);
    
    vec_clear(&nums);
}

void test_vec_capacity_and_sizes() {
    printf("Testing capacity, shrink, and sizes...\n");
    ints_t nums = { 0 };
    
    assert(vec_cap(&nums) == 0);
    vec_reserve(&nums, 10);
    assert(vec_cap(&nums) >= 10);
    
    vec_push(&nums, 1);
    vec_push(&nums, 2);
    vec_push(&nums, 3);
    
    size_t old_cap = vec_cap(&nums);
    vec_shrink(&nums);
    assert(vec_cap(&nums) == 3);
    assert(vec_cap(&nums) < old_cap);
    
    assert(vec_elem_size(&nums) == sizeof(int));
    assert(vec_data_size(&nums) == 3 * sizeof(int));
    assert(vec_reserved_size(&nums) == vec_cap(&nums) * sizeof(int));
    assert(vec_data(&nums) != NULL);
    assert(vec_len(&nums) == 3);
    
    vec_clear(&nums);
}

void test_vec_foreach() {
    printf("Testing foreach...\n");
    ints_t nums = { 0 };
    
    vec_push(&nums, 1);
    vec_push(&nums, 2);
    vec_push(&nums, 3);
    
    int sum = 0;
    vec_foreach(it, &nums) {
        sum += *it;
    }
    assert(sum == 6);
    
    vec_clear(&nums);
}

// --- Helper for function-call-as-index tests ---

static int call_count = 0;

int get_index(int idx) {
    call_count++;
    return idx;
}

// --- Corner-case tests ---

// vec_get evaluates idx twice: once for the bounds check, once for the access.
// Passing a pre-increment should only advance the counter once.
void test_vec_get_no_double_eval() {
    ints_t nums = { 0 };
    vec_push(&nums, 100);
    vec_push(&nums, 200);
    vec_push(&nums, 300);

    int i = 0;
    int val = vec_get(&nums, i++);
    assert(val == 100);
    assert(i == 1);

    vec_clear(&nums);
}

// Same test but using a function call to observe side effects.
void test_vec_get_function_call_idx() {
    ints_t nums = { 0 };
    vec_push(&nums, 10);
    vec_push(&nums, 20);
    vec_push(&nums, 30);

    call_count = 0;
    int val = vec_get(&nums, get_index(1));
    assert(val == 20);
    assert(call_count == 1);

    vec_clear(&nums);
}

// vec_set also goes through vec_get, so the same double-eval applies.
void test_vec_set_no_double_eval() {
    ints_t nums = { 0 };
    vec_push(&nums, 10);
    vec_push(&nums, 20);
    vec_push(&nums, 30);

    int i = 1;
    vec_set(&nums, i++, 99);
    assert(vec_get(&nums, 1) == 99);
    assert(vec_get(&nums, 2) == 30);
    assert(i == 2);

    vec_clear(&nums);
}

void test_vec_set_function_call_idx() {
    ints_t nums = { 0 };
    vec_push(&nums, 10);
    vec_push(&nums, 20);

    call_count = 0;
    vec_set(&nums, get_index(0), 42);
    assert(vec_get(&nums, 0) == 42);
    assert(call_count == 1);

    vec_clear(&nums);
}

// vec_swap, vec_remove_unordered, vec_copy all use do{...}while(0)
// These tests verify that macros work inside single-statement if/else.
void test_vec_swap_in_single_stmt_if() {
    ints_t nums = { 0 };
    vec_push(&nums, 1);
    vec_push(&nums, 2);
    vec_push(&nums, 3);

    int do_swap = 1;
    if (do_swap)
        vec_swap(&nums, 0, 2);
    else
        vec_swap(&nums, 0, 1);

    assert(vec_get(&nums, 0) == 3);
    assert(vec_get(&nums, 2) == 1);

    vec_clear(&nums);
}

void test_vec_remove_unordered_in_single_stmt_if() {
    ints_t nums = { 0 };
    vec_push(&nums, 10);
    vec_push(&nums, 20);
    vec_push(&nums, 30);

    int do_remove = 1;
    if (do_remove)
        vec_remove_unordered(&nums, 0);
    else
        vec_remove_unordered(&nums, 1);

    assert(vec_len(&nums) == 2);

    vec_clear(&nums);
}

void test_vec_copy_in_single_stmt_if() {
    ints_t nums = { 0 };
    vec_push(&nums, 5);
    vec_push(&nums, 6);

    ints_t dst = { 0 };
    ints_t alt = { 0 };

    int use_dst = 1;
    if (use_dst)
        vec_copy(&dst, &nums);
    else
        vec_copy(&alt, &nums);

    assert(vec_len(&dst) == 2);
    assert(vec_get(&dst, 0) == 5);

    vec_clear(&nums);
    vec_clear(&dst);
    vec_clear(&alt);
}

// vec_push should also work inside single-statement if (it's an expression,
// but let's verify it doesn't have a trailing semicolon issue).
void test_vec_push_in_single_stmt_if() {
    printf("Testing vec_push in single-statement if/else...\n");
    ints_t nums = { 0 };

    int flag = 1;
    if (flag)
        vec_push(&nums, 42);
    else
        vec_push(&nums, 99);

    assert(vec_len(&nums) == 1);
    assert(vec_get(&nums, 0) == 42);

    vec_clear(&nums);
}

// vec_set should also work inside single-statement if.
void test_vec_set_in_single_stmt_if() {
    printf("Testing vec_set in single-statement if/else...\n");
    ints_t nums = { 0 };
    vec_push(&nums, 0);

    int flag = 1;
    if (flag)
        vec_set(&nums, 0, 77);
    else
        vec_set(&nums, 0, 88);

    assert(vec_get(&nums, 0) == 77);

    vec_clear(&nums);
}

// Pop all elements, verify count goes to 0 correctly.
void test_vec_pop_drain() {
    printf("Testing popping all elements...\n");
    ints_t nums = { 0 };
    vec_push(&nums, 1);
    vec_push(&nums, 2);
    vec_push(&nums, 3);

    assert(vec_pop(&nums) == 3);
    assert(vec_pop(&nums) == 2);
    assert(vec_pop(&nums) == 1);
    assert(vec_is_empty(&nums));
    assert(vec_len(&nums) == 0);

    vec_clear(&nums);
}

// Use vec_remove with a function-call index.
void test_vec_remove_function_call_idx() {
    printf("Testing vec_remove with function-call idx...\n");
    ints_t nums = { 0 };
    vec_push(&nums, 10);
    vec_push(&nums, 20);
    vec_push(&nums, 30);

    call_count = 0;
    vec_remove(&nums, get_index(1));
    // Should remove 20, result: [10, 30]
    assert(vec_len(&nums) == 2);
    assert(vec_get(&nums, 0) == 10);
    assert(vec_get(&nums, 1) == 30);
    assert(call_count == 1);

    vec_clear(&nums);
}

// Test that vec_swap with pre-increment indices doesn't double-evaluate.
// vec_swap caches idxa/idxb with __typeof__, so this should be safe.
void test_vec_swap_no_double_eval() {
    printf("Testing vec_swap does not double-evaluate idx...\n");
    ints_t nums = { 0 };
    vec_push(&nums, 100);
    vec_push(&nums, 200);
    vec_push(&nums, 300);

    int a = 0, b = 1;
    // Should swap indices 0 and 2.  a++ evaluates to 0, b++ evaluates to 1.
    // If double-evaluated, we'd swap wrong indices.
    vec_swap(&nums, a++, b++ + 1);
    assert(a == 1);
    assert(b == 2);
    assert(vec_get(&nums, 0) == 300);
    assert(vec_get(&nums, 2) == 100);

    vec_clear(&nums);
}

// Test vec_remove_unordered with pre-increment (idx passed to vec_swap which caches).
void test_vec_remove_unordered_no_double_eval() {
    printf("Testing vec_remove_unordered does not double-evaluate idx...\n");
    ints_t nums = { 0 };
    vec_push(&nums, 10);
    vec_push(&nums, 20);
    vec_push(&nums, 30);

    int i = 0;
    // Should remove index 0 (swap with last, then decrement count).
    vec_remove_unordered(&nums, i++);
    assert(i == 1);
    assert(vec_len(&nums) == 2);

    vec_clear(&nums);
}

// Verify growth: push enough items to trigger multiple reallocations.
void test_vec_growth() {
    printf("Testing vector growth over many pushes...\n");
    ints_t nums = { 0 };
    int n = 100;
    for (int i = 0; i < n; i++) {
        vec_push(&nums, i);
    }
    assert(vec_len(&nums) == (size_t)n);
    assert(vec_cap(&nums) >= (size_t)n);

    for (int i = 0; i < n; i++) {
        assert(vec_get(&nums, i) == i);
    }

    vec_clear(&nums);
}

// Verify that vec_data returns a pointer we can use directly.
void test_vec_data_direct_access() {
    printf("Testing direct data pointer access...\n");
    ints_t nums = { 0 };
    vec_push(&nums, 11);
    vec_push(&nums, 22);
    vec_push(&nums, 33);

    int* raw = vec_data(&nums);
    assert(raw[0] == 11);
    assert(raw[1] == 22);
    assert(raw[2] == 33);

    // Mutate through raw pointer, read through vec_get.
    raw[1] = 99;
    assert(vec_get(&nums, 1) == 99);

    vec_clear(&nums);
}

void test_vec_nested() {
    printf("Testing nested vectors...\n");
    VEC(ints_t) matrix = { 0 };

    ints_t row1 = { 0 };
    vec_push(&row1, 1);
    vec_push(&row1, 2);

    ints_t row2 = { 0 };
    vec_push(&row2, 3);
    vec_push(&row2, 4);
    vec_push(&row2, 5);

    vec_push(&matrix, row1);
    vec_push(&matrix, row2);

    assert(vec_len(&matrix) == 2);
    
    ints_t* r1 = &vec_get(&matrix, 0);
    ints_t* r2 = &vec_get(&matrix, 1);

    assert(vec_len(r1) == 2);
    assert(vec_len(r2) == 3);

    assert(vec_get(r1, 0) == 1);
    assert(vec_get(r1, 1) == 2);

    assert(vec_get(r2, 0) == 3);
    assert(vec_get(r2, 1) == 4);
    assert(vec_get(r2, 2) == 5);

    vec_clear(r1);
    vec_clear(r2);
    vec_clear(&matrix);
}

void test_vec_reserve_preserves_data() {
    printf("Testing vec_reserve preserves data...\n");
    VEC(int) v = {0};
    vec_push(&v, 10);
    vec_push(&v, 20);
    vec_reserve(&v, 100);
    // This will fail because vec_reserve currently wipes data and count
    assert(vec_len(&v) == 2);
    assert(vec_get(&v, 0) == 10);
    assert(vec_get(&v, 1) == 20);
    vec_clear(&v);
}

typedef struct {
    int id;
    float value;
} my_struct_t;

void test_vec_swap_non_integer() {
    printf("Testing vec_swap with floats...\n");
    VEC(float) vf = {0};
    vec_push(&vf, 1.1f);
    vec_push(&vf, 2.2f);
    
    vec_swap(&vf, 0, 1);
    
    assert(vec_get(&vf, 0) > 2.1f && vec_get(&vf, 0) < 2.3f);
    vec_clear(&vf);
}

void test_vec_remove_unordered_struct() {
    printf("Testing vec_remove_unordered with structs...\n");
    VEC(my_struct_t) vs = {0};
    my_struct_t a = {1, 1.1f};
    my_struct_t b = {2, 2.2f};
    vec_push(&vs, a);
    vec_push(&vs, b);
    
    vec_remove_unordered(&vs, 0);
    
    assert(vec_get(&vs, 0).id == 2);
    vec_clear(&vs);
}


void test_vec_reserve_smaller_capacity() {
    printf("Testing vec_reserve with smaller capacity...\n");
    VEC(int) v = {0};
    vec_reserve(&v, 10);
    vec_push(&v, 1);
    vec_push(&v, 2);
    vec_push(&v, 3);
    
    size_t old_len = v.len;
    size_t old_cap = v.cap;
    int* old_data = v.data;
    
    // Issue reserve with cap < vec->cap
    vec_reserve(&v, 1);
    
    // Assert all fields remain the same
    assert(v.len == old_len);
    assert(v.cap == old_cap);
    assert(v.data == old_data);
    
    vec_clear(&v);
}

int main() {
    test_vec_basic_push_pop();
    test_vec_sorting();
    test_vec_removal();
    test_vec_string_manipulation();
    test_vec_copy();
    test_vec_extra_ops();
    test_vec_capacity_and_sizes();
    test_vec_indexof();
    test_vec_foreach();

    // Corner-case tests
    test_vec_get_no_double_eval();
    test_vec_get_function_call_idx();
    test_vec_set_no_double_eval();
    test_vec_set_function_call_idx();
    test_vec_swap_in_single_stmt_if();
    test_vec_remove_unordered_in_single_stmt_if();
    test_vec_copy_in_single_stmt_if();
    test_vec_push_in_single_stmt_if();
    test_vec_set_in_single_stmt_if();
    test_vec_pop_drain();
    test_vec_remove_function_call_idx();
    test_vec_swap_no_double_eval();
    test_vec_remove_unordered_no_double_eval();
    test_vec_growth();
    test_vec_data_direct_access();
    test_vec_nested();
    test_vec_reserve_preserves_data();
    test_vec_swap_non_integer();
    test_vec_remove_unordered_struct();
    test_vec_reserve_smaller_capacity();

    printf("\nAll vec.h tests passed successfully!\n");
    return 0;
}
