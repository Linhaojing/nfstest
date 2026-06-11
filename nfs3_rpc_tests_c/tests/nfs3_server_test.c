#include "nfs3_server_test.h"
#include "nfstest/rpc_client.h"
#include "nfs3_c/nfs3_constants.h"
#include "nfs3_c/nfs3_xdr.h"
#include <stdio.h>
#include <stdlib.h>
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

static const char* nfs3_server_host(void) {
    return getenv("NFS_TEST_SERVER");
}

static const char* nfs3_server_export(void) {
    return getenv("NFS_TEST_EXPORT");
}

static uint16_t nfs3_server_port(void) {
    const char* port_env = getenv("NFS_TEST_PORT");
    if (!port_env || port_env[0] == '\0') {
        return 2049;
    }

    char* end = NULL;
    unsigned long port = strtoul(port_env, &end, 10);
    if (end == port_env || *end != '\0' || port == 0 || port > 65535) {
        return 0;
    }
    return (uint16_t)port;
}

int nfs3_server_is_configured(void) {
    const char* host = nfs3_server_host();
    const char* export_path = nfs3_server_export();
    return host && host[0] != '\0' && export_path && export_path[0] != '\0' && nfs3_server_port() != 0;
}

int nfs3_server_require_server(void) {
    const char* require = getenv("NFS_TEST_REQUIRE_SERVER");
    return require && strcmp(require, "1") == 0;
}

const char* nfs3_server_skip_message(void) {
    return "set NFS_TEST_SERVER and NFS_TEST_EXPORT to run real NFS server tests";
}

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
    if (!client) {
        return NFSTEST_RPC_CONN_ERR;
    }

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
    const char* host = nfs3_server_host();
    if (!host || host[0] == '\0') {
        return 0;
    }

    xdr_buf_t args;
    xdr_buf_init(&args);
    xdr_pack_uint32(&args, prog);
    xdr_pack_uint32(&args, vers);
    xdr_pack_uint32(&args, IPPROTO_TCP_NUM);
    xdr_pack_uint32(&args, 0);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = rpc_call_bytes(
        host,
        111,
        RPCBPROG,
        RPCBVERS,
        RPCBPROC_GETPORT,
        xdr_buf_data(&args),
        xdr_buf_size(&args),
        &resp,
        &resp_len
    );
    xdr_buf_destroy(&args);
    if (status != NFSTEST_RPC_OK || resp == NULL) {
        free(resp);
        return 0;
    }

    xdr_buf_t body;
    xdr_buf_init_copy(&body, resp, resp_len);
    uint32_t port = 0;
    xdr_unpack_uint32(&body, &port);

    free(resp);
    xdr_buf_destroy(&body);

    if (port == 0 || port > 65535) {
        return 0;
    }
    return (uint16_t)port;
}

int nfs3_mount_root(nfs_fh3_t* root_fh) {
    const char* export_path = nfs3_server_export();
    uint16_t mount_port = get_rpc_service_port(MOUNT_PROGRAM, MOUNT_V3);
    if (!export_path || export_path[0] == '\0' || mount_port == 0) {
        return NFSTEST_RPC_CONN_ERR;
    }

    xdr_buf_t args;
    xdr_buf_init(&args);
    xdr_pack_cstring(&args, export_path);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = rpc_call_bytes(
        nfs3_server_host(),
        mount_port,
        MOUNT_PROGRAM,
        MOUNT_V3,
        MOUNTPROC3_MNT,
        xdr_buf_data(&args),
        xdr_buf_size(&args),
        &resp,
        &resp_len
    );
    xdr_buf_destroy(&args);
    if (status != NFSTEST_RPC_OK || resp == NULL) {
        free(resp);
        return status == NFSTEST_RPC_OK ? NFSTEST_RPC_PROTO_ERR : status;
    }

    xdr_buf_t body;
    xdr_buf_init_copy(&body, resp, resp_len);
    uint32_t mount_status = 0;
    xdr_unpack_uint32(&body, &mount_status);
    if (mount_status == 0) {
        xdr_unpack_nfs_fh3(&body, root_fh);
    }

    free(resp);
    xdr_buf_destroy(&body);

    if (mount_status != 0) {
        return (int)mount_status;
    }
    if (nfs_fh3_is_empty(root_fh)) {
        return NFSTEST_RPC_PROTO_ERR;
    }
    return NFSTEST_RPC_OK;
}

int nfs3_server_call(uint32_t proc, xdr_buf_t* args, uint8_t** resp, size_t* resp_len) {
    uint16_t port = nfs3_server_port();
    if (!nfs3_server_is_configured()) {
        return NFSTEST_RPC_CONN_ERR;
    }

    return rpc_call_bytes(
        nfs3_server_host(),
        port,
        NFS_PROGRAM,
        NFS_V3,
        proc,
        args ? xdr_buf_data(args) : NULL,
        args ? xdr_buf_size(args) : 0,
        resp,
        resp_len
    );
}

void nfs3_unique_name(const char* prefix, char* buffer, size_t buffer_len) {
    snprintf(buffer, buffer_len, "%s_%ld_%ld", prefix, (long)getpid(), (long)time(NULL));
}

int nfs3_remove_if_test_file(const nfs_fh3_t* root_fh, const char* name) {
    xdr_buf_t args;
    xdr_buf_init(&args);

    REMOVE3args_t remove_args;
    remove3args_init(&remove_args);
    nfs_fh3_set(&remove_args.object_dir, root_fh->data, root_fh->len);
    remove3args_set_name(&remove_args, name);
    xdr_pack_REMOVE3args(&args, &remove_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_REMOVE, &args, &resp, &resp_len);
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
    if (status != NFSTEST_RPC_OK) {
        return status;
    }
    return nfs_status == NFS3ERR_NOENT ? NFS3_OK : nfs_status;
}
