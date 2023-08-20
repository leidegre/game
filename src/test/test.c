#include "test.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#if _DEBUG
#define TEST_WIN32_DEBUG_HEAP 1
#include <crtdbg.h>
#endif
#endif

typedef struct test_context {
  int        is_dry_run_;
  test_case* curr_;
  test_case* last_; // Most recently registered test
  int32_t    chunk_size_;
  int32_t    chunk_iter_;
  int        argc_;
  char**     argv_;
} test_context;

static test_context g_test_context;

#if TEST_WIN32_DEBUG_HEAP
static _CrtMemState g_mem_state;
#endif

void test_end(test_case* test) {
#if TEST_WIN32_DEBUG_HEAP
  // Only run memory leak detection if the test ran to completion normally
  // it might be misleading to dump a bunch of memory leaks that are due
  // abnormal control flow
  if (test->status_ == TEST_STATUS_OK) {
    _CrtMemState diff, mem_state;
    _CrtMemCheckpoint(&mem_state);
    if (_CrtMemDifference(&diff, &g_mem_state, &mem_state)) {
      int   old_report_mode = _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
      void* old_report_file = _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
      fprintf(stderr, "\nMemory leaks detected in %s!\n", test->name_);
      _CrtMemDumpAllObjectsSince(&g_mem_state);
      _CrtSetReportMode(_CRT_WARN, old_report_mode);
      _CrtSetReportFile(_CRT_WARN, old_report_file);
      test->status_ = TEST_STATUS_FAIL;
    }
  }
#endif
}

static void test_at_exit(void) {
  test_case* test = g_test_context.last_;

  if (test) {
    test_end(test);
  }

  puts(""); // new line

  int passed = 0, failed = 0, skipped = 0;

  while (test) {
    switch (test->status_) {
    case TEST_STATUS_NOT_RUN: {
      skipped++;
      break;
    }
    case TEST_STATUS_OK: {
      passed++;
      break;
    }
    case TEST_STATUS_FAIL: {
      failed++;
      break;
    }
    case TEST_BENCHMARK_OK: {
      test_benchmark* bench = (test_benchmark*)test;

      double ms       = (double)(1000 * bench->t_) / (double)test_time_freq();
      double stdev_ms = (double)(1000 * bench->v_) / (double)test_time_freq();

      if (bench->n_) {
        double throughput =
            16.666666666666666666666666666667 * ((1.0 / (1024 * 1024)) * (double)(bench->n_ * bench->z_) / ms);

        printf("%-30s %7.3f (stdev %7.3f) milliseconds [%9.3f MiB/frame]\n", test->name_, ms, stdev_ms, throughput);
      } else {
        printf("%-30s %7.3f (stdev %7.3f) milliseconds\n", test->name_, ms, stdev_ms);
      }

      break;
    }
    default:
      break; // whatever
    }
    test = test->next_;
  }

  printf(
      "%i %s passed, %i %s failed, %i %s skipped\n",
      passed,
      passed == 1 ? "test" : "tests",
      failed,
      failed == 1 ? "test" : "tests",
      skipped,
      skipped == 1 ? "test" : "tests");
}

#if _WIN32
int test_report_hook(int report_type, char* message, int* return_value) {
  // Just report on the "crashing" test as it happens
  test_case* curr = g_test_context.curr_;
  if (curr) {
    fprintf(stderr, "\n%s exited abnormally (crashed!)\n", curr->name_);
  }
  return 1; // calling _CrtDbgReport next
}
#endif

void test_assertion_fail() {
#if _WIN32
  if (IsDebuggerPresent()) {
    abort();
  }
#endif
}

void test_init(int argc, char* argv[]) {
#if _WIN32
  if (!IsDebuggerPresent()) {
    _CrtSetReportHook(&test_report_hook);

    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);

    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);

    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
  }
#endif
  g_test_context.argc_ = argc;
  g_test_context.argv_ = argv;
  atexit(&test_at_exit);
}

void test_begin(test_case* test, test_status status) {
  g_test_context.curr_          = test;
  g_test_context.curr_->status_ = status;
  putchar('.'); // progress indicator
  fflush(stdout);
#if TEST_WIN32_DEBUG_HEAP
  _CrtMemCheckpoint(&g_mem_state);
#endif
}

void test_register(test_case* test) {
  test->next_          = g_test_context.last_;
  g_test_context.last_ = test;
}

int test_eval(test_case* test) {
  if (test->next_) {
    test_end(test->next_);
  }

  // 1: run test; 0: skip test
  int eval = 1;

  for (int i = 1; i < g_test_context.argc_; i++) {
    if (strncmp(g_test_context.argv_[i], "-", 1) == 0) {
      i++;
      continue;
    } else {
      eval = 0;
      break;
    }
  }

  if (eval == 0) {
    for (int i = 1; i < g_test_context.argc_; i++) {
      if (strncmp(g_test_context.argv_[i], "-", 1) == 0) {
        i++;
        continue;
      } else if (strcmp(g_test_context.argv_[i], test->name_) == 0) {
        eval = 1;
        break;
      }
    }
  }

  if (eval == 1) {
    test_begin(test, TEST_STATUS_OK);
  } else {
    test_begin(test, TEST_STATUS_NOT_RUN);
  }

  return eval;
}

void test_benchmark_register(test_benchmark* bench) {
  test_register(&bench->test_case_);
}

void test_benchmark_set_chunk_size(int32_t chunk_size) {
  g_test_context.chunk_size_ = chunk_size;
}

