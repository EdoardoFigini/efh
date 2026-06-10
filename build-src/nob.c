#define NOB_IMPLEMENTATION
#define NOB_WARN_DEPRECATED
#include "nob.h"

#define FLAG_IMPLEMENTATION
#include "flag.h"

#ifdef _WIN32
#define OS_SEP  "\\"
#define EXE_EXT ".exe"
#define NUL_SINK "NUL"
#else
#define OS_SEP "/"
#define EXE_EXT ""
#define NUL_SINK "/dev/null"
#endif

#define LOCAL_FOLDER(x) "." OS_SEP #x OS_SEP

typedef struct {
  const char* const* items;
  size_t count;
} const_str_list_t;

#define CSTR_LIST(...) { \
  .items = (const char*[]){ __VA_ARGS__ }, \
  .count = (sizeof((const char*[]){ __VA_ARGS__ }) / sizeof(const char*)) \
}
#define CSTR_LIST_EMPTY { .items = NULL, .count = 0 }

NOBDEF Nob_Log_Handler nob_color_log_handler;

//////////////////////////////////////////////////

// DEFINE BUILD PARAMETERS HERE 

// Folders
#define TESTS_FOLDER LOCAL_FOLDER(tests)
#define BUILD_FOLDER LOCAL_FOLDER(build)
#define BIN_FOLDER   LOCAL_FOLDER(bin)

#define COMPILE_COMMANDS BUILD_FOLDER "compile_commands.json"

#define PROJ_NAME "main" 

// Supported Toolchains
typedef enum {
  T_MSVC,
  T_GNU,
  T_LLVM,

  COUNT_TOOLCHAINS
} tc_t;

const char* tc_names[] = {
  [T_MSVC] = "MSVC",
  [T_GNU]  = "GNU",
  [T_LLVM] = "LLVM",
};

const char* tc_cc[] = {
  [T_MSVC] = "cl",
  [T_GNU]  = "gcc",
  [T_LLVM] = "clang",
};

const char* tc_dbg[] = {
  [T_MSVC] = "x64dbg",
  [T_GNU]  = "gdb",
  [T_LLVM] = "gdb",
};

// Default toolchain
// Edit this based on platform
#ifdef _WIN32
#define DEFAULT_TOOLCHAIN T_MSVC
#else
#define DEFAULT_TOOLCHAIN T_GNU
#endif

// Flags per toolchain
// Edit these to change compilation settings and parameters
const_str_list_t tc_cflags_debug[] = {
  // cl implements std=C89 by default
  [T_MSVC] = CSTR_LIST("/nologo", "/W2", "/diagnostics:caret", "/D_CRT_SECURE_NO_WARNINGS", "/Od", "/TC"),
  [T_GNU]  = CSTR_LIST("-Wall", "-Wextra", "-Wswitch-enum", "-ggdb", "-O0", "-x", "c", "-std=c99"),
  [T_LLVM] = CSTR_LIST("-Wall", "-Wextra", "-Wswitch-enum", "-ggdb", "-O0", "-x", "c", "-std=c99"),
};

const_str_list_t tc_cflags_release[] = {
  [T_MSVC] = CSTR_LIST("/nologo", "/D_CRT_SECURE_NO_WARNINGS", "/O2", "/TC"),
  [T_GNU]  = CSTR_LIST("-O3", "-x", "c", "-std=c99"),
  [T_LLVM] = CSTR_LIST("-O3", "-x", "c", "-std=c99"),
};

const_str_list_t tc_lflags_debug[] = {
#ifdef _WIN32
  [T_MSVC] = CSTR_LIST("/nologo", "/DEBUG"),
  [T_GNU]  = CSTR_LIST("-ggdb"),
  [T_LLVM] = CSTR_LIST("-ggdb"),
#else
  [T_GNU]  = CSTR_LIST("-ggdb"),
  [T_LLVM] = CSTR_LIST("-ggdb"),
#endif
};

const_str_list_t tc_lflags_release[] = {
#ifdef _WIN32
  [T_MSVC] = CSTR_LIST("/nologo"),
  [T_GNU]  = CSTR_LIST("-s"),
  [T_LLVM] = CSTR_LIST("-s"),
#else
  [T_GNU]  = CSTR_LIST("-s"),
  [T_LLVM] = CSTR_LIST("-s"),
#endif
};

const_str_list_t tc_includes[] = {
#if defined(_WIN32)
  [T_MSVC] = CSTR_LIST("."),
  [T_GNU]  = CSTR_LIST("."),
  [T_LLVM] = CSTR_LIST("."),
#elif defined(__linux__)
  [T_GNU]  = CSTR_LIST("."),
  [T_LLVM] = CSTR_LIST("."),
#endif
};

