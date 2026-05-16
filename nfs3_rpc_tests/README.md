# NFSv3 RPC Test Suite

A comprehensive C++17 test suite for **NFSv3 protocol compliance testing** at the RPC level.

## ✨ Features

- **Direct RPC Testing**: Constructs and validates raw RPC packets (bypasses kernel NFS client)
- **Hand-written XDR Codec**: Zero dependency on `rpcgen` - fully type-safe C++ implementation
- **Complete Coverage**: All 22 NFSv3 procedures (RFC 1813) with normal, error, and boundary tests
- **Minimal Dependencies**: Only requires `libtirpc` for runtime
- **Modern C++17**: Uses `std::expected`, `std::optional`, RAII, templates
- **Google Test Integration**: Rich assertions, fixtures, CI-friendly output

## 📋 Architecture

```
┌─────────────────────────────────────────┐
│           Test Cases Layer              │
│  test_rpc_null.cpp  test_nfs3_readwrite │
│  ... (10 test files, 63 test cases)     │
├─────────────────────────────────────────┤
│          Test Framework Layer           │
│  ┌──────────────┐ ┌──────────────────┐  │
│  │ TestContext   │ │ NFS3TestClient    │  │
│  │ (GTest Fixture)│ │ (22 typed APIs)  │  │
│  └──────┬───────┘ └────────┬─────────┘  │
│         │                  │             │
│  ┌──────▼──────────────────▼─────────┐  │
│  │         RPCEndpoint               │  │
│  │    (libtirpc TCP connection)      │  │
│  └──────────────────┬────────────────┘  │
├─────────────────────┼───────────────────┤
│      Protocol Layer (Hand-written XDR)  │
│  ┌────────────┐  ┌───────────────────┐  │
│  │ xdr_codec  │  │ nfs3_types        │  │
│  │ (template) │  │ (RFC 1813 structs)│  │
│  └────────────┘  └───────────────────┘  │
├─────────────────────────────────────────┤
│         Transport: TCP via libtirpc      │
└─────────────────────────────────────────┘
```

## 🔧 Dependencies

### Required (Runtime)
| Package | Version | Purpose |
|---------|---------|---------|
| `g++` | ≥ 7.0 | C++17 compiler |
| `cmake` | ≥ 2.8 | Build system |
| `libtirpc-dev` | any | RPC/XDR runtime |

### Optional (Testing)
| Package | Purpose |
|---------|---------|
| `libgtest-dev` | Google Test framework |

**Total: 3 required + 1 optional packages**

## 🚀 Quick Start

### Installation (Ubuntu/Debian)

```bash
# Required dependencies
sudo apt update && sudo apt install -y \
    g++ cmake pkg-config libtirpc-dev

# Optional: Google Test for running tests
sudo apt install -y libgtest-dev
```

### Building

```bash
# Clone and configure
cd nfs3_rpc_tests
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build core library (works without GTest)
cmake --build . --target nfs3_test_core

# Build everything including tests
cmake --build .
```

### Running Tests

```bash
# Using convenience script (recommended)
./scripts/run_tests.sh --server <NFS_SERVER_IP>

# Examples:
./scripts/run_tests.sh --server 192.168.1.100 --verbose
./scripts/run_tests.sh --server localhost --filter "RpcNull*"
./scripts/run_tests.sh --list-tests

# Or manually
cd build
./tests/test_nfs3_suite --server=192.168.1.100 --port=2049
```

## 📁 Project Structure

```
nfs3_rpc_tests/
├── CMakeLists.txt                 # Top-level CMake config
├── cmake/
│   └── FindTirpc.cmake            # libtirpc finder module
├── include/nfs3/
│   ├── xdr_codec.hpp              # XDR buffer template class
│   ├── rpc_endpoint.hpp           # RPC connection manager (PIMPL)
│   ├── nfs3_client.hpp            # 22 NFSv3 procedure APIs
│   ├── test_context.hpp           # GTest fixture with lifecycle
│   ├── nfs3_types.hpp             # All RFC 1813 data structures
│   ├── nfs3_constants.hpp         # Enums, constants, error codes
│   └── detail/
│       └── xdr_primitive.hpp      # Byte-order conversion helpers
├── src/
│   ├── xdr_codec.cpp              # XDR codec implementation
│   ├── rpc_endpoint.cpp           # RPCEndpoint (libtirpc wrapper)
│   ├── nfs3_client.cpp            # NFS3TestClient implementation
│   ├── test_context.cpp           # TestContext implementation
│   └── detail/
│       └── xdr_primitive.cpp      # Byte-order helpers
├── tests/
│   ├── CMakeLists.txt
│   ├── test_main.cpp              # GTest entry point
│   ├── test_rpc_null.cpp          # NULL procedure tests
│   ├── test_rpc_errors.cpp        # RPC error handling tests
│   ├── test_nfs3_getattr.cpp      # GETATTR/SETATTR tests
│   ├── test_nfs3_lookup.cpp       # LOOKUP tests
│   ├── test_nfs3_readwrite.cpp    # READ/WRITE tests
│   ├── test_nfs3_create.cpp       # CREATE/MKDIR tests
│   ├── test_nfs3_remove.cpp       # REMOVE/RMDIR tests
│   ├── test_nfs3_readdir.cpp      # READDIR/READDIRPLUS tests
│   ├── test_nfs3_other.cpp        # ACCESS/LINK/RENAME/etc.
│   └── test_nfs3_stress.cpp       # Boundary & stress tests
├── scripts/
│   └── run_tests.sh               # Test runner script
└── README.md                      # This file
```

