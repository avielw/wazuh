/*
 * Copyright (C) 2015-2019, Wazuh Inc.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <string.h>
#include "../wazuh_db/wdb.h"
#include "../headers/shared.h"
#include "../os_crypto/sha1/sha1_op.h"
#include "../external/sqlite/sqlite3.h"

/* setup/teardown */
static int setup_wdb_t(void **state) {
    wdb_t *data = calloc(1, sizeof(wdb_t));

    if(!data) {
        return -1;
    }

    *state = data;
    return 0;
}

static int teardown_wdb_t(void **state) {
    wdb_t *data = *state;

    if(data) {
        os_free(data->agent_id);
        os_free(data);
    }

    return 0;
}

/* redefinitons/wrapping */
void __wrap__mdebug1(const char * file, int line, const char * func, const char *msg, ...) {
    char *param1;
    char *param2;
    va_list args;
    const char *aux = msg;
    int i = 0;

    va_start(args, msg);

    while(aux = strchr(aux, '%'), aux) {
        i++;
        aux++;
    }

    if(i) {
        param1 = va_arg(args, char*);
        check_expected(param1);
        i--;
    }
    if(i) {
        param2 = va_arg(args, char*);
        check_expected(param2);
    }
    va_end(args);

    check_expected(msg);
    return;
}

int __wrap_wdb_stmt_cache(wdb_t * wdb, int index)
{
    return mock();
}

int __wrap_sqlite3_step(sqlite3_stmt * stmt)
{
    return mock();
}

const char * __wrap_sqlite3_errmsg(sqlite3 *db)
{
    return mock_type(const char*);
}

int __wrap_EVP_DigestInit(EVP_MD_CTX *ctx, const EVP_MD *type)
{
    return mock();
}

int __wrap_EVP_DigestInit_ex(EVP_MD_CTX *ctx, const EVP_MD *type, ENGINE *impl)
{
    return mock();
}
int __wrap_EVP_DigestUpdate(EVP_MD_CTX *ctx, const void *d, size_t cnt)
{
    return mock();
}

int __wrap_EVP_DigestFinal_ex(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s)
{
    return mock();
}

const unsigned char * __wrap_sqlite3_column_text(sqlite3_stmt * stmt, int iCol)
{
    return mock_type(const unsigned char *);
}

/* tests */
static void test_wdbi_checksum_range_wbs_null(void **state)
{
    (void) state; /* unused */
    int ret;
    os_sha1 test_hex = "";
    will_return(__wrap_wdb_stmt_cache, 1);
    ret = wdbi_checksum_range(NULL, 0, "test_begin", "test_end",test_hex);
    assert_int_equal(ret, -1);
}

static void test_wdbi_checksum_range_begin_null(void **state)
{
    int ret;

    wdb_t * data = *state;
    data->agent_id = strdup("000");
    os_sha1 test_hex = "";
    will_return(__wrap_wdb_stmt_cache, 1);
    will_return(__wrap_sqlite3_step, 0); 
    ret = wdbi_checksum_range(data, 0, NULL, "test_end",test_hex);
    assert_int_equal(ret, 0);
}

static void test_wdbi_checksum_range_end_null(void **state)
{
    int ret;

    wdb_t * data = *state;
    data->agent_id = strdup("000");
    os_sha1 test_hex = "";
    will_return(__wrap_wdb_stmt_cache, 1);
    will_return(__wrap_sqlite3_step, 0);
    ret = wdbi_checksum_range(data, 0, "test_begin", NULL, test_hex);
    assert_int_equal(ret, 0);
}

static void test_wdbi_checksum_range_hexdigest_null(void **state)
{
    int ret;

    wdb_t * data = *state;
    data->agent_id = strdup("000");
    will_return(__wrap_wdb_stmt_cache, 1);
    will_return(__wrap_sqlite3_step, 100);
    will_return(__wrap_sqlite3_step, 0);
    ret = wdbi_checksum_range(data, 0, "test_begin", "test_end", NULL); 
    assert_int_equal(ret, 1);
}

static void test_wdbi_checksum_range_wdb_stmt_cache_fail(void **state)
{
    int ret;

    wdb_t * data = *state;
    data->agent_id = strdup("000");
    os_sha1 test_hex = "";
    will_return(__wrap_wdb_stmt_cache, -1);
    ret = wdbi_checksum_range(data, 0, "test_begin", "test_end", test_hex);
    assert_int_equal(ret, -1);
}

