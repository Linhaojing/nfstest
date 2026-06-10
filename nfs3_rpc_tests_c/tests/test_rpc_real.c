#include "test_framework.h"
#include "nfstest/rpc_client.h"
#include "nfs3_c/nfs3_xdr.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define RPCBPROG 100000U
#define RPCBVERS 2U
#define RPCBPROC_GETPORT 3U
#define IPPROTO_TCP_NUM 6U

#define MOUNT_PROGRAM 100005U
#define MOUNT_V3 3U
#define MOUNTPROC3_MNT 1U

#define TEST_HOST "127.0.0.1"
#define NFS_PORT 2049U
#define EXPORT_PATH "/srv/nfs"

static int rpc_call_bytes(
    const char* host,
    uint16_t port,
    uint32_t prog,
    uint32_t vers,
    uint32_t proc,
    const uint8_t* args,
    size_t args_len,
    uint8_t** resp,
    size_t* resp_len
) {
    nfstest_rpc_client_t* client = nfstest_rpc_connect(host, port, 3000);
    ASSERT_TRUE(client != NULL, "connect rpc service");

    int status = nfstest_rpc_call(
        client,
        prog,
        vers,
        proc,
        args,
        args_len,
        resp,
        resp_len,
        3000
    );

    nfstest_rpc_disconnect(client);
    return status;
}

static uint16_t get_rpc_service_port(uint32_t prog, uint32_t vers) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    xdr_pack_uint32(&args, prog);
    xdr_pack_uint32(&args, vers);
    xdr_pack_uint32(&args, IPPROTO_TCP_NUM);
    xdr_pack_uint32(&args, 0);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = rpc_call_bytes(
        TEST_HOST,
        111,
        RPCBPROG,
        RPCBVERS,
        RPCBPROC_GETPORT,
        xdr_buf_data(&args),
        xdr_buf_size(&args),
        &resp,
        &resp_len
    );
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "portmap GETPORT should succeed");
    ASSERT_TRUE(resp != NULL, "portmap response data");

    xdr_buf_t body;
    xdr_buf_init_copy(&body, resp, resp_len);
    uint32_t port = 0;
    xdr_unpack_uint32(&body, &port);

    free(resp);
    xdr_buf_destroy(&body);
    xdr_buf_destroy(&args);

    ASSERT_TRUE(port > 0 && port <= 65535, "service port should be valid");
    return (uint16_t)port;
}

static void mount_root(nfs_fh3_t* root_fh) {
    uint16_t mount_port = get_rpc_service_port(MOUNT_PROGRAM, MOUNT_V3);

    xdr_buf_t args;
    xdr_buf_init(&args);
    xdr_pack_cstring(&args, EXPORT_PATH);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = rpc_call_bytes(
        TEST_HOST,
        mount_port,
        MOUNT_PROGRAM,
        MOUNT_V3,
        MOUNTPROC3_MNT,
        xdr_buf_data(&args),
        xdr_buf_size(&args),
        &resp,
        &resp_len
    );
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "MOUNT MNT should succeed");
    ASSERT_TRUE(resp != NULL, "mount response data");

    xdr_buf_t body;
    xdr_buf_init_copy(&body, resp, resp_len);
    uint32_t mount_status = 0;
    xdr_unpack_uint32(&body, &mount_status);
    ASSERT_EQ_U32(mount_status, 0, "MOUNT status should be OK");
    xdr_unpack_nfs_fh3(&body, root_fh);
    ASSERT_TRUE(!nfs_fh3_is_empty(root_fh), "root file handle should not be empty");

    free(resp);
    xdr_buf_destroy(&body);
    xdr_buf_destroy(&args);
}

static int nfs_call_xdr(
    uint32_t proc,
    xdr_buf_t* args,
    uint8_t** resp,
    size_t* resp_len
) {
    return rpc_call_bytes(
        TEST_HOST,
        NFS_PORT,
        NFS_PROGRAM,
        NFS_V3,
        proc,
        xdr_buf_data(args),
        xdr_buf_size(args),
        resp,
        resp_len
    );
}

