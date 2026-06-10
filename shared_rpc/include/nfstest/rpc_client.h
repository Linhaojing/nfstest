#ifndef NFSTEST_RPC_CLIENT_H
#define NFSTEST_RPC_CLIENT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nfstest_rpc_client nfstest_rpc_client_t;

#define NFSTEST_RPC_OK             0
#define NFSTEST_RPC_CONN_ERR      -1
#define NFSTEST_RPC_TIMEOUT       -2
#define NFSTEST_RPC_PROG_UNAVAIL  -3
#define NFSTEST_RPC_PROG_MISMATCH -4
#define NFSTEST_RPC_PROC_UNAVAIL  -5
#define NFSTEST_RPC_GARBAGE_ARGS  -6
#define NFSTEST_RPC_AUTH_ERROR    -7
#define NFSTEST_RPC_PROTO_ERR     -8
#define NFSTEST_RPC_SEND_ERR      -9
#define NFSTEST_RPC_RECV_ERR     -10

nfstest_rpc_client_t* nfstest_rpc_connect(const char* host, uint16_t port, int timeout_ms);

void nfstest_rpc_disconnect(nfstest_rpc_client_t* client);

int nfstest_rpc_call(nfstest_rpc_client_t* client,
                     uint32_t prog, uint32_t vers, uint32_t proc,
                     const uint8_t* args_data, size_t args_len,
                     uint8_t** resp_data, size_t* resp_len,
                     int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