static void test_wdbi_checksum_range_no_row(void **state)
{
    int ret;

    wdb_t * data = *state;
    data->agent_id = strdup("000");
    os_sha1 test_hex = "";
    will_return(__wrap_wdb_stmt_cache, 1);
    will_return(__wrap_sqlite3_step, 0);
    ret = wdbi_checksum_range(data, 0, "test_begin", "test_end", test_hex);
    assert_int_equal(ret, 0);
}

static void test_wdbi_checksum_range_success(void **state)
{
    int ret;

    wdb_t * data = *state;
    data->agent_id = strdup("000");
    os_sha1 test_hex = {5,5,0,8,6,'c','e','f',9,'c',8,7,'d',6,'d',0,3,1,'c','d',5,'d','b',2,9,'c','d',0,3,'a',2,'e','d',0,2,5,2,'b',4,5};
    will_return(__wrap_wdb_stmt_cache, 1);
    will_return(__wrap_sqlite3_step, 100);
    will_return(__wrap_sqlite3_step, 0);
    ret = wdbi_checksum_range(data, 0, "test_begin", "test_end", test_hex); 
    assert_int_equal(ret, 1);
}

static void test_wdbi_delete_wbs_null(void **state)
{
    (void) state; /* unused */
    int ret;
    will_return(__wrap_wdb_stmt_cache, 1);
    ret = wdbi_delete(NULL, 0, "test_begin", "test_end","test_tail");
    assert_int_equal(ret, -1);
}

static void test_wdbi_delete_begin_null(void **state)
{
    int ret;

    wdb_t * data = *state;
    data->agent_id = strdup("000");
    will_return(__wrap_wdb_stmt_cache, 1);
    will_return(__wrap_sqlite3_step, 0);
    will_return(__wrap_sqlite3_errmsg, 0);
    ret = wdbi_delete(data, 0, NULL, "test_end",0);
    assert_int_equal(ret, -1);
}

static void test_wdbi_delete_end_null(void **state)
{
    int ret;

    wdb_t * data = *state;
    data->agent_id = strdup("000");
    will_return(__wrap_wdb_stmt_cache, 1);
    will_return(__wrap_sqlite3_step, 0);
    will_return(__wrap_sqlite3_errmsg, 0);
    ret = wdbi_delete(data, 0, "test_begin", NULL, "test_tail");
    assert_int_equal(ret, -1);
}

static void test_wdbi_delete_tail_null(void **state)
{
    int ret;

    wdb_t * data = *state;
    data->agent_id = strdup("000");
    will_return(__wrap_wdb_stmt_cache, 1);
    will_return(__wrap_sqlite3_step, 0);
    will_return(__wrap_sqlite3_errmsg, 0);
    ret = wdbi_delete(data, 0, "test_begin", "test_end",NULL);
    assert_int_equal(ret, -1);
}

static void test_wdbi_delete_wdb_stmt_cache_fail(void **state)
{
    int ret;

    wdb_t * data = *state;
    data->agent_id = strdup("000");
    will_return(__wrap_wdb_stmt_cache, -1);
    ret = wdbi_delete(data, 0, "test_begin", "test_end","test_tail");
    assert_int_equal(ret, -1);
}

static void test_wdbi_delete_sql_no_done(void **state)
{
    int ret;

    wdb_t * data = *state;
    data->agent_id = strdup("000");
    will_return(__wrap_wdb_stmt_cache, 1);
    will_return(__wrap_sqlite3_step, 0);
    will_return(__wrap_sqlite3_errmsg, 0);
    ret = wdbi_delete(data, 0, "test_begin", "test_end",NULL);
    assert_int_equal(ret, -1);
}

static void test_wdbi_delete_success(void **state)
{
    int ret;

    wdb_t * data = *state;
    data->agent_id = strdup("000");
    will_return(__wrap_wdb_stmt_cache, 1);
    will_return(__wrap_sqlite3_step, 101);
    ret = wdbi_delete(data, 0, "test_begin", "test_end",NULL);
    assert_int_equal(ret, 0);
}

