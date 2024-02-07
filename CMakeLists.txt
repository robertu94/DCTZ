cmake_minimum_required(VERSION 3.18)
project(DCTZ VERSION 0.2.2)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "prefer to build shared libraries" ON)
if(NOT USE_SANITIZER)
  set(USE_SANITIZER "" CACHE STRING "use a build sanitizer" FORCE)
endif()
set_property(CACHE USE_SANITIZER PROPERTY STRINGS "" "Address" "Thread" "Undefined" "Leak" "Memory")
if (USE_SANITIZER STREQUAL "Address")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
elseif (USE_SANITIZER STREQUAL "Thread")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
elseif (USE_SANITIZER STREQUAL "Undefined")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
elseif (USE_SANITIZER STREQUAL "Leak")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=leak")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
elseif (USE_SANITIZER STREQUAL "Memory")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory")
endif()



find_package(ZLIB)
find_package(PkgConfig REQUIRED)
pkg_search_module(FFTW3 REQUIRED IMPORTED_TARGET fftw3)
pkg_search_module(FFTW3F REQUIRED IMPORTED_TARGET fftw3f)

function(dctz_executable name defines)
    add_executable(${name} dctz-test.c dctz-comp-lib.c dctz-decomp-lib.c binning.c util.c dct.c dct-float.c)
    target_link_libraries(${name} PRIVATE PkgConfig::FFTW3 PkgConfig::FFTW3F ZLIB::ZLIB m)
    target_compile_features(${name} PRIVATE c_std_99)
    target_compile_definitions(${name} PRIVATE ${defines})
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

dctz_executable(dctz-ec-test USE_TRUNCATE)
dctz_executable(dctz-qt-test "USE_TRUNCATE;USE_QTABLE")

option(DCTZ_HAVE_LIBPRESSIO "build support for libpressio" FALSE)
if(DCTZ_HAVE_LIBPRESSIO)
    find_package(LibPressio REQUIRED)
    find_package(std_compat REQUIRED)
    add_library(libpressio_dctz libpressio_dctz.cc dctz-comp-lib.c dctz-decomp-lib.c binning.c util.c dct.c dct-float.c)
    target_link_libraries(libpressio_dctz PRIVATE LibPressio::libpressio PkgConfig::FFTW3 PkgConfig::FFTW3F ZLIB::ZLIB m)
    target_compile_features(libpressio_dctz PRIVATE c_std_99)
    target_compile_definitions(libpressio_dctz PRIVATE "USE_TRUNCATE")
    target_include_directories(libpressio_dctz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(libpressio_dctz PUBLIC 
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        )
    install(TARGETS libpressio_dctz  EXPORT DCTZTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      )
    install(FILES libpressio_dctz.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/)
endif()


install(TARGETS
    dctz-ec-test
    dctz-qt-test
    EXPORT DCTZTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
install(EXPORT DCTZTargets NAMESPACE DCTZ:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DCTZ/)
export(EXPORT DCTZTargets FILE ${CMAKE_CURRENT_BINARY_DIR}/cmake/DCTZTargets.cmake NAMESPACE DCTZ::)
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/DCTZConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/DCTZConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DCTZ
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/DCTZConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY AnyNewerVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/DCTZConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/DCTZConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DCTZ
    )
