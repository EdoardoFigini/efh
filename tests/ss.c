#include "test.h"

#define SS_IMPLEMENTATION
#include "ss.h"

#include <stdio.h>
#include <string.h>

bool creation() {
    const char* raw = "Hello, World";
    ss_t s = ss_from_cstr(raw);
    TEST_ASSERT(s.len == 12);
    TEST_ASSERT(s.data == raw);
    
    ss_t sub = ss_advance_by(s, 7);
    TEST_ASSERT(sub.len == 5);
    TEST_ASSERT(memcmp(sub.data, "World", 5) == 0);

    return true;
}

bool trimming() {
    ss_t s = ss_from_cstr("   surround   ");
    
    ss_t trimmed = ss_trim(s);
    TEST_ASSERT(trimmed.len == 8);
    TEST_ASSERT(memcmp(trimmed.data, "surround", 8) == 0);

    ss_t start_only = ss_trim_start(s);
    TEST_ASSERT(start_only.len == 11); // "surround   "
    
    ss_t end_only = ss_trim_end(s);
    TEST_ASSERT(end_only.len == 11); // "   surround"

    return true;
}

bool search_and_match() {
    ss_t s = ss_from_cstr("filename.tar.gz");
    
    TEST_ASSERT(ss_starts_with_cstr(s, "file"));
    TEST_ASSERT(ss_ends_with_cstr(s, ".gz"));
    TEST_ASSERT(!ss_eq_cstr(s, "filename.tar"));
    
    ss_t after_dot = ss_advance_after(s, ".");
    TEST_ASSERT(ss_eq_cstr(after_dot, "tar.gz"));
    
    size_t idx = ss_index_of(s, 't');
    TEST_ASSERT(idx == 9);

    return true;
}

bool conversion() {
    ss_t si = ss_from_cstr("12345");
    TEST_ASSERT(ss_atoi(si) == 12345);
    
    ss_t sf = ss_from_cstr(" 3.14159");
    float f = ss_atof(sf);
    TEST_ASSERT(f > 3.14f && f < 3.15f);
    
    ss_t s2 = ss_from_cstr("  67ABCD");
    TEST_ASSERT(ss_atoi(s2) == 67);

    return true;
}

bool tok() {
  ss_t list = ss_from_cstr("apple,banana,cherry");

  for (size_t i=0; i < 3; i++) {
    ss_t s = ss_tok(&list, ',');

    switch(i) {
      case 0: TEST_ASSERT(ss_eq_cstr(s, "apple")); break;
      case 1: TEST_ASSERT(ss_eq_cstr(s, "banana")); break;
      case 2: TEST_ASSERT(ss_eq_cstr(s, "cherry")); break;
      default: TEST_ASSERT(0 && "UNREACHABLE");
    }
  }

  TEST_ASSERT(list.len == 0);

    return true;
}

#define SB_IMPLEMENTATION
#include "sb.h"

bool from_sb() {
    sb_t sb = {0};
    sb_append(&sb, "dynamic string");
    
    ss_t s = ss_from_sb(sb);
    TEST_ASSERT(s.len == 14);
    TEST_ASSERT(ss_eq_cstr(s, "dynamic string"));
    
    sb_free(&sb);

    return true;
}

bool sub_and_eq() {
    ss_t s = ss_from_cstr("hello world");
    
    ss_t sub = ss_sub(s, 0, 5); // "hello"
    TEST_ASSERT(sub.len == 5);
    TEST_ASSERT(ss_eq_cstr(sub, "hello"));

    ss_t sub2 = ss_sub(s, 6, 11); // "world"
    TEST_ASSERT(ss_eq(sub2, ss_from_cstr("world")));

    // ss_starts_with / ss_ends_with
    TEST_ASSERT(ss_starts_with(s, ss_from_cstr("hello")));
    TEST_ASSERT(ss_ends_with(s, ss_from_cstr("world")));
    
    TEST_ASSERT(!ss_starts_with(s, ss_from_cstr("world")));
    TEST_ASSERT(!ss_ends_with(s, ss_from_cstr("hello")));
    
    TEST_ASSERT(!ss_eq(sub, sub2));

    ss_t empty1 = ss_from_cstr("");
    ss_t empty2 = ss_from_cstr("");
    // This will fail due to the ss_eq bug
    TEST_ASSERT(ss_eq(empty1, empty2));

    return true;
}

bool search_more() {
    ss_t s = ss_from_cstr("foo.bar.baz");

    size_t idx = ss_index_of_cstr(s, "bar");
    TEST_ASSERT(idx == 4);

    ss_t until_bar = ss_advance_until(s, "bar");
    TEST_ASSERT(ss_eq_cstr(until_bar, "bar.baz"));

    ss_t not_found = ss_advance_until(s, "qux");
    TEST_ASSERT(not_found.len == 0);
    
    size_t no_idx = ss_index_of_cstr(s, "qux");
    TEST_ASSERT(no_idx == s.len);

    return true;
}

