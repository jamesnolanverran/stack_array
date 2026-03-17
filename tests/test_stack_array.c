#include "test_stack_array.h"

#include "test_framework.h"
#include "../stack_array.h"

#include <string.h>

static sa_error_overflow_fn saved_overflow_handler = NULL;
static sa_error_underflow_fn saved_underflow_handler = NULL;
static int handlers_enabled = 0;

static void test_overflow_handler(size_t cap)
{
    (void)cap;
    tf_expected_error = 1;
}

static void test_underflow_handler(void)
{
    tf_expected_error = 1;
}

static void enable_test_handlers(void)
{
    if (handlers_enabled) {
        return;
    }

    saved_overflow_handler = sa_set_overflow_handler(test_overflow_handler);
    saved_underflow_handler = sa_set_underflow_handler(test_underflow_handler);
    handlers_enabled = 1;
}

static void disable_test_handlers(void)
{
    if (!handlers_enabled) {
        return;
    }

    sa_set_overflow_handler(saved_overflow_handler);
    sa_set_underflow_handler(saved_underflow_handler);

    saved_overflow_handler = NULL;
    saved_underflow_handler = NULL;
    handlers_enabled = 0;
}

/*
===============================================================================
stack_array tests
===============================================================================
*/

static void test_sa_basic_push_pop_peek(void)
{
    stack_array(values, int, 4);

    ASSERT_SIZE_EQ(sa_len(values), 0);
    ASSERT_SIZE_EQ(sa_cap(values), 4);

    int *p0 = sa_push(values, 10);
    int *p1 = sa_push(values, 20);
    int *p2 = sa_push(values, 30);

    ASSERT_TRUE(p0 != NULL);
    ASSERT_TRUE(p1 != NULL);
    ASSERT_TRUE(p2 != NULL);

    ASSERT_SIZE_EQ(sa_len(values), 3);
    ASSERT_EQ(sa_at(values, 0), 10);
    ASSERT_EQ(sa_at(values, 1), 20);
    ASSERT_EQ(sa_at(values, 2), 30);
    ASSERT_EQ(sa_peek(values), 30);

    ASSERT_EQ(sa_pop(values), 30);
    ASSERT_EQ(sa_pop(values), 20);
    ASSERT_EQ(sa_pop(values), 10);
    ASSERT_SIZE_EQ(sa_len(values), 0);
}

static void test_sa_clear_and_reuse(void)
{
    stack_array(values, int, 4);

    sa_push(values, 1);
    sa_push(values, 2);
    sa_push(values, 3);

    ASSERT_SIZE_EQ(sa_len(values), 3);
    sa_clear(values);
    ASSERT_SIZE_EQ(sa_len(values), 0);

    sa_push(values, 99);
    ASSERT_SIZE_EQ(sa_len(values), 1);
    ASSERT_EQ(sa_at(values, 0), 99);
    ASSERT_EQ(sa_peek(values), 99);
}

static void test_sa_append_success(void)
{
    stack_array(dst, int, 5);
    int src[] = {10, 20, 30};

    sa_append(dst, src, 3);

    ASSERT_SIZE_EQ(sa_len(dst), 3);
    ASSERT_EQ(sa_at(dst, 0), 10);
    ASSERT_EQ(sa_at(dst, 1), 20);
    ASSERT_EQ(sa_at(dst, 2), 30);
}

static void test_sa_append_exact_fill(void)
{
    stack_array(dst, int, 4);
    int src[] = {1, 2, 3, 4};

    sa_append(dst, src, 4);

    ASSERT_SIZE_EQ(sa_len(dst), 4);
    ASSERT_SIZE_EQ(sa_cap(dst), 4);
    ASSERT_EQ(sa_at(dst, 0), 1);
    ASSERT_EQ(sa_at(dst, 1), 2);
    ASSERT_EQ(sa_at(dst, 2), 3);
    ASSERT_EQ(sa_at(dst, 3), 4);
}