static void test_wdbi_update_attempt_wbs_null(void **state)
{
    (void) state; /* unused */

    will_return(__wrap_wdb_stmt_cache, 1);
    wdbi_update_attempt(NULL, 0, 1);
}

static void test_wdbi_update_attempt_stmt_cache_fail(void **state)
{
    wdb_t * data = *state;
    data->agent_id = strdup("000");
    will_return(__wrap_wdb_stmt_cache, -1);
    wdbi_update_attempt(data, 0, 0);
}

static void test_wdbi_update_attempt_no_sql_done(void **state)
{
    wdb_t * data = *state;
    data->agent_id = strdup("000");
    will_return(__wrap_wdb_stmt_cache, 1);
    will_return(__wrap_sqlite3_step, 1);
    will_return(__wrap_sqlite3_errmsg, 0);
    wdbi_update_attempt(data, 0, 0);
}

static void test_wdbi_update_attempt_success(void **state)
{
    wdb_t * data = *state;
    data->agent_id = strdup("000");
    will_return(__wrap_wdb_stmt_cache, 1);
    will_return(__wrap_sqlite3_step, 101);
    wdbi_update_attempt(data, 0, 0);
}

static void test_wdbi_update_completion_wbs_null(void **state)
{
    (void) state; /* unused */

    will_return(__wrap_wdb_stmt_cache, 1);
    wdbi_update_attempt(NULL, 0, 1);
}

static void test_wdbi_update_completion_stmt_cache_fail(void **state)
{
    wdb_t * data = *state;
    data->agent_id = strdup("000");
    will_return(__wrap_wdb_stmt_cache, -1);
    wdbi_update_attempt(data, 0, 0);
}

static void test_wdbi_update_completion_no_sql_done(void **state)
{
    wdb_t * data = *state;
    data->agent_id = strdup("000");
    will_return(__wrap_wdb_stmt_cache, 1);
    will_return(__wrap_sqlite3_step, 1);
    will_return(__wrap_sqlite3_errmsg, 0);
    wdbi_update_attempt(data, 0, 0);
}

static void test_wdbi_update_completion_success(void **state)
{
    wdb_t * data = *state;
    data->agent_id = strdup("000");
    will_return(__wrap_wdb_stmt_cache, 1);
    will_return(__wrap_sqlite3_step, 101);
    wdbi_update_attempt(data, 0, 0);
}

/* This test must fail with a SEGFAULT */
void test_wdbi_query_clear_null_payload(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    char * payload = NULL;

    ret = wdbi_query_clear(data, WDB_FIM, payload);

    assert_int_equal(ret, -1);
}

void test_wdbi_query_clear_invalid_payload(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    char payload[] = "This is some test";
    
    expect_string(__wrap__mdebug1, msg, "DB(%s): cannot parse checksum range payload: '%s'");
    expect_string(__wrap__mdebug1, param1, "000");
    expect_string(__wrap__mdebug1, param2, "This is some test");

    ret = wdbi_query_clear(data, WDB_FIM, payload);

    assert_int_equal(ret, -1);
}

void test_wdbi_query_clear_no_id(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    char payload[] = "{\"Key\":\"Value\"}";
    
    expect_string(__wrap__mdebug1, msg, "No such string 'id' in JSON payload.");

    ret = wdbi_query_clear(data, WDB_FIM, payload);

    assert_int_equal(ret, -1);
}

void test_wdbi_query_clear_stmt_cache_error(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    const char *payload = "{\"id\":5678}";

    will_return(__wrap_wdb_stmt_cache, -1);

    ret = wdbi_query_clear(data, WDB_FIM, payload);

    assert_int_equal(ret, -1);
}

void test_wdbi_query_clear_sql_step_error(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    const char *payload = "{\"id\":5678}";

    will_return(__wrap_wdb_stmt_cache, 0);
    will_return(__wrap_sqlite3_step, 0);
    will_return(__wrap_sqlite3_errmsg, "Fake error");

    expect_string(__wrap__mdebug1, msg, "DB(%s) sqlite3_step(): %s");
    expect_string(__wrap__mdebug1, param1, "000");
    expect_string(__wrap__mdebug1, param2, "Fake error");

    ret = wdbi_query_clear(data, WDB_FIM, payload);

    assert_int_equal(ret, -1);
}

