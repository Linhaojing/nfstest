#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>

static int tests_passed = 0;
static int tests_failed = 0;
static int tests_skipped = 0;
static const char* current_test_name = NULL;

#define TEST(name) \
    static void test_##name(void); \
    __attribute__((constructor)) static void register_##name(void) { \
        test_##name(); \
    } \
    static void test_##name(void)

#define RUN_TEST(name) do { \
    current_test_name = #name; \
    printf("  RUN   %s ... ", #name); \
    fflush(stdout); \
    name(); \
} while(0)

#define ASSERT_TRUE(cond, msg) do { \
    if (!(cond)) { \
        printf("\n  FAIL  %s: %s (line %d)\n", current_test_name, (msg), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ_INT(a, b, msg) do { \
    int _a = (a); int _b = (b); \
    if (_a != _b) { \
        printf("\n  FAIL  %s: %s - expected %d, got %d (line %d)\n", \
               current_test_name, (msg), _b, _a, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ_U32(a, b, msg) do { \
    uint32_t _a = (a); uint32_t _b = (b); \
    if (_a != _b) { \
        printf("\n  FAIL  %s: %s - expected %u, got %u (line %d)\n", \
               current_test_name, (msg), _b, _a, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ_U64(a, b, msg) do { \
    uint64_t _a = (a); uint64_t _b = (b); \
    if (_a != _b) { \
        printf("\n  FAIL  %s: %s - expected %lu, got %lu (line %d)\n", \
               current_test_name, (msg), (unsigned long)_b, (unsigned long)_a, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_STREQ(a, b, msg) do { \
    const char* _a = (a); const char* _b = (b); \
    if (!_a || !_b || strcmp(_a, _b) != 0) { \
        printf("\n  FAIL  %s: %s - expected '%s', got '%s' (line %d)\n", \
               current_test_name, (msg), _b ? _b : "(null)", _a ? _a : "(null)", __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_MEMEQ(a, b, len, msg) do { \
    if (memcmp((a), (b), (len)) != 0) { \
        printf("\n  FAIL  %s: %s - memory differs (line %d)\n", \
               current_test_name, (msg), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define TEST_PASS() do { \
    printf("OK\n"); \
    tests_passed++; \
} while(0)

#define TEST_SKIP(msg) do { \
    printf("SKIP: %s\n", (msg)); \
    tests_skipped++; \
    return; \
} while(0)

#define PRINT_SUMMARY() do { \
    printf("\n========================================\n"); \
    printf("  Results: %d passed, %d failed, %d skipped\n", tests_passed, tests_failed, tests_skipped); \
    printf("========================================\n"); \
} while(0)

#endif