static int remove_file_by_name(const nfs_fh3_t* root_fh, const char* filename) {
    xdr_buf_t args;
    xdr_buf_init(&args);

    REMOVE3args_t remove_args;
    remove3args_init(&remove_args);
    nfs_fh3_set(&remove_args.object_dir, root_fh->data, root_fh->len);
    remove3args_set_name(&remove_args, filename);
    xdr_pack_REMOVE3args(&args, &remove_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs_call_xdr(NFSPROC3_REMOVE, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        REMOVE3res_t remove_res;
        memset(&remove_res, 0, sizeof(remove_res));
        xdr_unpack_REMOVE3res(&body, &remove_res);
        nfs_status = (int)remove_res.status;
        xdr_buf_destroy(&body);
    }

    free(resp);
    remove3args_destroy(&remove_args);
    xdr_buf_destroy(&args);
    return status == NFSTEST_RPC_OK ? nfs_status : status;
}

static void test_nfs_null_rpc_allows_empty_args(void) {
    uint8_t* resp_data = NULL;
    size_t resp_len = 0;
    int status = rpc_call_bytes(
        TEST_HOST,
        NFS_PORT,
        NFS_PROGRAM,
        NFS_V3,
        NFSPROC3_NULL,
        NULL,
        0,
        &resp_data,
        &resp_len
    );

    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "NFS NULL RPC should accept empty args");
    ASSERT_TRUE(resp_data == NULL, "empty RPC response should not allocate data");
    ASSERT_EQ_U32((uint32_t)resp_len, 0, "empty RPC response length");

    free(resp_data);
    TEST_PASS();
}

static void test_mount_returns_root_file_handle(void) {
    nfs_fh3_t root_fh;
    nfs_fh3_init(&root_fh);

    mount_root(&root_fh);

    ASSERT_TRUE(root_fh.len > 0, "root file handle length");
    ASSERT_TRUE(root_fh.len <= NFS3_FHSIZE, "root file handle max length");

    nfs_fh3_destroy(&root_fh);
    TEST_PASS();
}

static void test_getattr_access_fsinfo_fsstat_on_root(void) {
    nfs_fh3_t root_fh;
    nfs_fh3_init(&root_fh);
    mount_root(&root_fh);

    xdr_buf_t args;
    xdr_buf_init(&args);

    GETATTR3args_t getattr_args;
    nfs_fh3_init(&getattr_args.object);
    nfs_fh3_set(&getattr_args.object, root_fh.data, root_fh.len);
    xdr_pack_GETATTR3args(&args, &getattr_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs_call_xdr(NFSPROC3_GETATTR, &args, &resp, &resp_len);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "GETATTR RPC should succeed");
    xdr_buf_t body;
    xdr_buf_init_copy(&body, resp, resp_len);
    GETATTR3res_t getattr_res;
    memset(&getattr_res, 0, sizeof(getattr_res));
    xdr_unpack_GETATTR3res(&body, &getattr_res);
    ASSERT_EQ_U32((uint32_t)getattr_res.status, (uint32_t)NFS3_OK, "GETATTR NFS status");
    ASSERT_EQ_U32((uint32_t)getattr_res.resok.obj_attributes.type, (uint32_t)NF3DIR, "root type should be directory");
    free(resp);
    xdr_buf_destroy(&body);
    xdr_buf_clear(&args);

    ACCESS3args_t access_args;
    memset(&access_args, 0, sizeof(access_args));
    nfs_fh3_init(&access_args.object);
    nfs_fh3_set(&access_args.object, root_fh.data, root_fh.len);
    access_args.access = NFS3_ACCESS_READ | NFS3_ACCESS_LOOKUP;
    xdr_pack_ACCESS3args(&args, &access_args);
    resp = NULL;
    resp_len = 0;
    status = nfs_call_xdr(NFSPROC3_ACCESS, &args, &resp, &resp_len);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "ACCESS RPC should succeed");
    xdr_buf_init_copy(&body, resp, resp_len);
    ACCESS3res_t access_res;
    memset(&access_res, 0, sizeof(access_res));
    xdr_unpack_ACCESS3res(&body, &access_res);
    ASSERT_EQ_U32((uint32_t)access_res.status, (uint32_t)NFS3_OK, "ACCESS NFS status");
    ASSERT_TRUE((access_res.resok.access & NFS3_ACCESS_READ) != 0, "root should be readable");
    free(resp);
    xdr_buf_destroy(&body);
    xdr_buf_clear(&args);

    FSSTAT3args_t fsstat_args;
    memset(&fsstat_args, 0, sizeof(fsstat_args));
    nfs_fh3_init(&fsstat_args.fsroot);
    nfs_fh3_set(&fsstat_args.fsroot, root_fh.data, root_fh.len);
    xdr_pack_FSSTAT3args(&args, &fsstat_args);
    resp = NULL;
    resp_len = 0;
    status = nfs_call_xdr(NFSPROC3_FSSTAT, &args, &resp, &resp_len);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "FSSTAT RPC should succeed");
    xdr_buf_init_copy(&body, resp, resp_len);
    FSSTAT3res_t fsstat_res;
    memset(&fsstat_res, 0, sizeof(fsstat_res));
    xdr_unpack_FSSTAT3res(&body, &fsstat_res);
    ASSERT_EQ_U32((uint32_t)fsstat_res.status, (uint32_t)NFS3_OK, "FSSTAT NFS status");
    ASSERT_TRUE(fsstat_res.resok.tbytes > 0, "filesystem total bytes");
    free(resp);
    xdr_buf_destroy(&body);
    xdr_buf_clear(&args);

    FSINFO3args_t fsinfo_args;
    memset(&fsinfo_args, 0, sizeof(fsinfo_args));
    nfs_fh3_init(&fsinfo_args.fsroot);
    nfs_fh3_set(&fsinfo_args.fsroot, root_fh.data, root_fh.len);
    xdr_pack_FSINFO3args(&args, &fsinfo_args);
    resp = NULL;
    resp_len = 0;
    status = nfs_call_xdr(NFSPROC3_FSINFO, &args, &resp, &resp_len);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "FSINFO RPC should succeed");
    xdr_buf_init_copy(&body, resp, resp_len);
    FSINFO3res_t fsinfo_res;
    memset(&fsinfo_res, 0, sizeof(fsinfo_res));
    xdr_unpack_FSINFO3res(&body, &fsinfo_res);
    ASSERT_EQ_U32((uint32_t)fsinfo_res.status, (uint32_t)NFS3_OK, "FSINFO NFS status");
    ASSERT_TRUE(fsinfo_res.resok.rtmax > 0, "rtmax should be positive");
    ASSERT_TRUE(fsinfo_res.resok.wtmax > 0, "wtmax should be positive");

    free(resp);
    xdr_buf_destroy(&body);
    nfs_fh3_destroy(&fsinfo_args.fsroot);
    nfs_fh3_destroy(&fsstat_args.fsroot);
    nfs_fh3_destroy(&access_args.object);
    nfs_fh3_destroy(&getattr_args.object);
    xdr_buf_destroy(&args);
    nfs_fh3_destroy(&root_fh);
    TEST_PASS();
}