bool in_place_ops() {
    
    ss_t s = ss_from_cstr("  whitespace  ");
    ss_trim_start_in_place(&s);
    TEST_ASSERT(ss_eq_cstr(s, "whitespace  "));
    
    ss_trim_end_in_place(&s);
    TEST_ASSERT(ss_eq_cstr(s, "whitespace"));
    
    ss_t s2 = ss_from_cstr("  both  ");
    ss_trim_in_place(&s2);
    TEST_ASSERT(ss_eq_cstr(s2, "both"));

    ss_t s3 = ss_from_cstr("abcdef");
    ss_advance_by_in_place(&s3, 2);
    TEST_ASSERT(ss_eq_cstr(s3, "cdef"));

    // This will fail due to the inverted return value bug
    bool found = ss_advance_until_in_place(&s3, "e");
    TEST_ASSERT(found == true);
    TEST_ASSERT(ss_eq_cstr(s3, "ef"));

    ss_t s4 = ss_from_cstr("prefix_suffix");
    ss_advance_after_in_place(&s4, "_");
    TEST_ASSERT(ss_eq_cstr(s4, "suffix"));

    ss_advance_after_in_place(&s4, "missing");
    TEST_ASSERT(s4.len == 0);

    return true;
}

bool ss_empty_prefix_bug() {
    ss_t s = ss_from_cstr("hello");
    ss_t empty = ss_from_cstr("");
    
    // ss_ends_with correctly returns true
    TEST_ASSERT(ss_ends_with(s, empty) == true);
    
    // ss_starts_with incorrectly returns false.
    // This TEST_ASSERT will fail, proving the inconsistency bug!
    TEST_ASSERT(ss_starts_with(s, empty) == true);

    return true;
}

bool ss_atof_trim_bug() {
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
    TEST_ASSERT(val > 3.1f && val < 3.145f);

    return true;
}



// --- Predicates for ss_advance_while tests ---

static bool is_digit(char c) { return c >= '0' && c <= '9'; }
static bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

// --- ss_advance_while tests ---

// Matching prefix chars are skipped; the returned slice starts at the first non-match.
bool ss_advance_while_skips_matching_prefix() {
    ss_t s = ss_from_cstr("123abc");

    ss_t result = ss_advance_while(s, is_digit);

    // "123" consumed; "abc" (3 chars) remains
    TEST_ASSERT(result.len == 3);
    TEST_ASSERT(memcmp(result.data, "abc", 3) == 0);

    return true;
}

// When the first char does not match, the returned slice equals the original.
bool ss_advance_while_no_match_returns_original() {
    ss_t s = ss_from_cstr("abc123");

    ss_t result = ss_advance_while(s, is_digit);

    TEST_ASSERT(result.len  == s.len);
    TEST_ASSERT(result.data == s.data);

    return true;
}

// When every char matches, the returned slice is empty.
bool ss_advance_while_all_match_returns_empty() {
    ss_t s = ss_from_cstr("12345");

    ss_t result = ss_advance_while(s, is_digit);

    TEST_ASSERT(result.len == 0);

    return true;
}

// ss_advance_while is value-returning; the original slice must be unmodified.
bool ss_advance_while_does_not_modify_original() {
    ss_t s = ss_from_cstr("999rest");
    const char* original_data = s.data;
    size_t      original_len  = s.len;

    ss_advance_while(s, is_digit);

    TEST_ASSERT(s.data == original_data);
    TEST_ASSERT(s.len  == original_len);

    return true;
}

// --- ss_advance_while_in_place tests ---

// The slice is mutated: it starts at the first char that does not match.
bool ss_advance_while_in_place_mutates_slice() {
    ss_t s = ss_from_cstr("42abc");

    ss_advance_while_in_place(&s, is_digit);

    // "42" consumed; "abc" (3 chars) remains
    TEST_ASSERT(s.len == 3);
    TEST_ASSERT(memcmp(s.data, "abc", 3) == 0);

    return true;
}

// Returns true when the slice still has characters after advancement.
bool ss_advance_while_in_place_returns_true_when_nonempty_remains() {
    ss_t s = ss_from_cstr("42abc");

    bool result = ss_advance_while_in_place(&s, is_digit);

    // "42" consumed, "abc" remains — non-empty → true
    TEST_ASSERT(result == true);

    return true;
}

// Returns false when every character matched and the slice is now empty.
bool ss_advance_while_in_place_returns_false_when_fully_consumed() {
    ss_t s = ss_from_cstr("777");

    bool result = ss_advance_while_in_place(&s, is_digit);

    TEST_ASSERT(result == false);
    TEST_ASSERT(s.len  == 0);

    return true;
}