const_str_list_t tc_lib_path[] = {
#if defined(_WIN32)
  [T_MSVC] = CSTR_LIST_EMPTY,
  [T_GNU]  = CSTR_LIST_EMPTY,
  [T_LLVM] = CSTR_LIST_EMPTY,
#elif defined(__linux__)
  [T_GNU]  = CSTR_LIST_EMPTY,
  [T_LLVM] = CSTR_LIST_EMPTY,
#endif
};

const_str_list_t tc_libs[] = {
#if defined(_WIN32)
  [T_MSVC] = CSTR_LIST_EMPTY,
  [T_GNU]  = CSTR_LIST_EMPTY,
  [T_LLVM] = CSTR_LIST_EMPTY,
#elif defined(__linux__)
  [T_GNU]  = CSTR_LIST_EMPTY,
  [T_LLVM] = CSTR_LIST_EMPTY,
#endif
};

// Toolchain specific flags/parameters
// Edit these if adding a new toolchain, otherwise these SHOULD NOT BE CUSTOMIZED
const char* tc_out_obj[] = {
  [T_MSVC] = "/Fo:",
  [T_GNU]  = "-o",
  [T_LLVM] = "-o",
};

const char* tc_out_exe[] = {
  [T_MSVC] = "/Fe:",
  [T_GNU]  = "-o",
  [T_LLVM] = "-o",
};

const char* tc_include_flag[] = {
  [T_MSVC] = "/I",
  [T_GNU]  = "-I",
  [T_LLVM] = "-I",
};

const char* tc_compile_flag[] = {
  [T_MSVC] = "/c",
  [T_GNU]  = "-c",
  [T_LLVM] = "-c",
};

const char* tc_lib_flag[] = {
  [T_MSVC] = "",
  [T_GNU]  = "-l",
  [T_LLVM] = "-l",
};

const char* tc_lib_path_flag[] = {
  [T_MSVC] = "/LIBPATH:",
  [T_GNU]  = "-L",
  [T_LLVM] = "-L",
};

const char* tc_link_options_flag[] = {
  [T_MSVC] = "/link",
  [T_GNU]  = "",
  [T_LLVM] = "",
};

const char* tc_dbg_args_flag[] = {
  [T_MSVC] = "",
  [T_GNU]  = "--args",
  [T_LLVM] = "--args",
};

//////////////////////////////////////////////////

typedef struct {
  const char* src_path;
  const char* exe_path;
  bool compiled;
  bool passed;
} test_t;

typedef struct {
  Procs procs;
  struct {
    test_t* items;
    size_t count;
    size_t capacity;
  } tests;
} walk_data_t;

typedef enum {
  BT_DEBUG,
  BT_RELEASE,

  COUNT_BUILD_TYPES
} build_type_t;

static const char* bt_names[] = {
  [BT_DEBUG]   = "DEBUG",
  [BT_RELEASE] = "RELEASE",
};

static tc_t tc  = DEFAULT_TOOLCHAIN;
static build_type_t bt = BT_DEBUG;

const char* supported_tcs() {
  Nob_String_Builder sb = { 0 };

  nob_sb_appendf(&sb, "%s", tc_names[0]);
  for (size_t i = 1; i < COUNT_TOOLCHAINS; i++) {
    nob_sb_appendf(&sb, ", %s", tc_names[i]);
  }

  return nob_temp_sprintf("%.*s", (int)sb.count, sb.items);
}

const char* supported_bts() {
  Nob_String_Builder sb = { 0 };

  nob_sb_appendf(&sb, "%s", bt_names[0]);
  for (size_t i = 1; i < COUNT_BUILD_TYPES; i++) {
    nob_sb_appendf(&sb, ", %s", bt_names[i]);
  }

  return nob_temp_sprintf("%.*s", (int)sb.count, sb.items);
}

void usage(FILE *stream, const char* exe)
{
  fprintf(stream, "Usage: %s [OPTIONS]\n", exe);
  fprintf(stream, "OPTIONS:\n");
  flag_print_options(stream);
}

Nob_String_View filename_from_path(Nob_String_View path) {
  while (path.count > 0 && path.data[--path.count] != '.');

  Nob_String_View tmp = (Nob_String_View){
    .data = &path.data[path.count],
    .count = 0,
  };
  while(*(--tmp.data) != *OS_SEP && ++tmp.count <= path.count);
  tmp.data++;

  return tmp;
}

