#define SS_IMPLEMENTATION
#include "ss.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_creation() {
    printf("Testing creation...\n");
    const char* raw = "Hello, World";
    ss_t s = ss_from_cstr(raw);
    assert(s.len == 12);
    assert(s.data == raw);
    
    ss_t sub = ss_advance_by(s, 7);
    assert(sub.len == 5);
    assert(memcmp(sub.data, "World", 5) == 0);
}

void test_trimming() {
    printf("Testing trimming...\n");
    ss_t s = ss_from_cstr("   surround   ");
    
    ss_t trimmed = ss_trim(s);
    assert(trimmed.len == 8);
    assert(memcmp(trimmed.data, "surround", 8) == 0);

    ss_t start_only = ss_trim_start(s);
    assert(start_only.len == 11); // "surround   "
    
    ss_t end_only = ss_trim_end(s);
    assert(end_only.len == 11); // "   surround"
}

void test_search_and_match() {
    printf("Testing search and match...\n");
    ss_t s = ss_from_cstr("filename.tar.gz");
    
    assert(ss_starts_with_cstr(s, "file"));
    assert(ss_ends_with_cstr(s, ".gz"));
    assert(!ss_eq_cstr(s, "filename.tar"));
    
    ss_t after_dot = ss_advance_after(s, ".");
    assert(ss_eq_cstr(after_dot, "tar.gz"));
    
    size_t idx = ss_index_of(s, 't');
    assert(idx == 9);
}

void test_conversion() {
    printf("Testing atoi/atof...\n");
    ss_t si = ss_from_cstr("12345");
    assert(ss_atoi(si) == 12345);
    
    ss_t sf = ss_from_cstr(" 3.14159");
    float f = ss_atof(sf);
    assert(f > 3.14f && f < 3.15f);
    
    ss_t s2 = ss_from_cstr("  67ABCD");
    assert(ss_atoi(s2) == 67);
}

void test_tok() {
  printf("Testing tokenization...\n");
  ss_t list = ss_from_cstr("apple,banana,cherry");

  for (size_t i=0; i < 3; i++) {
    ss_t s = ss_tok(&list, ',');

    switch(i) {
      case 0: assert(ss_eq_cstr(s, "apple")); break;
      case 1: assert(ss_eq_cstr(s, "banana")); break;
      case 2: assert(ss_eq_cstr(s, "cherry")); break;
      default: assert(0 && "UNREACHABLE");
    }
  }

  assert(list.len == 0);
}

#define SB_IMPLEMENTATION
#include "sb.h"

void test_from_sb() {
    printf("Testing ss_from_sb...\n");
    sb_t sb = {0};
    sb_append(&sb, "dynamic string");
    
    ss_t s = ss_from_sb(sb);
    assert(s.len == 14);
    assert(ss_eq_cstr(s, "dynamic string"));
    
    sb_free(&sb);
}

void test_sub_and_eq() {
    printf("Testing ss_sub and ss_eq...\n");
    ss_t s = ss_from_cstr("hello world");
    
    ss_t sub = ss_sub(s, 0, 5); // "hello"
    assert(sub.len == 5);
    assert(ss_eq_cstr(sub, "hello"));

    ss_t sub2 = ss_sub(s, 6, 11); // "world"
    assert(ss_eq(sub2, ss_from_cstr("world")));

    // ss_starts_with / ss_ends_with
    assert(ss_starts_with(s, ss_from_cstr("hello")));
    assert(ss_ends_with(s, ss_from_cstr("world")));
    
    assert(!ss_starts_with(s, ss_from_cstr("world")));
    assert(!ss_ends_with(s, ss_from_cstr("hello")));
    
    assert(!ss_eq(sub, sub2));

    ss_t empty1 = ss_from_cstr("");
    ss_t empty2 = ss_from_cstr("");
    // This will fail due to the ss_eq bug
    assert(ss_eq(empty1, empty2));
}

void test_search_more() {
    printf("Testing advanced search...\n");
    ss_t s = ss_from_cstr("foo.bar.baz");

    size_t idx = ss_index_of_cstr(s, "bar");
    assert(idx == 4);

    ss_t until_bar = ss_advance_until(s, "bar");
    assert(ss_eq_cstr(until_bar, "bar.baz"));

    ss_t not_found = ss_advance_until(s, "qux");
    assert(not_found.len == 0);
    
    size_t no_idx = ss_index_of_cstr(s, "qux");
    assert(no_idx == s.len);
}

