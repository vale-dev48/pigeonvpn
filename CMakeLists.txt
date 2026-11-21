cmake_minimum_required(VERSION 3.21)
project(PigeonVPN VERSION 1.0.0 LANGUAGES CXX)
if(WIN32)
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Qt6::Concurrent non serve: tutto il lavoro esterno passa da QProcess asincrono.
find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Network)

add_executable(PigeonVPN WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/core/WireGuardConfig.h
    src/core/WireGuardConfig.cpp
    src/core/VpnTypes.h
    src/core/Format.h
    src/vpn/IVpnBackend.h
    src/vpn/SimulatedBackend.h
    src/vpn/SimulatedBackend.cpp
    src/vpn/WgQuickBackend.h
    src/vpn/WgQuickBackend.cpp
    src/ui/PigeonMascot.h
    src/ui/PigeonMascot.cpp
    src/ui/Sparkline.h
    src/ui/Sparkline.cpp
    src/ui/Backdrop.h
    src/ui/Theme.h
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

if(WIN32)
    target_sources(PigeonVPN PRIVATE assets/pigeonvpn.rc)  # icona dell'.exe
endif()

target_include_directories(PigeonVPN PRIVATE src)
target_link_libraries(PigeonVPN PRIVATE Qt6::Widgets Qt6::Network)

if(MSVC)
    target_compile_options(PigeonVPN PRIVATE /W4 /permissive-)
else()
    target_compile_options(PigeonVPN PRIVATE -Wall -Wextra)
endif()
