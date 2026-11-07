
#ifndef TEST_H
#define TEST_H

#include <stdbool.h>


#define ANSI_PRE "\033["
#define ANSI_POST "m"
#define ANSI_RED_T    "31"
#define ANSI_GREEN_T  "32"
#define ANSI_YELLOW_T "33"
#define ANSI_BLUE_T   "34"
#define ANSI_VIOLET_T "35"
#define ANSI_CYAN_T   "36"
#define ANSI_GRAY_T   "37"
#define ANSI_RED_B    "41"
#define ANSI_GREEN_B  "42"
#define ANSI_YELLOW_B "43"
#define ANSI_BLUE_B   "44"
#define ANSI_VIOLET_B "45"
#define ANSI_CYAN_B   "46"
#define ANSI_GRAY_B   "47"
#define ANSI_NONE "0"
#define ANSI_BOLD "1"

#define TEST(fn) do {\
  fprintf(stdout, ANSI_PRE ANSI_GRAY_T ";" ANSI_BOLD ANSI_POST "  [ RUNNING  ]" ANSI_PRE ANSI_NONE ANSI_POST  " %s\r", # fn); \
  if (fn()) { \
    fprintf(stdout, ANSI_PRE ANSI_GREEN_T ";" ANSI_BOLD ANSI_POST "  [  PASSED  ]" ANSI_PRE ANSI_NONE ANSI_POST  " %s\n", # fn); \
    passed++; \
  } else { \
    fprintf(stdout,                                            \
        ANSI_PRE ANSI_RED_T ";" ANSI_BOLD ANSI_POST            \
        "  [  FAILED  ]" ANSI_PRE ANSI_NONE ANSI_POST  " %s\n" \
        "               %s:%d: Assertion `%s` failed\n"        \
        , # fn,                                                \
        last_failed_loc_file, last_failed_loc_line,            \
        last_failed_condition                                  \
    );                                                         \
    failed++; \
  } \
  tot++; \
} while(0)

#define TEST_ASSERT(cnd) if (!(cnd)) { last_failed_loc_file = __FILE__; last_failed_loc_line = __LINE__; last_failed_condition = # cnd; return false; }

#define REPORT(name) \
  printf( \
      "\n\t" name ": " \
      ANSI_PRE ANSI_GREEN_T ";" ANSI_BOLD ANSI_POST "%3d" ANSI_PRE ANSI_NONE ANSI_POST " passed, " \
      ANSI_PRE ANSI_RED_T   ";" ANSI_BOLD ANSI_POST "%3d" ANSI_PRE ANSI_NONE ANSI_POST " failed / " \
      ANSI_PRE                  ANSI_BOLD ANSI_POST "%3d" ANSI_PRE ANSI_NONE ANSI_POST " total\n\n" \
      , passed, failed, tot \
  )

int tot = 0;
int passed = 0;
int failed = 0;

const char* last_failed_loc_file  = "";
int         last_failed_loc_line  = 0;
const char* last_failed_condition = "";

#endif 
