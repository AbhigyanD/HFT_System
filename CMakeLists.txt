cmake_minimum_required(VERSION 3.10)
project(HFT_System)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 (or Qt5 if Qt6 is not available)
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
if (NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
endif()

# Set up Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB SRC_FILES
    "src/*.cpp"
)

# Remove chart_widget from core system (it's GUI only)
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/chart_widget.cpp")
# Remove backtest-only sources from live nanoex
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/backtest_main.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/backtester.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/mean_reversion_strategy.cpp")

add_executable(nanoex ${SRC_FILES})
target_include_directories(nanoex PRIVATE src)

add_executable(backtest
  src/backtest_main.cpp
  src/backtester.cpp
  src/indicators.cpp
  src/mean_reversion_strategy.cpp
  src/order_book.cpp
  src/strategy.cpp
)
target_include_directories(backtest PRIVATE src)

# HFT System executable (original)
add_executable(nanoex_gui nanoex_gui.cpp src/chart_widget.cpp)
target_include_directories(nanoex_gui PRIVATE src)
target_link_libraries(nanoex_gui PRIVATE 
    Qt::Core 
    Qt::Widgets
)

# Set properties for GUI
set_target_properties(nanoex_gui PROPERTIES
    OUTPUT_NAME "NanoEX HFT System"
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
)

# Create Info.plist for macOS bundle
if(APPLE)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
        @ONLY
    )
endif()

# Installation
install(TARGETS nanoex nanoex_gui
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

# Print configuration info
message(STATUS "Building NanoEX HFT System")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt Version: ${Qt_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}") 