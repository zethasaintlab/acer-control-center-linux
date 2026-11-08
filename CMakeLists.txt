cmake_minimum_required(VERSION 3.26)
project(acer-control-center-linux
    VERSION 1.0.0
    DESCRIPTION "Acer Control Center — Linux Edition (Precision Hardware Atelier)"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_compile_options(-Wall -Wextra -Wpedantic)

find_package(PkgConfig REQUIRED)
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Qml
    Quick
    QuickControls2
    DBus
    Test
)
pkg_check_modules(POLKIT REQUIRED IMPORTED_TARGET polkit-gobject-1)

enable_testing()

# 1. Core Domain & Color/Config/Redaction Library
add_library(acc_core STATIC
    src/core/CapabilityState.h
    src/core/CapabilityState.cpp
    src/core/DomainModels.h
    src/core/ColorScience.h
    src/core/ColorScience.cpp
    src/core/ConfigStore.h
    src/core/ConfigStore.cpp
    src/core/DiagnosticRedactor.h
    src/core/DiagnosticRedactor.cpp
)
target_include_directories(acc_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(acc_core PUBLIC Qt6::Core Qt6::Gui)

# 2. Hardware Abstraction Layer (Discovery, Telemetry, Control, Mock Sysfs, Dynamic NVML)
add_library(acc_hal STATIC
    src/hal/SysfsRoot.h
    src/hal/SysfsRoot.cpp
    src/hal/NvmlDynamicLoader.h
    src/hal/NvmlDynamicLoader.cpp
    src/hal/HardwareDiscovery.h
    src/hal/HardwareDiscovery.cpp
    src/hal/TelemetryCollector.h
    src/hal/TelemetryCollector.cpp
    src/hal/ControlOperations.h
    src/hal/ControlOperations.cpp
)
target_include_directories(acc_hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(acc_hal PUBLIC acc_core Qt6::Core Qt6::DBus ${CMAKE_DL_LIBS})

# 3. Privileged D-Bus Service Components
add_library(acc_daemon_lib STATIC
    src/daemon/PolkitAuthorizer.h
    src/daemon/PolkitAuthorizer.cpp
    src/daemon/DaemonServiceObject.h
    src/daemon/DaemonServiceObject.cpp
)
target_include_directories(acc_daemon_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(acc_daemon_lib PUBLIC acc_hal Qt6::Core Qt6::DBus PkgConfig::POLKIT)

add_executable(acer-control-daemon
    src/daemon/main_daemon.cpp
)
target_link_libraries(acer-control-daemon PRIVATE acc_daemon_lib)

# 4. Unit & Mock HAL Tests
add_executable(test_core_and_hal
    tests/unit/test_core_and_hal.cpp
)
target_link_libraries(test_core_and_hal PRIVATE acc_daemon_lib Qt6::Test)
add_test(NAME test_core_and_hal COMMAND test_core_and_hal)
