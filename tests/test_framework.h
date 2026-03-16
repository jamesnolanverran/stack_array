#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tf_tests_run = 0;
static int tf_tests_failed = 0;
static int tf_assertions_failed = 0;
static int tf_expected_checks_run = 0;

#define TF_COLOR_RED    "\x1b[31m"
#define TF_COLOR_GREEN  "\x1b[32m"
#define TF_COLOR_YELLOW "\x1b[33m"
#define TF_COLOR_RESET  "\x1b[0m"

#define TF_FAIL_NOW(...)                                                  \
do {                                                                      \
    fprintf(stderr, TF_COLOR_RED __VA_ARGS__);                            \
    fprintf(stderr, TF_COLOR_RESET);                                      \
    tf_assertions_failed++;                                               \
    return;                                                               \
} while (0)

/*
===============================================================================
Core assertion helpers
===============================================================================
*/

#define ASSERT_TRUE(x)                                                     \
do {                                                                       \
    if (!(x)) {                                                            \
        TF_FAIL_NOW("ASSERT FAIL %s:%d: %s\n",                             \
                    __FILE__, __LINE__, #x);                               \
    }                                                                      \
} while (0)

#define ASSERT_FALSE(x) ASSERT_TRUE(!(x))

#define ASSERT_EQ(a,b)                                                     \
do {                                                                       \
    long _a = (long)(a);                                                   \
    long _b = (long)(b);                                                   \
    if (_a != _b) {                                                        \
        TF_FAIL_NOW("ASSERT FAIL %s:%d: %s (%ld) != %s (%ld)\n",           \
                    __FILE__, __LINE__, #a, _a, #b, _b);                   \
    }                                                                      \
} while (0)

#define ASSERT_NE(a,b)                                                     \
do {                                                                       \
    long _a = (long)(a);                                                   \
    long _b = (long)(b);                                                   \
    if (_a == _b) {                                                        \
        TF_FAIL_NOW("ASSERT FAIL %s:%d: %s == %s (%ld)\n",                 \
                    __FILE__, __LINE__, #a, #b, _a);                       \
    }                                                                      \
} while (0)

#define ASSERT_SIZE_EQ(a,b)                                                \
do {                                                                       \
    size_t _a = (size_t)(a);                                               \
    size_t _b = (size_t)(b);                                               \
    if (_a != _b) {                                                        \
        TF_FAIL_NOW("ASSERT FAIL %s:%d: %s (%zu) != %s (%zu)\n",           \
                    __FILE__, __LINE__, #a, _a, #b, _b);                   \
    }                                                                      \
} while (0)

#define ASSERT_STREQ(a,b)                                                  \
do {                                                                       \
    if (strcmp((a),(b)) != 0) {                                            \
        TF_FAIL_NOW("ASSERT FAIL %s:%d:\n'%s'\n!=\n'%s'\n",                \
                    __FILE__, __LINE__, (a), (b));                         \
    }                                                                      \
} while (0)

/*
===============================================================================
Expected failure support
===============================================================================
*/

static int tf_expected_error = 0;

#define EXPECT_FAIL(stmt)                                                  \
do {                                                                       \
    tf_expected_checks_run++;                                              \
    tf_expected_error = 0;                                                 \
    stmt;                                                                  \
    if (!tf_expected_error) {                                              \
        TF_FAIL_NOW("EXPECTED-ERROR FAIL %s:%d: %s\n",                     \
                    __FILE__, __LINE__, #stmt);                            \
    } else {                                                               \
        printf(TF_COLOR_YELLOW "EXPECTED-ERROR PASS %s\n"                  \
               TF_COLOR_RESET, #stmt);                                     \
    }                                                                      \
} while (0)

/*
===============================================================================
Test runner
===============================================================================
*/

#define RUN_TEST(fn)                                                       \
do {                                                                       \
    tf_tests_run++;                                                        \
    int before_asserts = tf_assertions_failed;                             \
    fn();                                                                  \
    if (tf_assertions_failed == before_asserts) {                          \
        printf(TF_COLOR_GREEN "PASS %s\n" TF_COLOR_RESET, #fn);            \
    } else {                                                               \
        tf_tests_failed++;                                                 \
        printf(TF_COLOR_RED "FAIL %s\n" TF_COLOR_RESET, #fn);              \
    }                                                                      \
} while (0)

static inline int tf_summary(void)
{
    printf("\nTests run: %d\n", tf_tests_run);
    printf("Expected-error checks: %d\n", tf_expected_checks_run);
    printf("Assertion failures: %d\n", tf_assertions_failed);

    if (tf_tests_failed) {
        printf(TF_COLOR_RED "Tests failed: %d\n" TF_COLOR_RESET,
               tf_tests_failed);
        return 1;
    }

    printf(TF_COLOR_GREEN "All tests passed\n" TF_COLOR_RESET);
    return 0;
}
