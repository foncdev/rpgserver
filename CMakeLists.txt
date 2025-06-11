# =============================================================================
# CMakeLists.txt (Root) - Updated with Common Libraries
# =============================================================================

cmake_minimum_required(VERSION 3.16)
project(MMORPG_Server_System VERSION 1.0.0)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 컴파일러별 옵션 설정
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /O2")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
endif()

# 플랫폼별 설정
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-D_WIN32_WINNT=0x0601)
    set(PLATFORM_LIBS ws2_32)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LIBS pthread)
elseif(APPLE)
    set(PLATFORM_LIBS pthread)
endif()

# 출력 디렉토리 설정
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 공통 라이브러리 생성
add_library(CommonLib STATIC
        common/log_manager.h
        common/log_manager.cpp
        common/config_manager.h
        common/config_manager.cpp
)

target_include_directories(CommonLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(CommonLib ${PLATFORM_LIBS})

# 네트워크 라이브러리 생성
add_library(NetworkLib STATIC
        network/network_manager.h
        network/network_manager.cpp
)

target_include_directories(NetworkLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(NetworkLib CommonLib ${PLATFORM_LIBS})

# 인증 서버
add_executable(AuthServer
        auth_server/main.cpp
)
target_link_libraries(AuthServer NetworkLib CommonLib)

# 게이트웨이 서버
add_executable(GatewayServer
        gateway_server/main.cpp
)
target_link_libraries(GatewayServer NetworkLib CommonLib)

# 게임 서버
add_executable(GameServer
        game_server/main.cpp
)
target_link_libraries(GameServer NetworkLib CommonLib)

# 존 서버
add_executable(ZoneServer
        zone_server/main.cpp
)
target_link_libraries(ZoneServer NetworkLib CommonLib)

# 테스트 클라이언트
add_executable(TestClient
        test_client/main.cpp
)
target_link_libraries(TestClient NetworkLib CommonLib)

# 설치 규칙
install(TARGETS AuthServer GatewayServer GameServer ZoneServer TestClient
        RUNTIME DESTINATION bin)

# 설정 파일 및 디렉토리 설치
install(DIRECTORY DESTINATION config)
install(DIRECTORY DESTINATION logs)

# CPack 설정 (패키징)
set(CPACK_PACKAGE_NAME "MMORPG_Server_System")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MMORPG Multi-Server System with Logging and Configuration")
set(CPACK_PACKAGE_VENDOR "MMORPG Dev Team")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)