cmake_minimum_required(VERSION 3.16)
project(HFT_System LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# WebAssembly demo: only the single-threaded engine pieces (no threads, no Qt).
#   emcmake cmake -S . -B build-wasm && cmake --build build-wasm
if(EMSCRIPTEN)
    add_executable(engine
        src/wasm/engine_api.cpp
        src/backtester.cpp
        src/extra_strategies.cpp
        src/indicators.cpp
        src/mean_reversion_strategy.cpp
        src/order_book.cpp
        src/paper_sim.cpp
        src/strategy.cpp
        src/strategy_factory.cpp
    )
    target_include_directories(engine PRIVATE src)
    target_compile_options(engine PRIVATE -O2)
    target_link_options(engine PRIVATE
        -O2
        -sMODULARIZE=1
        -sEXPORT_NAME=NanoEX
        -sSINGLE_FILE=1
        -sALLOW_MEMORY_GROWTH=1
        -sEXPORTED_RUNTIME_METHODS=cwrap,UTF8ToString
        -sENVIRONMENT=web,node
    )
    set_target_properties(engine PROPERTIES SUFFIX ".js")
    return()
endif()

find_package(Threads REQUIRED)

# Engine (no Qt dependency)
add_library(nanoex_core STATIC
    src/backtester.cpp
    src/extra_strategies.cpp
    src/indicators.cpp
    src/market_data.cpp
    src/matching_engine.cpp
    src/mean_reversion_strategy.cpp
    src/order_book.cpp
    src/paper_sim.cpp
    src/performance.cpp
    src/risk.cpp
    src/strategy.cpp
    src/strategy_factory.cpp
    src/threading.cpp
)
target_include_directories(nanoex_core PUBLIC src)
target_link_libraries(nanoex_core PUBLIC Threads::Threads)

add_executable(nanoex src/main.cpp)
target_link_libraries(nanoex PRIVATE nanoex_core)

add_executable(backtest src/backtest_main.cpp)
target_link_libraries(backtest PRIVATE nanoex_core)

# Desktop app (optional: skipped when Qt6 is not installed)
if(APPLE)
    # Recent macOS SDKs dropped the AGL framework that Qt's FindWrapOpenGL still asks for.
    execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE MACOS_SDK OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(MACOS_SDK AND NOT EXISTS "${MACOS_SDK}/System/Library/Frameworks/AGL.framework")
        set(WrapOpenGL_AGL "${MACOS_SDK}/System/Library/Frameworks/OpenGL.framework" CACHE FILEPATH "" FORCE)
    endif()
endif()
find_package(Qt6 COMPONENTS Widgets QUIET)
if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    add_executable(nanoex_gui MACOSX_BUNDLE
        src/gui/main_gui.cpp
        src/gui/main_window.cpp
        src/gui/plot_widget.cpp
        src/gui/sim_worker.cpp
    )
    target_link_libraries(nanoex_gui PRIVATE nanoex_core Qt6::Widgets)
    set_target_properties(nanoex_gui PROPERTIES
        OUTPUT_NAME "NanoEX"
        MACOSX_BUNDLE_BUNDLE_NAME "NanoEX"
    )
    install(TARGETS nanoex_gui BUNDLE DESTINATION . RUNTIME DESTINATION bin)
else()
    message(STATUS "Qt6 not found: building command-line targets only")
endif()

install(TARGETS nanoex backtest RUNTIME DESTINATION bin)