void compile_test(test_t* test) {
  Cmd cmd = {0};

  Nob_String_View path_sv = nob_sv_from_cstr(test->src_path);

  test->exe_path = nob_temp_sprintf(BUILD_FOLDER "%.*s-%s" EXE_EXT, SV_Arg(filename_from_path(path_sv)), tc_names[tc]);

  nob_cmd_append(&cmd, tc_cc[tc]);
  const_str_list_t tc_cflags;
  if (bt == BT_DEBUG)
    tc_cflags = tc_cflags_debug[tc];
  else
    tc_cflags = tc_cflags_release[tc];
  for (size_t i=0; i < tc_cflags.count; i++) {
    nob_cmd_append(&cmd, tc_cflags.items[i]);
  }
  for (size_t i=0; i < tc_includes[tc].count; i++) {
    nob_cmd_append(&cmd, nob_temp_sprintf("%s%s", tc_include_flag[tc], tc_includes[tc].items[i]));
  }
  nob_cmd_append(&cmd, test->src_path);
  nob_cmd_append(&cmd, tc_out_exe[tc], test->exe_path);

  test->compiled = cmd_run(&cmd);
}

void run_test(test_t* test) {
  Cmd cmd = { 0 };
  Nob_String_View path_sv = nob_sv_from_cstr(test->exe_path);

  nob_cmd_append(&cmd, test->exe_path); 
  test->passed = nob_cmd_run(&cmd);
}

bool compile_and_run_test(Nob_Walk_Entry entry) {
  Cmd cmd = {0};
  walk_data_t *wd = (walk_data_t*)entry.data;

  Nob_String_View path = nob_sv_from_cstr(entry.path);

  if (!nob_sv_ends_with_cstr(path, ".c")) return true;

  test_t test = { 0 };
  test.src_path = nob_temp_strdup(entry.path);

  compile_test(&test);
  if(test.compiled)
    run_test(&test);

  nob_da_append(&wd->tests, test);

  return true;
}

#ifdef _WIN32
bool run_in_msvc_env(int argc, char** argv) {
  if(!GetEnvironmentVariable("VSCMD_VER", NULL, 0) && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
    nob_log(NOB_INFO, "Loading MSVC environment");
    Cmd cmd = { 0 };

    nob_minimal_log_level = NOB_WARNING;
    nob_cmd_append(
      &cmd, 
      "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\Tools\\VsDevCmd.bat", 
      "-arch=x64", 
      "-host_arch=x64",
      ">", "NUL"
    );
    nob_cmd_append(&cmd, "&&");
    nob_da_append_many(&cmd, argv, argc);
    exit(cmd_run(&cmd));
  }
  return true;
}
#endif

void log_test_outcome(const test_t* test);

int main(int argc, char **argv)
{
#if defined(_MSC_VER) && defined(_WIN32)
    const char *binary_path = *argv;

    nob_set_log_handler(nob_color_log_handler);

    if (!nob_sv_end_with(nob_sv_from_cstr(binary_path), ".exe")) {
      binary_path = nob_temp_sprintf("%s.exe", binary_path);
    }

    if(nob_needs_rebuild1(binary_path, __FILE__))
      if(!run_in_msvc_env(argc, argv)) return 1;
#endif
    NOB_GO_REBUILD_URSELF(argc, argv);

    bool *f_help       = flag_bool("help", false, "Print this message to stdout and exit with 0.");
    char **f_toolchain = flag_str(
      "toolchain", 
      tc_names[DEFAULT_TOOLCHAIN], 
      nob_temp_sprintf("Select the toolchain. Available toolchains: [%s]", supported_tcs())
    );
    char **f_type = flag_str(
      "type", 
      bt_names[BT_DEBUG], 
      nob_temp_sprintf("Select the build type. Available build types: [%s]", supported_bts())
    );
    bool *f_test = flag_bool(
      "test",
      false,
      "Build and run tests."
    );

    const char* exe = argv[0];

    if (!flag_parse(argc, argv)) {
        usage(stderr, exe);
        flag_print_error(stderr);
        return 1; 
    }

    if (*f_help) {
        usage(stdout, exe);
        return 0;
    }

    for (; tc <= COUNT_TOOLCHAINS; tc++) {
      if (tc == COUNT_TOOLCHAINS) {
        nob_log(NOB_ERROR, "Invalid toolchain `%s`, Defaulting to %s", *f_toolchain, tc_names[DEFAULT_TOOLCHAIN]);
        tc = DEFAULT_TOOLCHAIN;
        break;
      }
      if(strcmp(*f_toolchain, tc_names[tc]) == 0) break;
    }

    for (; bt <= COUNT_BUILD_TYPES; bt++) {
      if (bt == COUNT_BUILD_TYPES) {
        nob_log(NOB_ERROR, "Invalid build type `%s`, Defaulting to %s", *f_toolchain, bt_names[BT_DEBUG]);
        bt = BT_DEBUG;
        break;
      }
      if(strcmp(*f_type, bt_names[bt]) == 0) break;
    }

    if (!mkdir_if_not_exists(BUILD_FOLDER)) return 1;
    if (!mkdir_if_not_exists(BIN_FOLDER)) return 1;

    if(tc == T_MSVC) {
#ifdef _WIN32
      if(!run_in_msvc_env(argc, argv)) return 1;
#else
      // TODO: wine?
      nob_log(NOB_ERROR, "MSVC toolchain not available outside of Windows.");
      return 1;
#endif
    }

    argc = flag_rest_argc();
    argv = flag_rest_argv();

    uint64_t start = nob_nanos_since_unspecified_epoch();

    if (*f_test) {
      size_t failed = 0;

      walk_data_t wd = { 0 };
      if(!nob_walk_dir(TESTS_FOLDER, compile_and_run_test, .data = &wd)) return 1;
      da_foreach(test_t, t, &wd.tests) {
        if(!t->passed) failed++;
      }

      fprintf(stderr, "\n\n");
      nob_log(NOB_INFO, "Passed %zu of %zu tests:", wd.tests.count - failed, wd.tests.count);
      da_foreach(test_t, t, &wd.tests) {
        log_test_outcome(t);
      }
    }

    return 0;
}

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