static void test_create_write_read_remove_file(void) {
    nfs_fh3_t root_fh;
    nfs_fh3_init(&root_fh);
    mount_root(&root_fh);

    char filename[128];
    snprintf(filename, sizeof(filename), "c_real_rpc_%ld_%ld.txt", (long)getpid(), (long)time(NULL));
    const uint8_t payload[] = {'H', 'e', 'l', 'l', 'o', ' ', 'R', 'P', 'C'};
    int created = 0;
    int failed = 0;

    xdr_buf_t args;
    xdr_buf_init(&args);
    xdr_buf_t body;
    xdr_buf_init(&body);
    uint8_t* resp = NULL;
    size_t resp_len = 0;

    CREATE3args_t create_args;
    create3args_init(&create_args);
    CREATE3res_t create_res;
    create3res_init(&create_res);
    WRITE3args_t write_args;
    write3args_init(&write_args);
    READ3args_t read_args;
    memset(&read_args, 0, sizeof(read_args));
    nfs_fh3_init(&read_args.file);
    READ3res_t read_res;
    read3res_init(&read_res);

    nfs_fh3_set(&create_args.where_dir, root_fh.data, root_fh.len);
    create3args_set_name(&create_args, filename);
    create_args.how_mode = UNCHECKED;
    create_args.how_attributes.mode_set = 1;
    create_args.how_attributes.mode = 0644;
    xdr_pack_CREATE3args(&args, &create_args);

    int status = nfs_call_xdr(NFSPROC3_CREATE, &args, &resp, &resp_len);
    if (status != NFSTEST_RPC_OK) {
        printf("\n  FAIL  %s: CREATE RPC should succeed - expected %d, got %d (line %d)\n",
               current_test_name, NFSTEST_RPC_OK, status, __LINE__);
        tests_failed++;
        failed = 1;
        goto cleanup;
    }
    xdr_buf_init_copy(&body, resp, resp_len);
    xdr_unpack_CREATE3res(&body, &create_res);
    free(resp);
    resp = NULL;
    xdr_buf_destroy(&body);
    xdr_buf_init(&body);
    xdr_buf_clear(&args);

    if (create_res.status != NFS3_OK || create_res.resok.object.len == 0) {
        printf("\n  FAIL  %s: CREATE should return OK and file handle (line %d)\n", current_test_name, __LINE__);
        tests_failed++;
        failed = 1;
        goto cleanup;
    }
    created = 1;

    nfs_fh3_set(&write_args.file, create_res.resok.object.data, create_res.resok.object.len);
    write_args.offset = 0;
    write_args.count = (uint32_t)sizeof(payload);
    write_args.stable = DATA_SYNC;
    write_args.data = (uint8_t*)malloc(sizeof(payload));
    memcpy(write_args.data, payload, sizeof(payload));
    write_args.data_len = (uint32_t)sizeof(payload);
    xdr_pack_WRITE3args(&args, &write_args);
    status = nfs_call_xdr(NFSPROC3_WRITE, &args, &resp, &resp_len);
    if (status != NFSTEST_RPC_OK) {
        printf("\n  FAIL  %s: WRITE RPC should succeed - expected %d, got %d (line %d)\n",
               current_test_name, NFSTEST_RPC_OK, status, __LINE__);
        tests_failed++;
        failed = 1;
        goto cleanup;
    }
    xdr_buf_init_copy(&body, resp, resp_len);
    WRITE3res_t write_res;
    memset(&write_res, 0, sizeof(write_res));
    xdr_unpack_WRITE3res(&body, &write_res);
    free(resp);
    resp = NULL;
    xdr_buf_destroy(&body);
    xdr_buf_init(&body);
    xdr_buf_clear(&args);
    if (write_res.status != NFS3_OK || write_res.resok.count != (uint32_t)sizeof(payload)) {
        printf("\n  FAIL  %s: WRITE should return OK and full count (line %d)\n", current_test_name, __LINE__);
        tests_failed++;
        failed = 1;
        goto cleanup;
    }

    nfs_fh3_set(&read_args.file, create_res.resok.object.data, create_res.resok.object.len);
    read_args.offset = 0;
    read_args.count = 1024;
    xdr_pack_READ3args(&args, &read_args);
    status = nfs_call_xdr(NFSPROC3_READ, &args, &resp, &resp_len);
    if (status != NFSTEST_RPC_OK) {
        printf("\n  FAIL  %s: READ RPC should succeed - expected %d, got %d (line %d)\n",
               current_test_name, NFSTEST_RPC_OK, status, __LINE__);
        tests_failed++;
        failed = 1;
        goto cleanup;
    }
    xdr_buf_init_copy(&body, resp, resp_len);
    xdr_unpack_READ3res(&body, &read_res);
    free(resp);
    resp = NULL;
    xdr_buf_destroy(&body);
    xdr_buf_init(&body);
    xdr_buf_clear(&args);
    if (read_res.status != NFS3_OK || read_res.resok.data_len != (uint32_t)sizeof(payload) ||
        memcmp(read_res.resok.data, payload, sizeof(payload)) != 0) {
        printf("\n  FAIL  %s: READ should return written data (line %d)\n", current_test_name, __LINE__);
        tests_failed++;
        failed = 1;
        goto cleanup;
    }

    status = remove_file_by_name(&root_fh, filename);
    if (status != NFS3_OK) {
        printf("\n  FAIL  %s: REMOVE should return OK - expected %d, got %d (line %d)\n",
               current_test_name, NFS3_OK, status, __LINE__);
        tests_failed++;
        failed = 1;
        goto cleanup;
    }
    created = 0;

cleanup:
    if (created) {
        remove_file_by_name(&root_fh, filename);
    }
    free(resp);
    xdr_buf_destroy(&body);
    read3res_destroy(&read_res);
    nfs_fh3_destroy(&read_args.file);
    write3args_destroy(&write_args);
    create3res_destroy(&create_res);
    create3args_destroy(&create_args);
    xdr_buf_destroy(&args);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

int main(void) {
    printf("=== Real RPC Integration Tests (C) ===\n\n");
    RUN_TEST(test_nfs_null_rpc_allows_empty_args);
    RUN_TEST(test_mount_returns_root_file_handle);
    RUN_TEST(test_getattr_access_fsinfo_fsstat_on_root);
    RUN_TEST(test_create_write_read_remove_file);
    PRINT_SUMMARY();
    return tests_failed;
}
