cmake_minimum_required(VERSION 3.16)
project(LightningXML
    VERSION 2.0.0
    LANGUAGES CXX
    DESCRIPTION "High-performance XML pull-parser for C++20"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
message(STATUS "LightningXML: Build Type = ${CMAKE_BUILD_TYPE}.")

# ---- Options ----
option(LIGHTNINGXML_BUILD_TESTS       "Build unit tests (requires GTest)"                      ON)
option(LIGHTNINGXML_BUILD_BENCHMARKS  "Build benchmarks (requires Google Benchmark)"           ON)
option(LIGHTNINGXML_BUILD_CODEGEN     "Build the XSD -> XmlMetadata code generator (xsdgen)"   ON)
option(LIGHTNINGXML_WITH_PUGIXML      "Build pugixml comparison benchmarks"                    OFF)
option(LIGHTNINGXML_WITH_RAPIDXML     "Build RapidXML comparison benchmarks (via Boost)"       OFF)
option(LIGHTNINGXML_WITH_LIBXML2      "Build libxml2 comparison benchmarks"                    OFF)
option(LIGHTNINGXML_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(LIGHTNINGXML_ENABLE_SANITIZERS)
    message(STATUS "LightningXML: sanitizers enabled")

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        add_compile_options(
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )

        add_link_options(
            -fsanitize=address,undefined
        )
    else()
        message(FATAL_ERROR
            "Sanitizers are only supported for GCC/Clang"
        )
    endif()
endif()

# ---- Header-only library target ----
add_library(lightningxml INTERFACE)
add_library(LightningXML::lightningxml ALIAS lightningxml)

target_include_directories(lightningxml INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(lightningxml INTERFACE cxx_std_20)

# ---- Shared test helpers (used by both tests and benchmarks) ----
add_library(lightningxml_helpers INTERFACE)
target_include_directories(lightningxml_helpers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/test
)
target_link_libraries(lightningxml_helpers INTERFACE lightningxml)

# ---- Dependencies via FetchContent ----
include(FetchContent)

# ---- Tests ----
if(LIGHTNINGXML_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
        FIND_PACKAGE_ARGS NAMES GTest
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(lightningxml_tests
        test/test_LightningXML.cc
        test/test_Conformance.cc
        test/test_XSDCodegen.cc
    )
    target_include_directories(lightningxml_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tools
    )
    target_link_libraries(lightningxml_tests PRIVATE
        lightningxml_helpers
        GTest::gtest_main
    )
    target_compile_options(lightningxml_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
    )

    include(GoogleTest)
    gtest_discover_tests(lightningxml_tests)

    message(STATUS "LightningXML: tests enabled")
endif()

# ---- Benchmarks ----
if(LIGHTNINGXML_BUILD_BENCHMARKS)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.1
        FIND_PACKAGE_ARGS NAMES benchmark
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)

    # pugixml (optional)
    if(LIGHTNINGXML_WITH_PUGIXML)
        FetchContent_Declare(
            pugixml
            GIT_REPOSITORY https://github.com/zeux/pugixml.git
            GIT_TAG        v1.14
            FIND_PACKAGE_ARGS NAMES pugixml
        )
        FetchContent_MakeAvailable(pugixml)
        message(STATUS "LightningXML: pugixml comparison benchmarks enabled")
    endif()

    # RapidXML (optional). RapidXML has no canonical upstream package, so we use
    # the copy bundled in Boost.PropertyTree (boost::property_tree::detail::rapidxml).
    # The official Boost "-cmake" release archive ships every header with no git
    # submodules; BOOST_INCLUDE_LIBRARIES restricts configuration to the
    # property_tree subtree (and its dependencies) so the first build stays quick.
    if(LIGHTNINGXML_WITH_RAPIDXML)
        set(BOOST_INCLUDE_LIBRARIES property_tree CACHE STRING "" FORCE)
        set(BOOST_ENABLE_CMAKE ON CACHE BOOL "" FORCE)
        FetchContent_Declare(
            Boost
            URL https://github.com/boostorg/boost/releases/download/boost-1.88.0/boost-1.88.0-cmake.tar.xz
        )
        FetchContent_MakeAvailable(Boost)
        message(STATUS "LightningXML: RapidXML (Boost) comparison benchmarks enabled")
    endif()

    # libxml2 (optional). A feature-rich parser: copies all strings, decodes
    # entities, and fully validates well-formedness. Fetched and built from
    # source with the optional codecs/tools disabled to keep the build lean.
    if(LIGHTNINGXML_WITH_LIBXML2)
        set(LIBXML2_WITH_PROGRAMS OFF CACHE BOOL "" FORCE)
        set(LIBXML2_WITH_TESTS    OFF CACHE BOOL "" FORCE)
        set(LIBXML2_WITH_PYTHON   OFF CACHE BOOL "" FORCE)
        set(LIBXML2_WITH_ICONV    OFF CACHE BOOL "" FORCE)
        set(LIBXML2_WITH_ZLIB     OFF CACHE BOOL "" FORCE)
        set(LIBXML2_WITH_LZMA     OFF CACHE BOOL "" FORCE)
        set(LIBXML2_WITH_HTTP     OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            libxml2
            GIT_REPOSITORY https://gitlab.gnome.org/GNOME/libxml2.git
            GIT_TAG        v2.14.6
        )
        FetchContent_MakeAvailable(libxml2)
        message(STATUS "LightningXML: libxml2 comparison benchmarks enabled")
    endif()

    # RapidXML (optional). RapidXML has no canonical upstream package, so we use
    # the copy bundled in Boost.PropertyTree (boost::property_tree::detail::rapidxml).
    # The official Boost "-cmake" release archive ships every header with no git
    # submodules; BOOST_INCLUDE_LIBRARIES restricts configuration to the
    # property_tree subtree (and its dependencies) so the first build stays quick.
    if(TURBOXML_WITH_RAPIDXML)
        set(BOOST_INCLUDE_LIBRARIES property_tree CACHE STRING "" FORCE)
        set(BOOST_ENABLE_CMAKE ON CACHE BOOL "" FORCE)
        FetchContent_Declare(
            Boost
            URL https://github.com/boostorg/boost/releases/download/boost-1.88.0/boost-1.88.0-cmake.tar.xz
        )
        FetchContent_MakeAvailable(Boost)
        message(STATUS "TurboXML: RapidXML (Boost) comparison benchmarks enabled")
    endif()

    # libxml2 (optional). A feature-rich parser: copies all strings, decodes
    # entities, and fully validates well-formedness. Fetched and built from
    # source with the optional codecs/tools disabled to keep the build lean.
    if(TURBOXML_WITH_LIBXML2)
        set(LIBXML2_WITH_PROGRAMS OFF CACHE BOOL "" FORCE)
        set(LIBXML2_WITH_TESTS    OFF CACHE BOOL "" FORCE)
        set(LIBXML2_WITH_PYTHON   OFF CACHE BOOL "" FORCE)
        set(LIBXML2_WITH_ICONV    OFF CACHE BOOL "" FORCE)
        set(LIBXML2_WITH_ZLIB     OFF CACHE BOOL "" FORCE)
        set(LIBXML2_WITH_LZMA     OFF CACHE BOOL "" FORCE)
        set(LIBXML2_WITH_HTTP     OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            libxml2
            GIT_REPOSITORY https://gitlab.gnome.org/GNOME/libxml2.git
            GIT_TAG        v2.14.6
        )
        FetchContent_MakeAvailable(libxml2)
        message(STATUS "TurboXML: libxml2 comparison benchmarks enabled")
    endif()

    # --- Timing benchmarks ---
    add_executable(lightningxml_bench test/bench_LightningXML.cc)
    target_link_libraries(lightningxml_bench PRIVATE lightningxml_helpers benchmark::benchmark)
    if(LIGHTNINGXML_WITH_PUGIXML)
        target_link_libraries(lightningxml_bench PRIVATE pugixml::pugixml)
        target_compile_definitions(lightningxml_bench PRIVATE LIGHTNINGXML_HAS_PUGIXML)
    endif()
    if(LIGHTNINGXML_WITH_RAPIDXML)
        target_link_libraries(lightningxml_bench PRIVATE Boost::property_tree)
        target_compile_definitions(lightningxml_bench PRIVATE LIGHTNINGXML_HAS_RAPIDXML)
    endif()
    if(LIGHTNINGXML_WITH_LIBXML2)
        target_link_libraries(lightningxml_bench PRIVATE LibXml2::LibXml2)
        target_compile_definitions(lightningxml_bench PRIVATE LIGHTNINGXML_HAS_LIBXML2)
    endif()
    if(TURBOXML_WITH_RAPIDXML)
        target_link_libraries(turboxml_bench PRIVATE Boost::property_tree)
        target_compile_definitions(turboxml_bench PRIVATE TURBOXML_HAS_RAPIDXML)
    endif()
    if(TURBOXML_WITH_LIBXML2)
        target_link_libraries(turboxml_bench PRIVATE LibXml2::LibXml2)
        target_compile_definitions(turboxml_bench PRIVATE TURBOXML_HAS_LIBXML2)
    endif()

    message(STATUS "LightningXML: benchmarks enabled")
endif()

# ---- XSD code generator ----
if(LIGHTNINGXML_BUILD_CODEGEN)
    add_executable(lightningxml_xsdgen tools/XSDGen.cc)
    target_link_libraries(lightningxml_xsdgen PRIVATE lightningxml)
    target_include_directories(lightningxml_xsdgen PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tools
    )
    target_compile_options(lightningxml_xsdgen PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
    )
    message(STATUS "LightningXML: XSD code generator enabled")
endif()

# ---- XSD code generator ----
if(TURBOXML_BUILD_CODEGEN)
    add_executable(turboxml_xsdgen tools/XSDGen.cc)
    target_link_libraries(turboxml_xsdgen PRIVATE turboxml)
    target_include_directories(turboxml_xsdgen PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tools
    )
    target_compile_options(turboxml_xsdgen PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
    )
    message(STATUS "TurboXML: XSD code generator enabled")
endif()

# Installation
set(INSTALL_DIR "${CMAKE_SOURCE_DIR}/bin")
foreach(_target lightningxml_tests lightningxml_bench lightningxml_xsdgen)
    if(TARGET ${_target})
        install(TARGETS ${_target} DESTINATION ${INSTALL_DIR})
    endif()
endforeach()
message(STATUS "Installing binaries to ${INSTALL_DIR}")