void test_in_place_ops() {
    printf("Testing in-place operations...\n");
    
    ss_t s = ss_from_cstr("  whitespace  ");
    ss_trim_start_in_place(&s);
    assert(ss_eq_cstr(s, "whitespace  "));
    
    ss_trim_end_in_place(&s);
    assert(ss_eq_cstr(s, "whitespace"));
    
    ss_t s2 = ss_from_cstr("  both  ");
    ss_trim_in_place(&s2);
    assert(ss_eq_cstr(s2, "both"));

    ss_t s3 = ss_from_cstr("abcdef");
    ss_advance_by_in_place(&s3, 2);
    assert(ss_eq_cstr(s3, "cdef"));

    // This will fail due to the inverted return value bug
    bool found = ss_advance_until_in_place(&s3, "e");
    assert(found == true);
    assert(ss_eq_cstr(s3, "ef"));

    ss_t s4 = ss_from_cstr("prefix_suffix");
    ss_advance_after_in_place(&s4, "_");
    assert(ss_eq_cstr(s4, "suffix"));

    ss_advance_after_in_place(&s4, "missing");
    assert(s4.len == 0);
}

void test_ss_empty_prefix_bug() {
    printf("Testing ss empty prefix inconsistency...\n");
    ss_t s = ss_from_cstr("hello");
    ss_t empty = ss_from_cstr("");
    
    // ss_ends_with correctly returns true
    assert(ss_ends_with(s, empty) == true);
    
    // ss_starts_with incorrectly returns false.
    // This assert will fail, proving the inconsistency bug!
    assert(ss_starts_with(s, empty) == true);
}

void test_ss_atof_trim_bug() {
    printf("Testing ss_atof trim out-of-bounds read bug...\n");
    // We create a string with spaces, followed by "3.14", followed by garbage "999"
    // Our slice len will be 7 (which only covers "   3.14")
    ss_t s = { .data = "   3.14999", .len = 7 };
    
    // ss_atof will calculate len = 7.
    // It will then trim 3 spaces.
    // It will then memcpy 7 bytes from the trimmed pointer ("3.14999"...)
    // This will cause it to read past the end of the slice and parse the "999" garbage!
    float val = ss_atof(s);
    
    // If the bug exists, val will be 3.14999f
    // If the bug is fixed, val will be 3.14f
    assert(val > 3.1f && val < 3.145f);
}



// --- Predicates for ss_advance_while tests ---

static bool is_digit(char c) { return c >= '0' && c <= '9'; }
static bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

// --- ss_advance_while tests ---

// Matching prefix chars are skipped; the returned slice starts at the first non-match.
void test_ss_advance_while_skips_matching_prefix() {
    printf("Testing ss_advance_while skips matching prefix...\n");
    ss_t s = ss_from_cstr("123abc");

    ss_t result = ss_advance_while(s, is_digit);

    // "123" consumed; "abc" (3 chars) remains
    assert(result.len == 3);
    assert(memcmp(result.data, "abc", 3) == 0);
}

// When the first char does not match, the returned slice equals the original.
void test_ss_advance_while_no_match_returns_original() {
    printf("Testing ss_advance_while with no match returns original...\n");
    ss_t s = ss_from_cstr("abc123");

    ss_t result = ss_advance_while(s, is_digit);

    assert(result.len  == s.len);
    assert(result.data == s.data);
}

// When every char matches, the returned slice is empty.
void test_ss_advance_while_all_match_returns_empty() {
    printf("Testing ss_advance_while with all chars matching returns empty...\n");
    ss_t s = ss_from_cstr("12345");

    ss_t result = ss_advance_while(s, is_digit);

    assert(result.len == 0);
}

// ss_advance_while is value-returning; the original slice must be unmodified.
void test_ss_advance_while_does_not_modify_original() {
    printf("Testing ss_advance_while does not modify original slice...\n");
    ss_t s = ss_from_cstr("999rest");
    const char* original_data = s.data;
    size_t      original_len  = s.len;

    ss_advance_while(s, is_digit);

    assert(s.data == original_data);
    assert(s.len  == original_len);
}

// --- ss_advance_while_in_place tests ---

// The slice is mutated: it starts at the first char that does not match.
void test_ss_advance_while_in_place_mutates_slice() {
    printf("Testing ss_advance_while_in_place mutates slice past matching prefix...\n");
    ss_t s = ss_from_cstr("42abc");

    ss_advance_while_in_place(&s, is_digit);

    // "42" consumed; "abc" (3 chars) remains
    assert(s.len == 3);
    assert(memcmp(s.data, "abc", 3) == 0);
}

// Returns true when the slice still has characters after advancement.
void test_ss_advance_while_in_place_returns_true_when_nonempty_remains() {
    printf("Testing ss_advance_while_in_place returns true when remainder non-empty...\n");
    ss_t s = ss_from_cstr("42abc");

    bool result = ss_advance_while_in_place(&s, is_digit);

    // "42" consumed, "abc" remains — non-empty → true
    assert(result == true);
}

// Returns false when every character matched and the slice is now empty.
void test_ss_advance_while_in_place_returns_false_when_fully_consumed() {
    printf("Testing ss_advance_while_in_place returns false when fully consumed...\n");
    ss_t s = ss_from_cstr("777");

    bool result = ss_advance_while_in_place(&s, is_digit);

    assert(result == false);
    assert(s.len  == 0);
}

