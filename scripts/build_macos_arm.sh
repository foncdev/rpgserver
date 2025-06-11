#!/bin/bash

echo "Building MMORPG Server System for macOS (Apple Silicon)..."

# 현재 디렉토리 확인
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run from project root."
    exit 1
fi

# Homebrew 경로 설정 (Apple Silicon)
if [ -d "/opt/homebrew" ]; then
    export PATH="/opt/homebrew/bin:$PATH"
fi

# 의존성 확인
check_dependencies() {
    echo "Checking dependencies..."

    # Xcode Command Line Tools 확인
    if ! xcode-select -p &> /dev/null; then
        echo "Error: Xcode Command Line Tools not found."
        echo "Please install with: xcode-select --install"
        exit 1
    fi

    # CMake 확인
    if ! command -v cmake &> /dev/null; then
        echo "Error: CMake not found."
        echo "Please install CMake via Homebrew:"
        echo "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        echo "  brew install cmake"
        exit 1
    fi

    # Make 확인
    if ! command -v make &> /dev/null; then
        echo "Error: make not found."
        echo "Please install Xcode Command Line Tools: xcode-select --install"
        exit 1
    fi

    echo "✓ All dependencies found"
}

# 의존성 확인
check_dependencies

# 빌드 디렉토리 생성
mkdir -p build/macos_arm
cd build/macos_arm

# CMake 구성 (Apple Silicon)
echo "Configuring with CMake for Apple Silicon..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=../../dist/macos_arm \
      -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
      ../..

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# 빌드 실행
CPU_COUNT=$(sysctl -n hw.ncpu)
echo "Building (using $CPU_COUNT cores)..."
make -j$CPU_COUNT

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

# 설치
echo "Installing..."
make install

if [ $? -ne 0 ]; then
    echo "Installation failed!"
    exit 1
fi

echo "Build completed successfully!"
echo "Binaries are located in: ../../dist/macos_arm/bin/"

# 빌드된 파일 목록 출력
echo ""
echo "Built executables:"
ls -la ../../dist/macos_arm/bin/