static void test_sa_append_overflow_does_not_mutate(void)
{
    stack_array(dst, int, 4);
    int src1[] = {10, 20, 30};
    int src2[] = {40, 50};

    sa_append(dst, src1, 3);
    ASSERT_SIZE_EQ(sa_len(dst), 3);

    enable_test_handlers();
    EXPECT_FAIL(sa_append(dst, src2, 2));
    disable_test_handlers();

    ASSERT_SIZE_EQ(sa_len(dst), 3);
    ASSERT_EQ(sa_at(dst, 0), 10);
    ASSERT_EQ(sa_at(dst, 1), 20);
    ASSERT_EQ(sa_at(dst, 2), 30);
}

static void test_sa_overflow_on_push_does_not_mutate(void)
{
    stack_array(values, int, 2);

    sa_push(values, 7);
    sa_push(values, 8);

    ASSERT_SIZE_EQ(sa_len(values), 2);
    ASSERT_EQ(sa_peek(values), 8);

    enable_test_handlers();
    int *result = NULL;
    EXPECT_FAIL(result = sa_push(values, 9));
    disable_test_handlers();

    ASSERT_TRUE(result == NULL);
    ASSERT_SIZE_EQ(sa_len(values), 2);
    ASSERT_EQ(sa_at(values, 0), 7);
    ASSERT_EQ(sa_at(values, 1), 8);
}

static void test_sa_underflow_paths(void)
{
    stack_array(values, int, 2);

    values[0] = 1234; /* defined fallback read */

    enable_test_handlers();
    EXPECT_FAIL((void)sa_pop(values));
    EXPECT_FAIL((void)sa_peek(values));
    disable_test_handlers();

    ASSERT_SIZE_EQ(sa_len(values), 0);
    ASSERT_EQ(values[0], 1234);
}

static void test_sa_at_out_of_bounds(void)
{
    stack_array(values, int, 3);

    sa_push(values, 11);
    sa_push(values, 22);

    values[0] = 11; /* defined fallback read */

    enable_test_handlers();
    EXPECT_FAIL((void)sa_at(values, 2));
    EXPECT_FAIL((void)sa_at(values, 999));
    disable_test_handlers();

    ASSERT_SIZE_EQ(sa_len(values), 2);
    ASSERT_EQ(sa_at(values, 0), 11);
    ASSERT_EQ(sa_at(values, 1), 22);
}

static void test_sa_field_init_and_use(void)
{
    struct FieldHolder {
        sa_field(values, int, 3);
    };

    struct FieldHolder holder = {0};

    holder.values[0] = 77; /* defined fallback read */

    enable_test_handlers();
    EXPECT_FAIL((void)sa_push(holder.values, 11));
    disable_test_handlers();

    sa_field_init(holder.values);

    ASSERT_SIZE_EQ(sa_len(holder.values), 0);
    ASSERT_SIZE_EQ(sa_cap(holder.values), 3);

    sa_push(holder.values, 11);
    sa_push(holder.values, 22);

    ASSERT_SIZE_EQ(sa_len(holder.values), 2);
    ASSERT_EQ(sa_at(holder.values, 0), 11);
    ASSERT_EQ(sa_at(holder.values, 1), 22);
}

/*
===============================================================================
stack_string tests
===============================================================================
*/

static void test_ss_basic_operations(void)
{
    stack_string(str, 32);

    ASSERT_SIZE_EQ(ss_len(str), 0);
    ASSERT_SIZE_EQ(ss_cap(str), 32);

    ss_append(str, "hello");
    ASSERT_STREQ(str, "hello");
    ASSERT_SIZE_EQ(ss_len(str), 5);
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));

    ss_append(str, " world");
    ASSERT_STREQ(str, "hello world");
    ASSERT_SIZE_EQ(ss_len(str), 11);
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));

    ss_pushc(str, '!');
    ASSERT_STREQ(str, "hello world!");
    ASSERT_SIZE_EQ(ss_len(str), 12);
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));

    ss_clear(str);
    ASSERT_SIZE_EQ(ss_len(str), 0);
    ASSERT_STREQ(str, "");
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));
}