// When the first char does not match, the slice is unchanged and true is returned.
bool ss_advance_while_in_place_no_match_leaves_slice_unchanged() {
    ss_t s = ss_from_cstr("abc");
    const char* original_data = s.data;

    bool result = ss_advance_while_in_place(&s, is_digit);

    // slice still has content → true; nothing was consumed
    TEST_ASSERT(result == true);
    TEST_ASSERT(s.data == original_data);
    TEST_ASSERT(s.len  == 3);

    return true;
}

// --- Corner-case tests ---

// ss_advance_while: empty slice.
// The loop evaluates f(*data) before checking len, so *data (the null terminator
// of ss_from_cstr("")) is read.  As long as f('\0') is false the loop exits
// immediately; len stays 0.
bool ss_advance_while_empty_slice_returns_empty() {
    ss_t s = ss_from_cstr("");

    ss_t result = ss_advance_while(s, is_digit); // f('\0') == false → exits immediately

    TEST_ASSERT(result.len == 0);

    return true;
}

// ss_advance_while_in_place: empty slice.
// The len-first loop never calls f, so there is no read past the slice.
// Returns false because len==0.
bool ss_advance_while_in_place_empty_slice_returns_false() {
    ss_t s = ss_from_cstr("");

    bool result = ss_advance_while_in_place(&s, is_digit);

    TEST_ASSERT(result == false);
    TEST_ASSERT(s.len  == 0);

    return true;
}

// ss_advance_while: single matching char is consumed, result is empty.
bool ss_advance_while_single_matching_char_returns_empty() {
    ss_t s = ss_from_cstr("7");

    ss_t result = ss_advance_while(s, is_digit);

    TEST_ASSERT(result.len == 0);

    return true;
}

// ss_advance_while_in_place: single matching char is consumed, slice becomes
// empty and false is returned.
bool ss_advance_while_in_place_single_matching_char_empties_and_returns_false() {
    ss_t s = ss_from_cstr("9");

    bool result = ss_advance_while_in_place(&s, is_digit);

    TEST_ASSERT(result == false);
    TEST_ASSERT(s.len  == 0);

    return true;
}

// ss_advance_while: sub-view respects len, does not read into the outer buffer.
// Slice "123" is carved from "123xyz" with len=3.  A greedy is_digit predicate
// must stop at the slice boundary, not at 'x' in the surrounding buffer.
bool ss_advance_while_stops_at_len_boundary() {
    // Arrange: a slice covering only the first 3 bytes of "123xyz"
    const char* buf = "123xyz";
    ss_t sub = { .data = buf, .len = 3 };

    ss_t result = ss_advance_while(sub, is_digit);

    // All 3 digits consumed; result must be empty, not point into "xyz"
    TEST_ASSERT(result.len == 0);

    return true;
}

// ss_advance_while_in_place: sub-view respects len, does not read into the outer buffer.
bool ss_advance_while_in_place_stops_at_len_boundary() {
    const char* buf = "123xyz";
    ss_t sub = { .data = buf, .len = 3 };

    bool result = ss_advance_while_in_place(&sub, is_digit);

    // All 3 digits consumed → empty → false
    TEST_ASSERT(result   == false);
    TEST_ASSERT(sub.len  == 0);

    return true;
}

int main() {
  TEST(creation);
  TEST(trimming);
  TEST(search_and_match);
  TEST(conversion);
  TEST(tok);
  TEST(from_sb);
  TEST(sub_and_eq);
  TEST(search_more);
  TEST(in_place_ops);
  TEST(ss_empty_prefix_bug);
  TEST(ss_atof_trim_bug);

  // ss_advance_while / ss_advance_while_in_place — normal cases
  TEST(ss_advance_while_skips_matching_prefix);
  TEST(ss_advance_while_no_match_returns_original);
  TEST(ss_advance_while_all_match_returns_empty);
  TEST(ss_advance_while_does_not_modify_original);
  TEST(ss_advance_while_in_place_mutates_slice);
  TEST(ss_advance_while_in_place_returns_true_when_nonempty_remains);
  TEST(ss_advance_while_in_place_returns_false_when_fully_consumed);
  TEST(ss_advance_while_in_place_no_match_leaves_slice_unchanged);

  // ss_advance_while / ss_advance_while_in_place — corner cases
  TEST(ss_advance_while_empty_slice_returns_empty);
  TEST(ss_advance_while_in_place_empty_slice_returns_false);
  TEST(ss_advance_while_single_matching_char_returns_empty);
  TEST(ss_advance_while_in_place_single_matching_char_empties_and_returns_false);
  TEST(ss_advance_while_stops_at_len_boundary);
  TEST(ss_advance_while_in_place_stops_at_len_boundary);

  REPORT("ss.h");
  return 0;
}
