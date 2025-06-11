#!/bin/bash
# =============================================================================
# scripts/build_all.sh - 통합 빌드 스크립트
# =============================================================================

echo "========================================="
echo "MMORPG Server System - Multi-Platform Build"
echo "========================================="

# 스크립트 위치 확인
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Project Root: $PROJECT_ROOT"
echo "Script Directory: $SCRIPT_DIR"

# 프로젝트 루트로 이동
cd "$PROJECT_ROOT"

BUILD_SUCCESS=0
BUILD_FAILED=0

# 운영체제 감지
OS=$(uname -s)
ARCH=$(uname -m)

echo "Detected OS: $OS"
echo "Detected Architecture: $ARCH"
echo ""

# 스크립트 실행 권한 부여
chmod +x scripts/*.sh

# 플랫폼별 빌드
case "$OS" in
    "Linux")
        echo "Building for Linux..."
        if [ -f "scripts/build_linux.sh" ]; then
            ./scripts/build_linux.sh
            if [ $? -eq 0 ]; then
                BUILD_SUCCESS=$((BUILD_SUCCESS + 1))
                echo "✓ Linux build completed"
            else
                BUILD_FAILED=$((BUILD_FAILED + 1))
                echo "✗ Linux build failed"
            fi
        else
            echo "build_linux.sh not found!"
            BUILD_FAILED=$((BUILD_FAILED + 1))
        fi
        ;;
    "Darwin")
        if [ "$ARCH" = "arm64" ]; then
            echo "Building for macOS (Apple Silicon)..."
            if [ -f "scripts/build_macos_arm.sh" ]; then
                ./scripts/build_macos_arm.sh
                if [ $? -eq 0 ]; then
                    BUILD_SUCCESS=$((BUILD_SUCCESS + 1))
                    echo "✓ macOS (Apple Silicon) build completed"
                else
                    BUILD_FAILED=$((BUILD_FAILED + 1))
                    echo "✗ macOS (Apple Silicon) build failed"
                fi
            else
                echo "build_macos_arm.sh not found!"
                BUILD_FAILED=$((BUILD_FAILED + 1))
            fi
        else
            echo "Building for macOS (Intel)..."
            if [ -f "scripts/build_macos_intel.sh" ]; then
                ./scripts/build_macos_intel.sh
                if [ $? -eq 0 ]; then
                    BUILD_SUCCESS=$((BUILD_SUCCESS + 1))
                    echo "✓ macOS (Intel) build completed"
                else
                    BUILD_FAILED=$((BUILD_FAILED + 1))
                    echo "✗ macOS (Intel) build failed"
                fi
            else
                echo "build_macos_intel.sh not found!"
                BUILD_FAILED=$((BUILD_FAILED + 1))
            fi
        fi
        ;;
    "CYGWIN"*|"MINGW"*|"MSYS"*)
        echo "Windows environment detected (Cygwin/MinGW/MSYS)"
        echo "Please use build_all.bat for Windows builds"
        exit 1
        ;;
    *)
        echo "Unsupported OS: $OS"
        echo "Supported platforms: Linux, macOS"
        echo "For Windows, use build_all.bat"
        exit 1
        ;;
esac

echo ""
echo "========================================="
echo "Build Summary:"
echo "Successful: $BUILD_SUCCESS"
echo "Failed: $BUILD_FAILED"
echo "========================================="

if [ $BUILD_FAILED -gt 0 ]; then
    echo "Some builds failed. Check the error messages above."
    exit 1
else
    echo "All builds completed successfully!"
    echo ""
    echo "Binaries location:"
    case "$OS" in
        "Linux")
            echo "  Linux: ./dist/linux/bin/"
            ;;
        "Darwin")
            if [ "$ARCH" = "arm64" ]; then
                echo "  macOS (Apple Silicon): ./dist/macos_arm/bin/"
            else
                echo "  macOS (Intel): ./dist/macos_intel/bin/"
            fi
            ;;
    esac
    echo ""
    echo "You can now run the servers:"
    echo "  ./dist/*/bin/AuthServer"
    echo "  ./dist/*/bin/GatewayServer"
    echo "  ./dist/*/bin/GameServer"
    echo "  ./dist/*/bin/ZoneServer"
    echo "  ./dist/*/bin/TestClient"
fi