void test_benchmark_set_chunk_iter(int32_t chunk_iter) {
  g_test_context.chunk_iter_ = chunk_iter;
}

#define SAMPLE_COUNT 16

int test_benchmark_eval(test_benchmark* bench) {
  static int32_t s_sample_i;
  static int64_t s_sample_d[SAMPLE_COUNT];

  switch (bench->test_case_.status_) {
  case TEST_BENCHMARK_NOT_RUN: {
    s_sample_i = 0;

    test_begin(&bench->test_case_, TEST_BENCHMARK_RUN_N);

    bench->n_ = g_test_context.chunk_iter_;
    bench->z_ = g_test_context.chunk_size_;

    bench->i_ = 0;
    bench->t_ = test_time_now();
    return 1; // continue
  }

  case TEST_BENCHMARK_RUN_N: {
    if (bench->i_++ < bench->n_) {
      return 1;
    }

    s_sample_d[s_sample_i++ & (SAMPLE_COUNT - 1)] = test_time_now() - bench->t_;

    if (s_sample_i < SAMPLE_COUNT) {
      bench->i_ = 0;
      bench->t_ = test_time_now();
      return 1;
    } else {
      double acc = 0;

      for (int32_t i = 0; i < SAMPLE_COUNT; i++) {
        acc += log((double)s_sample_d[i]);
      }

      acc /= SAMPLE_COUNT;

      int64_t mean = (int64_t)exp(acc); // geometric mean

      int64_t var = 0;

      for (int32_t i = 0; i < SAMPLE_COUNT; i++) {
        var += (s_sample_d[i] - mean) * (s_sample_d[i] - mean);
      }

      var /= SAMPLE_COUNT;

      bench->t_ = mean;
      bench->v_ = (int64_t)sqrt((double)var);

      bench->test_case_.status_ = TEST_BENCHMARK_OK;
      return 0;
    }
  }

  default:
    break; // whatever
  }
  return 0;
}

void test_report_failure() {
  assert(g_test_context.curr_);
  fprintf(stderr, "\n%s failed!\n", g_test_context.curr_->name_);
  g_test_context.curr_->status_ = TEST_STATUS_FAIL; // test failed
  g_test_context.curr_          = NULL;
}

int test_assert_eq_str(const test_str* expected, const test_str* actual, const test_assert_metadata* m) {
  if (!(expected->size_ == actual->size_ && memcmp(expected->data_, actual->data_, expected->size_) == 0)) {
    test_report_failure();
    char temp[64];
    sprintf(
        temp, actual->size_ < 50 ? "<%.*s>" : "<%.*s...", actual->size_ < 50 ? (int)actual->size_ : 50, actual->data_);
    fprintf(
        stderr,
        "%s(%i): %s == %s; <%.*s> != %s\n",
        m->file_,
        m->line_,
        m->expected_,
        m->actual_,
        (int)expected->size_,
        expected->data_,
        temp);
    return 0;
  }
  return 1;
}

#define GEN_ASSERT_EQ(name, type, fmt)                                                                                 \
  int _TEST_CONCAT(test_assert_eq_, name)(type expected, type actual, const test_assert_metadata* m) {                 \
    if (!(expected == actual)) {                                                                                       \
      test_report_failure();                                                                                           \
      const char* s = "%s(%i):\n  expression : %s == %s\n  expected   : " fmt "\n  actual     : " fmt "\n";            \
      fprintf(stderr, s, m->file_, m->line_, m->expected_, m->actual_, expected, actual);                              \
      return 0;                                                                                                        \
    }                                                                                                                  \
    return 1;                                                                                                          \
  }

GEN_ASSERT_EQ(int, int, "%i")
GEN_ASSERT_EQ(int64, int64_t, "%lli")
GEN_ASSERT_EQ(uint8, uint8_t, "%u")
GEN_ASSERT_EQ(uint16, uint16_t, "%u")
GEN_ASSERT_EQ(uint32, uint32_t, "%u")
GEN_ASSERT_EQ(uint64, uint64_t, "%llu")
GEN_ASSERT_EQ(size, size_t, "%zu")
GEN_ASSERT_EQ(ptr, const void*, "%p")

#define GEN_ASSERT_EQ_F(type, fmt)                                                                                     \
  int _TEST_CONCAT(test_assert_eq_, type)(type expected, type actual, type tolerance, const test_assert_metadata* m) { \
    if (!(fabsf(expected - actual) <= tolerance)) {                                                                    \
      test_report_failure();                                                                                           \
      const char* s = "%s(%i): %s == %s; " fmt " != " fmt "\n";                                                        \
      fprintf(stderr, s, m->file_, m->line_, m->expected_, m->actual_, expected, actual);                              \
      return 0;                                                                                                        \
    }                                                                                                                  \
    return 1;                                                                                                          \
  }

GEN_ASSERT_EQ_F(float, "%f")

#if _WIN32

// there's no portable high resolution timer that I know of in the C standard library

int64_t test_time_freq() {
  static int64_t s_freq = 0;
  if (!s_freq) {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    s_freq = freq.QuadPart;
  }
  return s_freq;
}

int64_t test_time_now() {
  LARGE_INTEGER val;
  QueryPerformanceCounter(&val);
  return val.QuadPart;
}

double test_time_diff_to_seconds(int64_t tick) {
  return (double)(test_time_now() - tick) / test_time_freq();
}

#endif