void test_wdbi_query_clear_ok(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    const char *payload = "{\"id\":5678}";

    will_return(__wrap_wdb_stmt_cache, 0);
    will_return(__wrap_sqlite3_step, 101);
    will_return(__wrap_wdb_stmt_cache, -1);

    ret = wdbi_query_clear(data, WDB_FIM, payload);

    assert_int_equal(ret, 0);
}

//SEGFAULT
void test_wdbi_query_checksum_null_payload(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    char * payload = NULL;

    ret = wdbi_query_checksum(data, WDB_FIM, "integrity_check_global", payload);

    assert_int_equal(ret, -1);
}

void test_wdbi_query_checksum_no_begin(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    const char * payload = "{\"Bad\":\"Payload\"}";

    expect_string(__wrap__mdebug1, msg, "No such string 'begin' in JSON payload.");

    ret = wdbi_query_checksum(data, WDB_FIM, "integrity_check_global", payload);
    

    assert_int_equal(ret, -1);
}

void test_wdbi_query_checksum_no_end(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    const char * payload = "{\"begin\":\"something\"}";

    expect_string(__wrap__mdebug1, msg, "No such string 'end' in JSON payload.");
    
    ret = wdbi_query_checksum(data, WDB_FIM, "integrity_check_global", payload);
    

    assert_int_equal(ret, -1);
}

void test_wdbi_query_checksum_no_checksum(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    const char * payload = "{\"begin\":\"something\",\"end\":\"something\"}";

    expect_string(__wrap__mdebug1, msg, "No such string 'checksum' in JSON payload.");
    
    ret = wdbi_query_checksum(data, WDB_FIM, "integrity_check_global", payload);
    

    assert_int_equal(ret, -1);
}

void test_wdbi_query_checksum_no_id(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    const char * payload = "{\"begin\":\"something\",\"end\":\"something\",\"checksum\":\"something\"}";

    expect_string(__wrap__mdebug1, msg, "No such string 'id' in JSON payload.");
    
    ret = wdbi_query_checksum(data, WDB_FIM, "integrity_check_global", payload);
    

    assert_int_equal(ret, -1);
}

void test_wdbi_query_checksum_range_fail(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    const char * payload = "{\"begin\":\"something\",\"end\":\"something\",\"checksum\":\"something\",\"id\":1234}";
    
    will_return(__wrap_wdb_stmt_cache, -1);

    ret = wdbi_query_checksum(data, WDB_FIM, "integrity_check_global", payload);

    assert_int_equal(ret, -1);
}

void test_wdbi_query_checksum_diff_hexdigest(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    const char * payload = "{\"begin\":\"something\",\"end\":\"something\",\"checksum\":\"something\",\"id\":1234}";
    
    will_return(__wrap_wdb_stmt_cache, 0);
    will_return(__wrap_sqlite3_step, 100);
    will_return(__wrap_sqlite3_column_text, NULL);
    will_return(__wrap_sqlite3_step, 101);
    will_return(__wrap_wdb_stmt_cache, -1);
    will_return(__wrap_wdb_stmt_cache, -1);

    expect_string(__wrap__mdebug1, msg, "DB(%s) has a NULL %s checksum.");
    expect_string(__wrap__mdebug1, param1, "000");
    expect_string(__wrap__mdebug1, param2, "fim");

    ret = wdbi_query_checksum(data, WDB_FIM, "integrity_check_global", payload);

    assert_int_equal(ret, 1);
}

