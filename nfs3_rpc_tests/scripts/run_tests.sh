#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build"

SERVER=""
PORT="2049"
EXPORT=""
VERBOSE=0
TEST_FILTER=""
LIST_ONLY=0

usage() {
    cat << EOF
NFSv3 RPC Test Suite Runner

Usage: $(basename "$0") [OPTIONS]

Options:
    --server <host>       NFS server hostname or IP
    --port <port>         NFS server port (default: 2049)
    --export <path>       NFS export path
    --verbose             Enable verbose output
    --filter <pattern>    GTest filter pattern (e.g., "RpcNull*")
    --list-tests          List all available tests without running
    -h, --help            Show this help message

Examples:
    # Run all tests against server 192.168.1.100 export /srv/nfs
    $(basename "$0") --server 192.168.1.100 --export /srv/nfs

    # Run only NULL procedure tests with verbose output
    $(basename "$0") --server localhost --export /srv/nfs --filter "RpcNull*" --verbose

    # List available tests
    $(basename "$0") --list-tests

Environment Variables:
    NFS_TEST_SERVER   Default NFS server (can be overridden by --server)
    NFS_TEST_PORT     Default NFS port (can be overridden by --port)
    NFS_TEST_EXPORT   Default NFS export path (can be overridden by --export)

Exit Codes:
    0   All tests passed
    1   One or more tests failed
    2   Build or configuration error

EOF
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --server)
                SERVER="$2"
                shift 2
                ;;
            --port)
                PORT="$2"
                shift 2
                ;;
            --export)
                EXPORT="$2"
                shift 2
                ;;
            --verbose)
                VERBOSE=1
                shift
                ;;
            --filter)
                TEST_FILTER="$2"
                shift 2
                ;;
            --list-tests)
                LIST_ONLY=1
                shift
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                echo "Error: Unknown option: $1" >&2
                usage
                exit 2
                ;;
        esac
    done
    
    if [[ -z "$SERVER" ]] && [[ $LIST_ONLY -eq 0 ]]; then
        SERVER="${NFS_TEST_SERVER:-}"
    fi
    
    if [[ -z "$PORT" ]]; then
        PORT="${NFS_TEST_PORT:-2049}"
    fi

    if [[ -z "$EXPORT" ]] && [[ $LIST_ONLY -eq 0 ]]; then
        EXPORT="${NFS_TEST_EXPORT:-}"
    fi
}

build_project() {
    echo "=== Building NFSv3 RPC Test Suite ==="
    
    if [[ ! -d "$BUILD_DIR" ]]; then
        mkdir -p "$BUILD_DIR"
    fi
    
    cd "$BUILD_DIR"
    cmake .. -DCMAKE_BUILD_TYPE=Debug 2>&1 | tail -5
    
    if [[ $? -ne 0 ]]; then
        echo "ERROR: CMake configuration failed" >&2
        exit 2
    fi
    
    cmake --build . --parallel$(nproc 2>/dev/null || echo 4) 2>&1 | tail -10
    
    if [[ $? -ne 0 ]]; then
        echo "ERROR: Build failed" >&2
        exit 2
    fi
    
    echo "✓ Build successful"
}

run_tests() {
    local TEST_BINARY="$BUILD_DIR/tests/test_nfs3_suite"
    
    if [[ ! -x "$TEST_BINARY" ]]; then
        echo "ERROR: Test binary not found: $TEST_BINARY" >&2
        exit 2
    fi
    
    local ARGS=(
        "--gtest_color=yes"
    )
    
    if [[ -n "$SERVER" ]]; then
        ARGS+=("--server" "$SERVER")
    fi
    
    ARGS+=("--port" "$PORT")

    if [[ -n "$EXPORT" ]]; then
        ARGS+=("--export" "$EXPORT")
    fi
    
    if [[ $VERBOSE -eq 1 ]]; then
        ARGS+=("--verbose")
        ARGS+=("--gtest_print_time=1")
    fi
    
    if [[ -n "$TEST_FILTER" ]]; then
        ARGS+=("--gtest_filter=$TEST_FILTER")
    fi
    
    echo ""
    echo "========================================="
    echo "  NFSv3 RPC Test Suite"
    echo "========================================="
    echo "Server: ${SERVER:-<not set>}"
    echo "Port:   $PORT"
    echo "Export: ${EXPORT:-<not set>}"
    echo "Binary: $TEST_BINARY"
    if [[ -n "$TEST_FILTER" ]]; then
        echo "Filter: $TEST_FILTER"
    fi
    echo "========================================="
    echo ""
    
    if [[ $LIST_ONLY -eq 1 ]]; then
        "$TEST_BINARY" "${ARGS[@]}" --gtest_list_tests
    else
        exec "$TEST_BINARY" "${ARGS[@]}"
    fi
}

main() {
    parse_args "$@"
    
    build_project
    run_tests
}

main "$@"
