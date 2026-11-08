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

# 4. Precision Hardware Atelier GUI & ViewModels Library
add_library(acc_gui_lib STATIC
    src/ui/TelemetryStripChart.h
    src/ui/TelemetryStripChart.cpp
    src/ui/HsvColorWheelItem.h
    src/ui/HsvColorWheelItem.cpp
    src/ui/FanCurveEditorItem.h
    src/ui/FanCurveEditorItem.cpp
    src/ui/KeyboardVisualizerItem.h
    src/ui/KeyboardVisualizerItem.cpp
    src/app/ThemeEngine.h
    src/app/ThemeEngine.cpp
    src/app/ControlCenterController.h
    src/app/ControlCenterController.cpp
    src/ui/resources.qrc
)
target_include_directories(acc_gui_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(acc_gui_lib PUBLIC
    acc_hal
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::DBus
)

# 5. Desktop Application Executable
add_executable(acer-control-center
    src/main.cpp
)
target_link_libraries(acer-control-center PRIVATE acc_gui_lib)

# 6. Unit, Mock HAL & GUI Tests
add_executable(test_core_and_hal
    tests/unit/test_core_and_hal.cpp
)
target_link_libraries(test_core_and_hal PRIVATE acc_daemon_lib Qt6::Test)
add_test(NAME test_core_and_hal COMMAND test_core_and_hal)

add_executable(test_gui_atelier
    tests/gui/test_gui_atelier.cpp
)
target_link_libraries(test_gui_atelier PRIVATE acc_gui_lib Qt6::Test)
add_test(NAME test_gui_atelier COMMAND test_gui_atelier -platform offscreen)

# 7. Installation Targets (Fedora 45 / RPM compatible)
include(GNUInstallDirs)

install(TARGETS acer-control-center
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(TARGETS acer-control-daemon
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}
)

install(FILES data/applications/io.github.zethasaintlab.acer-control-center.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
install(FILES data/metainfo/io.github.zethasaintlab.acer-control-center.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
)
install(FILES data/icons/io.github.zethasaintlab.acer-control-center.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)
install(FILES data/dbus/io.github.zethasaintlab.AcerControlCenter1.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/interfaces
)
install(FILES data/dbus/io.github.zethasaintlab.AcerControlCenter1.conf
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system.d
)
install(FILES data/dbus/io.github.zethasaintlab.AcerControlCenter1.service
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system-services
)
install(FILES data/polkit/io.github.zethasaintlab.acercontrol.policy
    DESTINATION ${CMAKE_INSTALL_DATADIR}/polkit-1/actions
)
install(FILES data/systemd/acer-control-daemon.service
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/system
)