void log_test_outcome(const test_t* test) {
  Nob_String_Builder sb = { 0 };

  if (!test->compiled) {
    nob_sb_append_cstr(&sb, ANSI_PRE ANSI_YELLOW_T   ";" ANSI_BOLD ANSI_POST "  [ NOT COMP ]" ANSI_PRE ANSI_NONE ANSI_POST);
  } else if (!test->passed) {
    nob_sb_append_cstr(&sb, ANSI_PRE ANSI_RED_T      ";" ANSI_BOLD ANSI_POST "  [  FAILED  ]" ANSI_PRE ANSI_NONE ANSI_POST);
  } else {
    nob_sb_append_cstr(&sb, ANSI_PRE ANSI_GREEN_T    ";" ANSI_BOLD ANSI_POST "  [  PASSED  ]" ANSI_PRE ANSI_NONE ANSI_POST);
  }

  Nob_String_View path_sv = nob_sv_from_cstr(test->src_path);

  nob_sb_append_cstr(&sb, " ");
  nob_sb_appendf(&sb, SV_Fmt, SV_Arg(filename_from_path(path_sv)));

  // nob_sb_append_cstr(&sb, "  -  ");
  // nob_sb_appendf(&sb, "stdout: " BUILD_FOLDER"%.*s.stdout.txt", SV_Arg(filename_from_path(path_sv)));

  nob_sb_append_cstr(&sb, "\n");

  Nob_String_View sv = nob_sb_to_sv(sb);
  fprintf(stdout, SV_Fmt, SV_Arg(sv));
}

#define LOG_COLOR_RESET "\033[0m"
#define LOG_COLOR_ERROR "\033[31m"
#define LOG_COLOR_WARN  "\033[33m"
#define LOG_COLOR_INFO  "\033[36m"

NOBDEF void nob_color_log_handler(Nob_Log_Level level, const char *fmt, va_list args)
{
  if (level < nob_minimal_log_level) return;

  Nob_String_Builder sb = { 0 };
  switch (level) {
    case NOB_INFO:
    nob_sb_append_cstr(&sb, LOG_COLOR_INFO "[INFO] ");
      break;
    case NOB_WARNING:
      nob_sb_append_cstr(&sb, LOG_COLOR_WARN "[WARN] ");
      break;
    case NOB_ERROR:
      nob_sb_append_cstr(&sb, LOG_COLOR_ERROR "[ERROR] ");
      break;
    case NOB_NO_LOGS: return;
    default:
        NOB_UNREACHABLE("Nob_Log_Level");
  }

  va_list copy;
  va_copy(copy, args);
  int n = vsnprintf(NULL, 0, fmt, copy);
  va_end(copy);

  nob_da_reserve(&sb, sb.count + n + 1);
  char *dest = sb.items + sb.count;
  vsnprintf(dest, n+1, fmt, args);
  sb.count += n;

  nob_sb_append_cstr(&sb, LOG_COLOR_RESET);
  nob_sb_append(&sb, '\n');

  fprintf(stderr, SV_Fmt, SV_Arg(sb_to_sv(sb)));
}