static void test_ss_append_n(void)
{
    stack_string(str, 32);

    ss_append_n(str, "helloXXX", 5);
    ASSERT_STREQ(str, "hello");
    ASSERT_SIZE_EQ(ss_len(str), 5);
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));
}

static void test_ss_appendf_success(void)
{
    stack_string(str, 32);

    ss_appendf(str, "%s", "value");
    ASSERT_STREQ(str, "value");
    ASSERT_SIZE_EQ(ss_len(str), strlen("value"));
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));

    ss_appendf(str, " name=%s", "foo");
    ASSERT_STREQ(str, "value name=foo");
    ASSERT_SIZE_EQ(ss_len(str), strlen("value name=foo"));
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));
}

static void test_ss_sprintf_success(void)
{
    stack_string(str, 32);

    ss_append(str, "prefix");
    ASSERT_STREQ(str, "prefix");

    ss_sprintf(str, "%s-%d", "value", 42);
    ASSERT_STREQ(str, "value-42");
    ASSERT_SIZE_EQ(ss_len(str), strlen("value-42"));
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));
}

static void test_ss_exact_fill_boundaries(void)
{
    stack_string(str, 32);
    const char *chars_31 = "abcdefghijklmnopqrstuvwxyz12345";

    ASSERT_SIZE_EQ(strlen(chars_31), 31);

    ss_appendf(str, "%s", chars_31);
    ASSERT_STREQ(str, chars_31);
    ASSERT_SIZE_EQ(ss_len(str), 31);
    ASSERT_SIZE_EQ(ss_cap(str), 32);
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));

    enable_test_handlers();
    EXPECT_FAIL(ss_appendf(str, "%s", "x"));
    disable_test_handlers();

    ASSERT_STREQ(str, chars_31);
    ASSERT_SIZE_EQ(ss_len(str), 31);
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));
}

static void test_ss_sprintf_full_and_overflow(void)
{
    stack_string(str, 32);
    const char *chars_31 = "abcdefghijklmnopqrstuvwxyz12345";

    ASSERT_SIZE_EQ(strlen(chars_31), 31);

    ss_sprintf(str, "%s", chars_31);
    ASSERT_STREQ(str, chars_31);
    ASSERT_SIZE_EQ(ss_len(str), 31);
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));

    enable_test_handlers();
    EXPECT_FAIL(ss_sprintf(str, "%s%s", chars_31, "x"));
    disable_test_handlers();

    ASSERT_STREQ(str, chars_31);
    ASSERT_SIZE_EQ(ss_len(str), 31);
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));
}

static void test_ss_append_overflow_does_not_mutate(void)
{
    stack_string(str, 8);

    ss_append(str, "abc");
    ASSERT_STREQ(str, "abc");
    ASSERT_SIZE_EQ(ss_len(str), 3);

    enable_test_handlers();
    EXPECT_FAIL(ss_append(str, "defgh"));
    disable_test_handlers();

    ASSERT_STREQ(str, "abc");
    ASSERT_SIZE_EQ(ss_len(str), 3);
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));
}

static void test_ss_append_n_overflow_does_not_mutate(void)
{
    stack_string(str, 8);

    ss_append(str, "abc");
    ASSERT_STREQ(str, "abc");
    ASSERT_SIZE_EQ(ss_len(str), 3);

    enable_test_handlers();
    EXPECT_FAIL(ss_append_n(str, "vwxyz", 5));
    disable_test_handlers();

    ASSERT_STREQ(str, "abc");
    ASSERT_SIZE_EQ(ss_len(str), 3);
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));
}

static void test_ss_pushc_overflow_does_not_mutate(void)
{
    stack_string(str, 4);

    ss_append(str, "abc");
    ASSERT_STREQ(str, "abc");
    ASSERT_SIZE_EQ(ss_len(str), 3);

    enable_test_handlers();
    EXPECT_FAIL(ss_pushc(str, '!'));
    disable_test_handlers();

    ASSERT_STREQ(str, "abc");
    ASSERT_SIZE_EQ(ss_len(str), 3);
    ASSERT_SIZE_EQ(ss_len(str), strlen(str));
}