void test_wdbi_query_checksum_equal_hexdigest(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    const char * payload = "{\"begin\":\"something\",\"end\":\"something\",\"checksum\":\"da39a3ee5e6b4b0d3255bfef95601890afd80709\",\"id\":1234}";
    
    will_return(__wrap_wdb_stmt_cache, 0);
    will_return(__wrap_sqlite3_step, 100);
    will_return(__wrap_sqlite3_column_text, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    will_return(__wrap_EVP_DigestUpdate, 0);
    will_return(__wrap_sqlite3_step, 101);
    will_return(__wrap_wdb_stmt_cache, -1);
    will_return(__wrap_wdb_stmt_cache, -1);

    ret = wdbi_query_checksum(data, WDB_FIM, "integrity_check_global", payload);

    assert_int_equal(ret, 2);
}

void test_wdbi_query_checksum_bad_command(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    const char * payload = "{\"begin\":\"something\",\"end\":\"something\",\"checksum\":\"something\",\"id\":1234}";
    
    will_return(__wrap_wdb_stmt_cache, 0);
    will_return(__wrap_sqlite3_step, 100);
    will_return(__wrap_sqlite3_column_text, NULL);
    will_return(__wrap_sqlite3_step, 101);

    expect_string(__wrap__mdebug1, msg, "DB(%s) has a NULL %s checksum.");
    expect_string(__wrap__mdebug1, param1, "000");
    expect_string(__wrap__mdebug1, param2, "fim");

    ret = wdbi_query_checksum(data, WDB_FIM, "bad_command", payload);

    assert_int_equal(ret, 1);
}

void test_wdbi_query_checksum_check_left_no_tail(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    const char * payload = "{\"begin\":\"something\",\"end\":\"something\",\"checksum\":\"something\",\"id\":1234}";
    
    will_return(__wrap_wdb_stmt_cache, 0);
    will_return(__wrap_sqlite3_step, 100);
    will_return(__wrap_sqlite3_column_text, "something");
    will_return(__wrap_EVP_DigestUpdate, 0);
    will_return(__wrap_sqlite3_step, 101);
    will_return(__wrap_wdb_stmt_cache, -1);

    ret = wdbi_query_checksum(data, WDB_FIM, "integrity_check_left", payload);

    assert_int_equal(ret, 1);
}

void test_wdbi_query_checksum_check_left_ok(void **state)
{
    wdb_t *data = *state;
    int ret;
    os_strdup("000", data->agent_id);
    const char * payload = "{\"begin\":\"something\",\"end\":\"something\",\"checksum\":\"something\",\"id\":1234,\"tail\":\"something\"}";
    
    will_return(__wrap_wdb_stmt_cache, 0);
    will_return(__wrap_sqlite3_step, 100);
    will_return(__wrap_sqlite3_column_text, "something");
    will_return(__wrap_EVP_DigestUpdate, 0);
    will_return(__wrap_sqlite3_step, 101);
    will_return(__wrap_wdb_stmt_cache, -1);

    ret = wdbi_query_checksum(data, WDB_FIM, "integrity_check_left", payload);

    assert_int_equal(ret, 1);
}


int main(void) {
    const struct CMUnitTest tests[] = {           
        //Test wdbi_checksum_range
        cmocka_unit_test_setup_teardown(test_wdbi_checksum_range_wbs_null, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_checksum_range_begin_null, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_checksum_range_end_null, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_checksum_range_hexdigest_null, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_checksum_range_wdb_stmt_cache_fail, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_checksum_range_no_row, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_checksum_range_success, setup_wdb_t, teardown_wdb_t),

        //Test wdbi_delete
        cmocka_unit_test_setup_teardown(test_wdbi_delete_wbs_null, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_delete_begin_null, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_delete_end_null, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_delete_tail_null, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_delete_wdb_stmt_cache_fail, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_delete_sql_no_done, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_delete_success, setup_wdb_t, teardown_wdb_t),

        //Test  wdbi_update_attempt 
        cmocka_unit_test_setup_teardown(test_wdbi_update_attempt_wbs_null, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_update_attempt_stmt_cache_fail, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_update_attempt_no_sql_done, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_update_attempt_success, setup_wdb_t, teardown_wdb_t),

        //Test wdbi_update_completion
        cmocka_unit_test_setup_teardown(test_wdbi_update_completion_wbs_null, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_update_completion_stmt_cache_fail, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_update_completion_no_sql_done, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_update_completion_success, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_clear_null_payload, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_clear_invalid_payload, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_clear_no_id, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_clear_stmt_cache_error, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_clear_sql_step_error, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_clear_ok, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_checksum_null_payload, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_checksum_no_begin, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_checksum_no_end, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_checksum_no_checksum, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_checksum_no_id, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_checksum_range_fail, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_checksum_diff_hexdigest, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_checksum_equal_hexdigest, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_checksum_bad_command, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_checksum_check_left_no_tail, setup_wdb_t, teardown_wdb_t),
        cmocka_unit_test_setup_teardown(test_wdbi_query_checksum_check_left_ok, setup_wdb_t, teardown_wdb_t),

    };  
    return cmocka_run_group_tests(tests, NULL, NULL);
}