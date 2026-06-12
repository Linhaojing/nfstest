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

int nfs3_create_test_file(const nfs_fh3_t* dir, const char* name, nfs_fh3_t* out_fh) {
    xdr_buf_t args;
    xdr_buf_init(&args);

    CREATE3args_t create_args;
    create3args_init(&create_args);
    nfs_fh3_set(&create_args.where_dir, dir->data, dir->len);
    create3args_set_name(&create_args, name);
    create_args.how_mode = UNCHECKED;
    create_args.how_attributes.mode_set = 1;
    create_args.how_attributes.mode = 0600;
    xdr_pack_CREATE3args(&args, &create_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_CREATE, &args, &resp, &resp_len);
    nfsstat3_t nfs_status = NFS3ERR_IO;
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        CREATE3res_t create_res;
        create3res_init(&create_res);
        xdr_unpack_CREATE3res(&body, &create_res);
        nfs_status = create_res.status;
        if (nfs_status == NFS3_OK && out_fh) {
            nfs_fh3_set(out_fh, create_res.resok.object.data, create_res.resok.object.len);
        }
        create3res_destroy(&create_res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    create3args_destroy(&create_args);
    xdr_buf_destroy(&args);
    if (status != NFSTEST_RPC_OK) {
        return status;
    }
    return resp == NULL ? NFSTEST_RPC_PROTO_ERR : (int)nfs_status;
}

int nfs3_create_test_dir(const nfs_fh3_t* dir, const char* name, nfs_fh3_t* out_fh) {
    xdr_buf_t args;
    xdr_buf_init(&args);

    MKDIR3args_t mkdir_args;
    mkdir3args_init(&mkdir_args);
    nfs_fh3_set(&mkdir_args.where_dir, dir->data, dir->len);
    mkdir3args_set_name(&mkdir_args, name);
    mkdir_args.attributes.mode_set = 1;
    mkdir_args.attributes.mode = 0700;
    xdr_pack_MKDIR3args(&args, &mkdir_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_MKDIR, &args, &resp, &resp_len);
    nfsstat3_t nfs_status = NFS3ERR_IO;
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        MKDIR3res_t mkdir_res;
        mkdir3res_init(&mkdir_res);
        xdr_unpack_MKDIR3res(&body, &mkdir_res);
        nfs_status = mkdir_res.status;
        if (nfs_status == NFS3_OK && out_fh) {
            nfs_fh3_set(out_fh, mkdir_res.resok.object.data, mkdir_res.resok.object.len);
        }
        mkdir3res_destroy(&mkdir_res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    mkdir3args_destroy(&mkdir_args);
    xdr_buf_destroy(&args);
    if (status != NFSTEST_RPC_OK) {
        return status;
    }
    return resp == NULL ? NFSTEST_RPC_PROTO_ERR : (int)nfs_status;
}

int nfs3_lookup_name(const nfs_fh3_t* dir, const char* name, nfs_fh3_t* out_fh) {
    xdr_buf_t args;
    xdr_buf_init(&args);

    LOOKUP3args_t lookup_args;
    lookup3args_init(&lookup_args);
    nfs_fh3_set(&lookup_args.what_dir, dir->data, dir->len);
    lookup3args_set_name(&lookup_args, name);
    xdr_pack_LOOKUP3args(&args, &lookup_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_LOOKUP, &args, &resp, &resp_len);
    nfsstat3_t nfs_status = NFS3ERR_IO;
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        LOOKUP3res_t lookup_res;
        lookup3res_init(&lookup_res);
        xdr_unpack_LOOKUP3res(&body, &lookup_res);
        nfs_status = lookup_res.status;
        if (nfs_status == NFS3_OK && out_fh) {
            nfs_fh3_set(out_fh, lookup_res.resok.object.data, lookup_res.resok.object.len);
        }
        lookup3res_destroy(&lookup_res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    lookup3args_destroy(&lookup_args);
    xdr_buf_destroy(&args);
    if (status != NFSTEST_RPC_OK) {
        return status;
    }
    return resp == NULL ? NFSTEST_RPC_PROTO_ERR : (int)nfs_status;
}

int nfs3_remove_test_file(const nfs_fh3_t* dir, const char* name) {
    xdr_buf_t args;
    xdr_buf_init(&args);

    REMOVE3args_t remove_args;
    remove3args_init(&remove_args);
    nfs_fh3_set(&remove_args.object_dir, dir->data, dir->len);
    remove3args_set_name(&remove_args, name);
    xdr_pack_REMOVE3args(&args, &remove_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_REMOVE, &args, &resp, &resp_len);
    nfsstat3_t nfs_status = NFS3ERR_IO;
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        REMOVE3res_t remove_res;
        memset(&remove_res, 0, sizeof(remove_res));
        xdr_unpack_REMOVE3res(&body, &remove_res);
        nfs_status = remove_res.status;
        xdr_buf_destroy(&body);
    }

    free(resp);
    remove3args_destroy(&remove_args);
    xdr_buf_destroy(&args);
    if (status != NFSTEST_RPC_OK) {
        return status;
    }
    if (resp == NULL) {
        return NFSTEST_RPC_PROTO_ERR;
    }
    return nfs_status == NFS3ERR_NOENT ? NFS3_OK : (int)nfs_status;
}

int nfs3_rmdir_test_dir(const nfs_fh3_t* dir, const char* name) {
    xdr_buf_t args;
    xdr_buf_init(&args);

    RMDIR3args_t rmdir_args;
    rmdir3args_init(&rmdir_args);
    nfs_fh3_set(&rmdir_args.object_dir, dir->data, dir->len);
    rmdir3args_set_name(&rmdir_args, name);
    xdr_pack_RMDIR3args(&args, &rmdir_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_RMDIR, &args, &resp, &resp_len);
    nfsstat3_t nfs_status = NFS3ERR_IO;
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        RMDIR3res_t rmdir_res;
        memset(&rmdir_res, 0, sizeof(rmdir_res));
        xdr_unpack_RMDIR3res(&body, &rmdir_res);
        nfs_status = rmdir_res.status;
        xdr_buf_destroy(&body);
    }

    free(resp);
    rmdir3args_destroy(&rmdir_args);
    xdr_buf_destroy(&args);
    if (status != NFSTEST_RPC_OK) {
        return status;
    }
    if (resp == NULL) {
        return NFSTEST_RPC_PROTO_ERR;
    }
    return nfs_status == NFS3ERR_NOENT ? NFS3_OK : (int)nfs_status;
}

int nfs3_write_data(const nfs_fh3_t* file, uint64_t offset, stable_how_t stable, const uint8_t* data, size_t len) {
    if (len > UINT32_MAX) {
        return NFS3ERR_INVAL;
    }

    xdr_buf_t args;
    xdr_buf_init(&args);

    WRITE3args_t write_args;
    write3args_init(&write_args);
    nfs_fh3_set(&write_args.file, file->data, file->len);
    write_args.offset = offset;
    write_args.count = (uint32_t)len;
    write_args.stable = stable;
    write_args.data = len > 0 ? (uint8_t*)malloc(len) : NULL;
    if (len > 0) {
        memcpy(write_args.data, data, len);
    }
    write_args.data_len = (uint32_t)len;
    xdr_pack_WRITE3args(&args, &write_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_WRITE, &args, &resp, &resp_len);
    nfsstat3_t nfs_status = NFS3ERR_IO;
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        WRITE3res_t write_res;
        memset(&write_res, 0, sizeof(write_res));
        xdr_unpack_WRITE3res(&body, &write_res);
        nfs_status = write_res.status;
        xdr_buf_destroy(&body);
    }

    free(resp);
    write3args_destroy(&write_args);
    xdr_buf_destroy(&args);
    if (status != NFSTEST_RPC_OK) {
        return status;
    }
    return resp == NULL ? NFSTEST_RPC_PROTO_ERR : (int)nfs_status;
}

