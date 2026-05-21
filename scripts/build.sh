#!/bin/bash
#
# build.sh - Build script for finkit-platform
#
# Usage:
#   ./scripts/build.sh                    # Build Linux native (default)
#   ./scripts/build.sh --all              # Build all platforms
#   ./scripts/build.sh --platform linux   # Build specific platform
#   ./scripts/build.sh --test             # Build and run tests
#   ./scripts/build.sh --help             # Show help
#
# Options:
#   --platform <name>   Build for specific platform (linux/windows/darwin)
#   --arch <name>       Target architecture (amd64/arm64)
#   --type <name>       Build type (Release/Debug)
#   --all               Build for all platforms
#   --test              Run tests after build
#   --clean             Clean before build
#   --help              Show this help
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Default values
BUILD_TYPE="${BUILD_TYPE:-Release}"
PLATFORM="${PLATFORM:-linux}"
ARCH="${ARCH:-amd64}"
BUILD_ALL=false
RUN_TEST=false
CLEAN=false

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --platform)
            PLATFORM="$2"
            shift 2
            ;;
        --arch)
            ARCH="$2"
            shift 2
            ;;
        --type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --all)
            BUILD_ALL=true
            shift
            ;;
        --test)
            RUN_TEST=true
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --help)
            sed -n '2,/^$/p' "$0" | grep "^#" | sed 's/^# \?//'
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

cd "$ROOT_DIR"

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo -e "${YELLOW}==> Cleaning build artifacts${NC}"
    rm -rf build/
fi

# Build single platform
build_platform() {
    local os=$1
    local arch=$2
    local build_dir="build/${os}_${arch}"

    echo -e "${GREEN}==> Building for ${os}_${arch} (${BUILD_TYPE})${NC}"

    # Determine toolchain
    local toolchain_arg=""
    local generator="Ninja"

    if [ "$os" = "windows" ]; then
        toolchain_arg="-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/x86_64-w64-mingw32.cmake"
        generator="Unix Makefiles"
    elif [ "$os" = "darwin" ] && [ "$arch" = "arm64" ]; then
        toolchain_arg="-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/aarch64-apple-darwin.cmake"
    fi

    # Configure
    cmake -B "$build_dir" \
        -G "$generator" \
        $toolchain_arg \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DFC_BUILD_TESTS=ON \
        -DFC_BUILD_BENCHMARKS=ON

    # Build
    cmake --build "$build_dir" --parallel

    echo -e "${GREEN}  ✓ Built: ${build_dir}/libfinkit_platform_static.a${NC}"
}

# Build all platforms
build_all_platforms() {
    echo "============================================================"
    echo " Building all platforms"
    echo "============================================================"
    echo ""

    local platforms=(
        "linux:amd64"
        "linux:arm64"
        "darwin:amd64"
        "darwin:arm64"
        "windows:amd64"
    )

    for platform_spec in "${platforms[@]}"; do
        local os=$(echo $platform_spec | cut -d: -f1)
        local arch=$(echo $platform_spec | cut -d: -f2)

        build_platform "$os" "$arch"
        echo ""
    done

    echo "============================================================"
    echo " All platforms built successfully"
    echo "============================================================"
    echo ""
    echo "Libraries available in build/ directory:"
    for platform_spec in "${platforms[@]}"; do
        local os=$(echo $platform_spec | cut -d: -f1)
        local arch=$(echo $platform_spec | cut -d: -f2)
        if [ -f "build/${os}_${arch}/libfinkit_platform_static.a" ]; then
            echo -e "  ${GREEN}✓${NC} build/${os}_${arch}/"
        fi
    done
}

# Run tests
run_tests() {
    echo ""
    echo -e "${YELLOW}==> Running tests${NC}"

    # Run Go tests
    echo "Running Go tests..."
    CGO_CFLAGS_ALLOW="-m(avx2|avx512f|avx512dq|fma|sse4\.2)" go test -v

    echo -e "${GREEN}✓ All tests passed${NC}"
}

# Main execution
echo "============================================================"
echo " finkit-platform build script"
echo "============================================================"
echo " Build type: ${BUILD_TYPE}"
echo " Platform:   ${PLATFORM}_${ARCH}"
echo "============================================================"
echo ""

if [ "$BUILD_ALL" = true ]; then
    build_all_platforms
else
    build_platform "$PLATFORM" "$ARCH"
fi

if [ "$RUN_TEST" = true ]; then
    run_tests
fi

echo ""
echo -e "${GREEN}============================================================${NC}"
echo -e "${GREEN} Build complete!${NC}"
echo -e "${GREEN}============================================================${NC}"