// When the first char does not match, the slice is unchanged and true is returned.
void test_ss_advance_while_in_place_no_match_leaves_slice_unchanged() {
    printf("Testing ss_advance_while_in_place with no match leaves slice unchanged...\n");
    ss_t s = ss_from_cstr("abc");
    const char* original_data = s.data;

    bool result = ss_advance_while_in_place(&s, is_digit);

    // slice still has content → true; nothing was consumed
    assert(result == true);
    assert(s.data == original_data);
    assert(s.len  == 3);
}

// --- Corner-case tests ---

// ss_advance_while: empty slice.
// The loop evaluates f(*data) before checking len, so *data (the null terminator
// of ss_from_cstr("")) is read.  As long as f('\0') is false the loop exits
// immediately; len stays 0.
void test_ss_advance_while_empty_slice_returns_empty() {
    printf("Testing ss_advance_while on empty slice returns empty...\n");
    ss_t s = ss_from_cstr("");

    ss_t result = ss_advance_while(s, is_digit); // f('\0') == false → exits immediately

    assert(result.len == 0);
}

// ss_advance_while_in_place: empty slice.
// The len-first loop never calls f, so there is no read past the slice.
// Returns false because len==0.
void test_ss_advance_while_in_place_empty_slice_returns_false() {
    printf("Testing ss_advance_while_in_place on empty slice returns false...\n");
    ss_t s = ss_from_cstr("");

    bool result = ss_advance_while_in_place(&s, is_digit);

    assert(result == false);
    assert(s.len  == 0);
}

// ss_advance_while: single matching char is consumed, result is empty.
void test_ss_advance_while_single_matching_char_returns_empty() {
    printf("Testing ss_advance_while single matching char returns empty...\n");
    ss_t s = ss_from_cstr("7");

    ss_t result = ss_advance_while(s, is_digit);

    assert(result.len == 0);
}

// ss_advance_while_in_place: single matching char is consumed, slice becomes
// empty and false is returned.
void test_ss_advance_while_in_place_single_matching_char_empties_and_returns_false() {
    printf("Testing ss_advance_while_in_place single matching char empties slice...\n");
    ss_t s = ss_from_cstr("9");

    bool result = ss_advance_while_in_place(&s, is_digit);

    assert(result == false);
    assert(s.len  == 0);
}

// ss_advance_while: sub-view respects len, does not read into the outer buffer.
// Slice "123" is carved from "123xyz" with len=3.  A greedy is_digit predicate
// must stop at the slice boundary, not at 'x' in the surrounding buffer.
void test_ss_advance_while_stops_at_len_boundary() {
    printf("Testing ss_advance_while stops at len boundary (sub-view)...\n");
    // Arrange: a slice covering only the first 3 bytes of "123xyz"
    const char* buf = "123xyz";
    ss_t sub = { .data = buf, .len = 3 };

    ss_t result = ss_advance_while(sub, is_digit);

    // All 3 digits consumed; result must be empty, not point into "xyz"
    assert(result.len == 0);
}

// ss_advance_while_in_place: sub-view respects len, does not read into the outer buffer.
void test_ss_advance_while_in_place_stops_at_len_boundary() {
    printf("Testing ss_advance_while_in_place stops at len boundary (sub-view)...\n");
    const char* buf = "123xyz";
    ss_t sub = { .data = buf, .len = 3 };

    bool result = ss_advance_while_in_place(&sub, is_digit);

    // All 3 digits consumed → empty → false
    assert(result   == false);
    assert(sub.len  == 0);
}

int main() {
    test_creation();
    test_trimming();
    test_search_and_match();
    test_conversion();
    test_tok();
    test_from_sb();
    test_sub_and_eq();
    test_search_more();
    test_in_place_ops();
    test_ss_empty_prefix_bug();
    test_ss_atof_trim_bug();

    // ss_advance_while / ss_advance_while_in_place — normal cases
    test_ss_advance_while_skips_matching_prefix();
    test_ss_advance_while_no_match_returns_original();
    test_ss_advance_while_all_match_returns_empty();
    test_ss_advance_while_does_not_modify_original();
    test_ss_advance_while_in_place_mutates_slice();
    test_ss_advance_while_in_place_returns_true_when_nonempty_remains();
    test_ss_advance_while_in_place_returns_false_when_fully_consumed();
    test_ss_advance_while_in_place_no_match_leaves_slice_unchanged();

    // ss_advance_while / ss_advance_while_in_place — corner cases
    test_ss_advance_while_empty_slice_returns_empty();
    test_ss_advance_while_in_place_empty_slice_returns_false();
    test_ss_advance_while_single_matching_char_returns_empty();
    test_ss_advance_while_in_place_single_matching_char_empties_and_returns_false();
    test_ss_advance_while_stops_at_len_boundary();
    test_ss_advance_while_in_place_stops_at_len_boundary();

    printf("\nAll ss.h tests passed successfully!\n");
    return 0;
}
