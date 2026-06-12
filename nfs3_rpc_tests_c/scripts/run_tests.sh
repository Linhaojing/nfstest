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
TEST_LABEL=""
LIST_ONLY=0
BUILD_ONLY=0

usage() {
    cat << EOF
NFSv3 RPC C Test Suite Runner

Usage: $(basename "$0") [OPTIONS]

Options:
    --server <host>       NFS server hostname or IP
    --port <port>         NFS server port (default: 2049)
    --export <path>       NFS export path
    --verbose             Enable verbose CTest output
    --filter <pattern>    CTest regex filter (e.g., "test_nfs3_meta_c")
    --label <label>       CTest label filter (e.g., "xdr" or "server")
    --list-tests          List available CTest tests without running
    --build-only          Configure and build without running tests
    -h, --help            Show this help message

Examples:
    # Build and run all C tests
    $(basename "$0")

    # Run all C server tests against server 192.168.1.100 export /srv/nfs
    $(basename "$0") --server 192.168.1.100 --export /srv/nfs --label server

    # Run only XDR unit tests
    $(basename "$0") --label xdr

    # Run one CTest target with verbose output
    $(basename "$0") --filter test_nfs3_namespace_c --verbose

Environment Variables:
    NFS_TEST_SERVER   Default NFS server (can be overridden by --server)
    NFS_TEST_PORT     Default NFS port (can be overridden by --port)
    NFS_TEST_EXPORT   Default NFS export path (can be overridden by --export)

Exit Codes:
    0   All tests passed
    1   One or more tests failed
    2   Build, configuration, or argument error

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
            --label)
                TEST_LABEL="$2"
                shift 2
                ;;
            --list-tests)
                LIST_ONLY=1
                shift
                ;;
            --build-only)
                BUILD_ONLY=1
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
    echo "=== Building NFSv3 RPC C Test Suite ==="
    mkdir -p "$BUILD_DIR"

    cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug
    cmake --build "$BUILD_DIR" --parallel"$(nproc 2>/dev/null || echo 4)"

    echo "✓ Build successful"
}

run_tests() {
    local CTEST_ARGS=(
        "--test-dir" "$BUILD_DIR"
    )

    if [[ $LIST_ONLY -eq 1 ]]; then
        CTEST_ARGS+=("-N")
    else
        CTEST_ARGS+=("--output-on-failure")
    fi

    if [[ $VERBOSE -eq 1 ]]; then
        CTEST_ARGS+=("--verbose")
    fi

    if [[ -n "$TEST_FILTER" ]]; then
        CTEST_ARGS+=("-R" "$TEST_FILTER")
    fi

    if [[ -n "$TEST_LABEL" ]]; then
        CTEST_ARGS+=("-L" "$TEST_LABEL")
    fi

    echo ""
    echo "========================================="
    echo "  NFSv3 RPC C Test Suite"
    echo "========================================="
    echo "Server: ${SERVER:-<not set>}"
    echo "Port:   $PORT"
    echo "Export: ${EXPORT:-<not set>}"
    echo "Build:  $BUILD_DIR"
    if [[ -n "$TEST_FILTER" ]]; then
        echo "Filter: $TEST_FILTER"
    fi
    if [[ -n "$TEST_LABEL" ]]; then
        echo "Label:  $TEST_LABEL"
    fi
    echo "========================================="
    echo ""

    if [[ -n "$SERVER" ]]; then
        export NFS_TEST_SERVER="$SERVER"
    fi
    export NFS_TEST_PORT="$PORT"
    if [[ -n "$EXPORT" ]]; then
        export NFS_TEST_EXPORT="$EXPORT"
    fi

    ctest "${CTEST_ARGS[@]}"
}

main() {
    parse_args "$@"
    build_project

    if [[ $BUILD_ONLY -eq 1 ]]; then
        exit 0
    fi

    run_tests
}

main "$@"