## 🎯 Test Coverage

### RPC Layer Tests
| Test File | Tests | Coverage |
|-----------|-------|----------|
| `test_rpc_null` | 4 | NULL procedure, sequential/concurrent requests |
| `test_rpc_errors` | 5 | Invalid version, program, procedure errors |

### NFSv3 Operation Tests
| Test File | Tests | Procedures |
|-----------|-------|-----------|
| `test_nfs3_getattr` | 5 | GETATTR (1), SETATTR (2) |
| `test_nfs3_lookup` | 6 | LOOKUP (3) |
| `test_nfs3_readwrite` | 8 | READ (6), WRITE (7) |
| `test_nfs3_create` | 7 | CREATE (8), MKDIR (9) |
| `test_nfs3_remove` | 5 | REMOVE (12), RMDIR (13) |
| `test_nfs3_readdir` | 6 | READDIR (16), READDIRPLUS (17) |
| `test_nfs3_other` | 11 | ACCESS, SYMLINK, MKNOD, RENAME, FSSTAT, etc. |

### Stress & Edge Cases
| Test File | Tests | Focus |
|-----------|-------|-------|
| `test_nfs3_stress` | 6 | Filename boundaries, concurrency, idempotency |

**Total: 10 test suites, 63 test cases covering all 22 NFSv3 procedures**

## 💡 Design Decisions

### Why Hand-written XDR instead of rpcgen?

| Aspect | rpcgen | Hand-written C++ |
|--------|--------|------------------|
| **Dependencies** | Requires rpcgen tool + .x files | ✅ Zero extra deps |
| **Type Safety** | C-style void* | ✅ Strong typing |
| **Memory Management** | Manual malloc/free | ✅ RAII automatic |
| **Debugging** | Macro-expanded code | ✅ Template debugging |
| **Maintainability** | Dual source (.x + generated) | ✅ Single source truth |

For a client-only project with fixed protocol version, hand-written XDR provides better long-term maintainability.

### Why libtirpc?

- Standard RPC library on modern Linux
- Provides socket-level API (`clnt_create`, `clnt_call`)
- Handles authentication, fragmentation, retransmission
- Only external runtime dependency required

## 🔬 API Example

```cpp
#include "nfs3/nfs3_client.hpp"

int main() {
    auto endpoint = nfs3::RPCEndpoint::create("192.168.1.100", 2049);
    
    if (!endpoint.is_connected()) {
        return 1;
    }
    
    nfs3::NFS3TestClient client(std::move(endpoint));
    
    // NULL procedure test
    auto result = client.null();
    if (!result) { /* handle error */ }
    
    // GETATTR on root
    nfs3::nfs_fh3 root; // zero-length or MOUNT-acquired
    auto attr_result = client.getattr(root);
    if (attr_result) {
        auto& attrs = attr_result->obj_attributes;
        printf("Root type: %u\n", static_cast<uint32_t>(attrs.type_));
    }
    
    return 0;
}
```

## ⚙️ Configuration Options

### CMake Options
| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | ON | Enable test compilation |
| `CMAKE_BUILD_TYPE` | Debug | Debug/Release mode |

### Runtime Options
| Option | Flag | Description |
|--------|------|-------------|
| Server address | `--server=<host>` | Required (or set `NFS_TEST_SERVER`) |
| Port | `--port=<port>` | Default 2049 (or set `NFS_TEST_PORT`) |
| Verbose | `--verbose` | Detailed output |
| Filter | `--filter=<pattern>` | GTest filter pattern |

## 🧪 Testing Without Real NFS Server

The framework is designed for integration testing against real NFSv3 servers. However:

- **XDR Unit Tests**: Can test encoding/decoding in isolation
- **Mock Testing**: Can mock `RPCEndpoint` for unit testing logic
- **Stubs**: Current stubs return errors when no server available

To run full tests, you need access to an NFSv3 server (Linux NFS kernel server, user-space server, etc.)

## 📊 Quality Metrics (Target)

| Metric | Target | Verification |
|--------|--------|--------------|
| Build time | < 30 sec | Single-threaded debug |
| Test execution | < 5 min | LAN environment |
| Code coverage | > 80% | Key paths 100% |
| Memory leaks | None | valgrind clean |
| Warnings | Zero | `-Wall -Wextra -Wpedantic` |

## 🤝 Contributing

1. Follow existing code style (C++17)
2. Add tests for new features
3. Update this README for API changes
4. Ensure zero compiler warnings

## 📄 License

This project is provided as-is for educational and testing purposes.

---

**Note**: This test suite targets RFC 1813 (NFS version 3 protocol specification). For production use, consider established tools like `cthon-nfs-tests` or `nfstest`.
