#!/bin/bash

echo "Building MMORPG Server System for Linux..."

# 현재 디렉토리 확인
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run from project root."
    exit 1
fi

# 필요한 패키지 확인
check_dependencies() {
    echo "Checking dependencies..."

    # GCC 확인
    if ! command -v g++ &> /dev/null; then
        echo "Error: g++ not found. Please install build-essential:"
        echo "  sudo apt-get install build-essential  # Ubuntu/Debian"
        echo "  sudo yum groupinstall 'Development Tools'  # CentOS/RHEL"
        exit 1
    fi

    # CMake 확인
    if ! command -v cmake &> /dev/null; then
        echo "Error: cmake not found. Please install cmake:"
        echo "  sudo apt-get install cmake  # Ubuntu/Debian"
        echo "  sudo yum install cmake  # CentOS/RHEL"
        exit 1
    fi

    # Make 확인
    if ! command -v make &> /dev/null; then
        echo "Error: make not found. Please install make:"
        echo "  sudo apt-get install make  # Ubuntu/Debian"
        echo "  sudo yum install make  # CentOS/RHEL"
        exit 1
    fi

    echo "✓ All dependencies found"
}

# 의존성 확인
check_dependencies

# 빌드 디렉토리 생성
mkdir -p build/linux
cd build/linux

# CMake 구성
echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=../../dist/linux \
      -DCMAKE_CXX_COMPILER=g++ \
      ../..

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# 빌드 실행 (병렬 빌드)
echo "Building (using $(nproc) cores)..."
make -j$(nproc)

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
echo "Binaries are located in: ../../dist/linux/bin/"

# 빌드된 파일 목록 출력
echo ""
echo "Built executables:"
ls -la ../../dist/linux/bin/