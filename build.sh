#!/bin/bash
# =============================================================================
# Links Build Script for Linux and macOS
# Usage:
#   ./build.sh [command]
#
# Commands:
#   (none)    - Default Release build
#   release   - Release build
#   clean     - Clean build directory
#   test      - Run tests
#   help      - Show this help
# =============================================================================

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VCPKG_DIR="$PROJECT_ROOT/third_party/vcpkg"
BUILD_DIR="$PROJECT_ROOT/build"
SDK_ARCH="${LINKS_SDK_ARCH:-x64}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${YELLOW}[*]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[+]${NC} $1"
}

log_error() {
    echo -e "${RED}[!]${NC} $1"
}

# -----------------------------------------------------------------------------
# show_help
# -----------------------------------------------------------------------------
show_help() {
    echo ""
    echo "Links Build Script"
    echo "=================="
    echo ""
    echo "Usage: ./build.sh [command]"
    echo ""
    echo "Commands:"
    echo "  release   - Build Release version"
    echo "  clean     - Clean build directory"
    echo "  test      - Run tests"
    echo "  help      - Show this help"
    echo ""
}

# -----------------------------------------------------------------------------
# check_dependencies
# -----------------------------------------------------------------------------
check_dependencies() {
    log_info "Checking dependencies..."
    
    # Check for required tools
    for cmd in git cmake ninja; do
        if ! command -v $cmd &> /dev/null; then
            log_error "$cmd is required but not installed"
            exit 1
        fi
    done
    
    log_success "Dependencies OK"
}

# -----------------------------------------------------------------------------
# check_vcpkg
# -----------------------------------------------------------------------------
check_vcpkg() {
    log_info "Checking vcpkg..."
    
    if [ ! -d "$VCPKG_DIR" ]; then
        log_info "vcpkg not found, cloning..."
        git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
        if [ $? -ne 0 ]; then
            log_error "Failed to clone vcpkg"
            exit 1
        fi
    fi
    
    if [ ! -f "$VCPKG_DIR/vcpkg" ]; then
        log_info "Bootstrapping vcpkg..."
        "$VCPKG_DIR/bootstrap-vcpkg.sh" -disableMetrics
        if [ $? -ne 0 ]; then
            log_error "Failed to bootstrap vcpkg"
            exit 1
        fi
    fi
    
    log_success "vcpkg is ready"
}

# -----------------------------------------------------------------------------
# build_release
# -----------------------------------------------------------------------------
build_release() {
    check_dependencies
    check_vcpkg
    
    echo ""
    log_info "Configuring Release build..."
    cmake --preset release -DLINKS_SDK_ARCH="$SDK_ARCH"
    if [ $? -ne 0 ]; then
        log_error "CMake configuration failed"
        exit 1
    fi
    
    echo ""
    log_info "Building Release..."
    cmake --build --preset release
    if [ $? -ne 0 ]; then
        log_error "Build failed"
        exit 1
    fi
    
    echo ""
    log_success "Release build completed successfully"
}

# -----------------------------------------------------------------------------
# clean
# -----------------------------------------------------------------------------
clean() {
    log_info "Cleaning build directory..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        log_success "Build directory cleaned"
    else
        log_info "Build directory does not exist"
    fi
}

# -----------------------------------------------------------------------------
# run_tests
# -----------------------------------------------------------------------------
run_tests() {
    build_release
    
    echo ""
    log_info "Running tests..."
    ctest --preset release
    if [ $? -ne 0 ]; then
        log_error "Some tests failed"
        exit 1
    fi
    
    log_success "All tests passed"
}

# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------
COMMAND="${1:-release}"

case "$COMMAND" in
    help|--help|-h)
        show_help
        ;;
    clean)
        clean
        ;;
    test)
        run_tests
        ;;
    release)
        build_release
        ;;
    *)
        log_error "Unknown command: $COMMAND"
        show_help
        exit 1
        ;;
esac