int nfs3_read_data(const nfs_fh3_t* file, uint64_t offset, uint32_t count, uint8_t** data, size_t* len, int* eof) {
    if (data) {
        *data = NULL;
    }
    if (len) {
        *len = 0;
    }
    if (eof) {
        *eof = 0;
    }

    xdr_buf_t args;
    xdr_buf_init(&args);

    READ3args_t read_args;
    memset(&read_args, 0, sizeof(read_args));
    nfs_fh3_init(&read_args.file);
    nfs_fh3_set(&read_args.file, file->data, file->len);
    read_args.offset = offset;
    read_args.count = count;
    xdr_pack_READ3args(&args, &read_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_READ, &args, &resp, &resp_len);
    nfsstat3_t nfs_status = NFS3ERR_IO;
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        READ3res_t read_res;
        read3res_init(&read_res);
        xdr_unpack_READ3res(&body, &read_res);
        nfs_status = read_res.status;
        if (nfs_status == NFS3_OK) {
            if (data) {
                *data = read_res.resok.data;
                read_res.resok.data = NULL;
            }
            if (len) {
                *len = read_res.resok.data_len;
            }
            if (eof) {
                *eof = read_res.resok.eof;
            }
        }
        read3res_destroy(&read_res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&read_args.file);
    xdr_buf_destroy(&args);
    if (status != NFSTEST_RPC_OK) {
        return status;
    }
    return resp == NULL ? NFSTEST_RPC_PROTO_ERR : (int)nfs_status;
}

int nfs3_status_is_allowed(nfsstat3_t status, const nfsstat3_t* allowed, size_t allowed_count) {
    for (size_t i = 0; i < allowed_count; ++i) {
        if (status == allowed[i]) {
            return 1;
        }
    }
    return 0;
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