static void test_ss_field_init_and_use(void)
{
    struct Label {
        int age;
        ss_field(name, 32);
    };

    struct Label label = {0};
    label.age = 20;

    label.name[0] = '\0';

    enable_test_handlers();
    EXPECT_FAIL(ss_append(label.name, "hi"));
    disable_test_handlers();

    ss_field_init(label.name);

    ASSERT_SIZE_EQ(ss_len(label.name), 0);
    ASSERT_SIZE_EQ(ss_cap(label.name), 32);

    ss_append(label.name, "hi - field initialized");
    ASSERT_STREQ(label.name, "hi - field initialized");
    ASSERT_SIZE_EQ(ss_len(label.name), strlen(label.name));
}

/*
===============================================================================
Error handler setter tests
===============================================================================
*/

static int custom_overflow_called = 0;
static int custom_underflow_called = 0;

static void custom_overflow_handler(size_t cap)
{
    (void)cap;
    custom_overflow_called++;
}

static void custom_underflow_handler(void)
{
    custom_underflow_called++;
}

static void test_error_handler_setters_return_previous_and_restore(void)
{
    stack_array(values, int, 1);
    values[0] = 0;

    custom_overflow_called = 0;
    custom_underflow_called = 0;

    sa_error_overflow_fn default_overflow =
        sa_set_overflow_handler(custom_overflow_handler);
    sa_error_underflow_fn default_underflow =
        sa_set_underflow_handler(custom_underflow_handler);

    ASSERT_TRUE(default_overflow != NULL);
    ASSERT_TRUE(default_underflow != NULL);

    sa_push(values, 1);
    ASSERT_SIZE_EQ(sa_len(values), 1);

    {
        int *result = sa_push(values, 2);
        ASSERT_TRUE(result == NULL);
    }
    ASSERT_EQ(custom_overflow_called, 1);

    sa_pop(values);
    ASSERT_SIZE_EQ(sa_len(values), 0);

    (void)sa_pop(values);
    ASSERT_EQ(custom_underflow_called, 1);

    {
        sa_error_overflow_fn restored_overflow =
            sa_set_overflow_handler(default_overflow);
        sa_error_underflow_fn restored_underflow =
            sa_set_underflow_handler(default_underflow);

        ASSERT_TRUE(restored_overflow == custom_overflow_handler);
        ASSERT_TRUE(restored_underflow == custom_underflow_handler);
    }

    {
        sa_error_overflow_fn prev_overflow = sa_set_overflow_handler(NULL);
        sa_error_underflow_fn prev_underflow = sa_set_underflow_handler(NULL);

        ASSERT_TRUE(prev_overflow == default_overflow);
        ASSERT_TRUE(prev_underflow == default_underflow);
    }
}

int stack_array_run_tests(void)
{
    RUN_TEST(test_sa_basic_push_pop_peek);
    RUN_TEST(test_sa_clear_and_reuse);
    RUN_TEST(test_sa_append_success);
    RUN_TEST(test_sa_append_exact_fill);
    RUN_TEST(test_sa_append_overflow_does_not_mutate);
    RUN_TEST(test_sa_overflow_on_push_does_not_mutate);
    RUN_TEST(test_sa_underflow_paths);
    RUN_TEST(test_sa_at_out_of_bounds);
    RUN_TEST(test_sa_field_init_and_use);

    RUN_TEST(test_ss_basic_operations);
    RUN_TEST(test_ss_append_n);
    RUN_TEST(test_ss_appendf_success);
    RUN_TEST(test_ss_sprintf_success);
    RUN_TEST(test_ss_exact_fill_boundaries);
    RUN_TEST(test_ss_sprintf_full_and_overflow);
    RUN_TEST(test_ss_append_overflow_does_not_mutate);
    RUN_TEST(test_ss_append_n_overflow_does_not_mutate);
    RUN_TEST(test_ss_pushc_overflow_does_not_mutate);
    RUN_TEST(test_ss_field_init_and_use);

    RUN_TEST(test_error_handler_setters_return_previous_and_restore);

    disable_test_handlers();
    return tf_summary();
}
