#!/bin/bash

echo "Cleaning MMORPG Server System build directories..."

# 현재 디렉토리 확인
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run from project root."
    exit 1
fi

# 빌드 디렉토리 제거
if [ -d "build" ]; then
    echo "Removing build/ directory..."
    rm -rf build/
    echo "✓ build/ directory removed"
else
    echo "build/ directory not found (already clean)"
fi

# 배포 디렉토리 제거
if [ -d "dist" ]; then
    echo "Removing dist/ directory..."
    rm -rf dist/
    echo "✓ dist/ directory removed"
else
    echo "dist/ directory not found (already clean)"
fi

# CMake 캐시 파일 제거
echo "Removing CMake cache files..."
find . -name "CMakeCache.txt" -delete 2>/dev/null
find . -name "CMakeFiles" -type d -exec rm -rf {} + 2>/dev/null

# IDE 생성 파일 제거 (선택적)
echo "Removing IDE files..."
find . -name "*.xcodeproj" -type d -exec rm -rf {} + 2>/dev/null
find . -name "*.sln" -delete 2>/dev/null
find . -name "*.vcxproj*" -delete 2>/dev/null

echo "✓ Clean completed!"
echo ""
echo "All build artifacts have been removed."
echo "You can now run a fresh build."