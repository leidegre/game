#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wunused-macros"
#endif

#define _TEST_CONCAT2(a, b) a##b
#define _TEST_CONCAT(a, b) _TEST_CONCAT2(a, b)

void test_assertion_fail();

#define _ASSERT_EQUAL(name, expected, actual)                                                                          \
  do {                                                                                                                 \
    test_assert_metadata _metadata = { __FILE__, __LINE__, #expected, #actual };                                       \
    if (!_TEST_CONCAT(test_assert_eq_, name)(expected, actual, &_metadata)) {                                          \
      test_assertion_fail();                                                                                           \
      return 1;                                                                                                        \
    }                                                                                                                  \
  } while (0)

#define _ASSERT_EQUAL_F(type, expected, actual, tolerance)                                                             \
  do {                                                                                                                 \
    test_assert_metadata _metadata = { __FILE__, __LINE__, #expected, #actual };                                       \
    if (!_TEST_CONCAT(test_assert_eq_, type)(expected, actual, tolerance, &_metadata)) {                               \
      test_assertion_fail();                                                                                           \
      return 1;                                                                                                        \
    }                                                                                                                  \
  } while (0)

#define ASSERT_EQUAL_PTR(expected, actual)                                                                             \
  do {                                                                                                                 \
    test_assert_metadata _metadata = { __FILE__, __LINE__, #expected, #actual };                                       \
    if (!test_assert_eq_ptr(expected, actual, &_metadata)) {                                                           \
      test_assertion_fail();                                                                                           \
      return 1;                                                                                                        \
    }                                                                                                                  \
  } while (0)

#define ASSERT_TRUE(expr) _ASSERT_EQUAL(int, !!(expr), 1)

#define ASSERT_FALSE(expr) _ASSERT_EQUAL(int, !!(expr), 0)

#define ASSERT_EQUAL_BYTE(expected, actual) _ASSERT_EQUAL(uint8, expected, actual)

#define ASSERT_EQUAL_U16(expected, actual) _ASSERT_EQUAL(uint16, expected, actual)

#define ASSERT_EQUAL_U32(expected, actual) _ASSERT_EQUAL(uint32, expected, actual)

#define ASSERT_EQUAL_U64(expected, actual) _ASSERT_EQUAL(uint64, expected, actual)

#define ASSERT_EQUAL_SIZE(expected, actual) _ASSERT_EQUAL(size, expected, actual)

#define ASSERT_EQUAL_I32(expected, actual) _ASSERT_EQUAL(int, expected, actual)

#define ASSERT_EQUAL_I64(expected, actual) _ASSERT_EQUAL(int64, expected, actual)

#define ASSERT_EQUAL_FLOAT(expected, actual, tolerance) _ASSERT_EQUAL_F(float, expected, actual, tolerance)

#define ASSERT_EQUAL_STRLEN(expected, actual, actual_size)                                                             \
  do {                                                                                                                 \
    test_assert_metadata _metadata = { __FILE__, __LINE__, #expected, #actual };                                       \
    test_str             expected2 = { expected, strlen(expected) };                                                   \
    test_str             actual2   = { actual, (size_t)actual_size };                                                  \
    if (!test_assert_eq_str(&expected2, &actual2, &_metadata)) {                                                       \
      test_assertion_fail();                                                                                           \
      return 1;                                                                                                        \
    }                                                                                                                  \
  } while (0)

#define TEST_CASE(label)                                                                                               \
  static test_case _TEST_CONCAT(test_, __LINE__) = { label, NULL, TEST_STATUS_NOT_RUN };                               \
  test_register(&_TEST_CONCAT(test_, __LINE__));                                                                       \
  if (test_eval(&_TEST_CONCAT(test_, __LINE__)))

// clang-format will break this incorrectly
// clang-format off
#define TEST_BENCHMARK(label)                                                                                          \
  static test_benchmark _TEST_CONCAT(test_, __LINE__) = {{label, NULL, TEST_BENCHMARK_NOT_RUN}};                       \
  test_benchmark_register(&_TEST_CONCAT(test_, __LINE__));                                                             \
  while (test_benchmark_eval(&_TEST_CONCAT(test_, __LINE__)))
// clang-format on

#if __clang__
#pragma clang diagnostic pop
#endif

typedef enum test_status {
  TEST_STATUS_NOT_RUN,
  TEST_STATUS_OK,
  TEST_STATUS_FAIL,
  TEST_BENCHMARK_NOT_RUN,
  TEST_BENCHMARK_RUN_N,
  TEST_BENCHMARK_OK
} test_status;

// forward delcaration
typedef struct test_case test_case;

// definition
struct test_case {
  const char* name_;
  test_case*  next_; // The test case that came before this
  test_status status_;
};

void test_init(int argc, char* argv[]);

void test_register(test_case* test);

int test_eval(test_case* test);

void test_report_failure();

typedef struct {
  test_case test_case_;
  int32_t   n_;
  int32_t   z_;
  int32_t   i_;
  int64_t   t_;
  int64_t   v_;
} test_benchmark;

void test_benchmark_register(test_benchmark* bench);

int test_benchmark_eval(test_benchmark* bench);

void test_benchmark_set_chunk_size(int32_t chunk_size);

void test_benchmark_set_chunk_iter(int32_t chunk_count);

typedef struct test_str {
  const char* data_;
  size_t      size_;
} test_str;

typedef struct test_assert_metadata {
  const char* file_;
  int         line_;
  const char* expected_;
  const char* actual_;
} test_assert_metadata;

int test_assert_eq_str(const test_str* expected, const test_str* actual, const test_assert_metadata* m);

int test_assert_eq_uint8(uint8_t expected, uint8_t actual, const test_assert_metadata* m);
int test_assert_eq_uint16(uint16_t expected, uint16_t actual, const test_assert_metadata* m);
int test_assert_eq_uint32(uint32_t expected, uint32_t actual, const test_assert_metadata* m);
int test_assert_eq_uint64(uint64_t expected, uint64_t actual, const test_assert_metadata* m);

int test_assert_eq_size(size_t expected, size_t actual, const test_assert_metadata* m);

int test_assert_eq_int(int expected, int actual, const test_assert_metadata* m);
int test_assert_eq_int64(int64_t expected, int64_t actual, const test_assert_metadata* m);

int test_assert_eq_float(float expected, float actual, float tolerance, const test_assert_metadata* m);

int test_assert_eq_ptr(const void* expected, const void* actual, const test_assert_metadata* m);

int64_t test_time_freq();

// the current time as expressed in a high resolution tick
int64_t test_time_now();

// compare the delta of a high resolution tick and now, return seconds
double test_time_diff_to_seconds(int64_t tick);

#ifdef __cplusplus
}
#endif
