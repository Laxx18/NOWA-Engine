cmake_minimum_required(VERSION 3.4)

project(fparser VERSION 4.5.2)

include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Type of build, options are: None Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()

option(BUILD_SHARED "Whether to build the shared object" ON)
option(BUILD_STATIC "Whether to build the static archive" OFF)

option(FP_SUPPORT_FLOAT_TYPE "Whether to enable float type support of the parser" ON)
option(FP_SUPPORT_LONG_DOUBLE_TYPE "Whether to enable long double type support of the parser" ON)
option(FP_SUPPORT_LONG_INT_TYPE "Whether to enable long int type support of the parser" ON)
option(FP_SUPPORT_COMPLEX_DOUBLE_TYPE "Whether to enable complex double type support of the parser" ON)
option(FP_SUPPORT_COMPLEX_FLOAT_TYPE "Whether to enable complex float type support of the parser" ON)
option(FP_SUPPORT_COMPLEX_LONG_DOUBLE_TYPE "Whether to enable complex long double type support of the parser" ON)

option(FP_DISABLE_DOUBLE_TYPE "Whether to disable basic double type support of the parser" OFF)
option(FP_SUPPORT_CPLUSPLUS11_MATH_FUNCS "Whether to use C++11 math functions in the parser " OFF)
option(FP_SUPPORT_MPFR_FLOAT_TYPE "Whether to enable MPFR float type support of the parser" OFF)
option(FP_SUPPORT_GMP_INT_TYPE "Whether to enable GMP int type support of the parser" OFF)

option(FP_DISABLE_SHORTCUT_LOGICAL_EVALUATION "Whether to use shortcut evalution for the & and | operators" OFF)
option(FP_NO_SUPPORT_OPTIMIZER "Whether to disable the optimizer" OFF)

option(FP_USE_THREAD_SAFE_EVAL "Whether to make Eval() function thread-safe at the cost of a small overhead" ON)
option(FP_USE_THREAD_SAFE_EVAL_WITH_ALLOCA "Whether to make Eval() function thread-safe using alloca() function (faster)" OFF)

set(FPARSER_INSTALL_CMAKECONFIG_DIR "share/cmake/fparser" CACHE STRING "Where to install CMake config file")

if(FP_SUPPORT_MPFR_FLOAT_TYPE)
  check_include_file(mpfr.h HAVE_MPFR_H)
  if(NOT HAVE_MPFR_H)
    message(FATAL_ERROR "mpfr.h is not found. Probably GNU MPFR is not installed? (required by FP_SUPPORT_MPFR_FLOAT_TYPE=ON)")
  endif()
endif()

if(FP_SUPPORT_GMP_INT_TYPE)
  check_include_file(gmp.h HAVE_GMP_H)
  if(NOT HAVE_GMP_H)
    message(FATAL_ERROR "gmp.h is not found. Probably GMP is not installed? (required by FP_SUPPORT_GMP_INT_TYPE=ON)")
  endif()
endif()

if(FP_USE_THREAD_SAFE_EVAL_WITH_ALLOCA)
  check_include_file(alloca.h HAVE_ALLOCA_H)
  if(NOT HAVE_ALLOCA_H)
    message(FATAL_ERROR "alloca.h is not found (required by FP_USE_THREAD_SAFE_EVAL_WITH_ALLOCA=ON)")
  endif()
endif()

check_function_exists(strtold FP_USE_STRTOLD)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/fpconfig.hh.in" "${CMAKE_CURRENT_BINARY_DIR}/fpconfig.hh")

set(fparser_CORE_HEADERS fparser.hh extrasrc/fpaux.hh extrasrc/fptypes.hh)
set(fparser_CORE_SOURCES fparser.cc fpoptimizer.cc)
set(fparser_MPFRFLOAT_SOURCES mpfr/MpfrFloat.cc)
set(fparser_MPFRFLOAT_MPFRDIR_HEADERS mpfr/MpfrFloat.hh)
set(fparser_MPFRFLOAT_ROOTDIR_HEADERS fparser_mpfr.hh)
set(fparser_GMPINT_SOURCES mpfr/GmpInt.cc)
set(fparser_GMPINT_MPFRDIR_HEADERS mpfr/GmpInt.hh)
set(fparser_GMPINT_ROOTDIR_HEADERS fparser_gmpint.hh)

set(fparser_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")
set(fparser_SOURCES ${fparser_CORE_SOURCES})
set(fparser_HEADERS ${fparser_CORE_HEADERS})
set(fparser_LIBRARIES m)

set(ENABLE_CXX11 0)

if(FP_SUPPORT_CPLUSPLUS11_MATH_FUNCS)
  set(ENABLE_CXX11 1)
endif()

if(FP_SUPPORT_MPFR_FLOAT_TYPE)
  list(APPEND fparser_LIBRARIES ${MPFR_LIBRARIES})
  list(APPEND fparser_SOURCES ${fparser_MPFRFLOAT_SOURCES})
  list(APPEND fparser_HEADERS ${fparser_MPFRFLOAT_MPFRDIR_HEADERS} ${fparser_MPFRFLOAT_ROOTDIR_HEADERS})
endif()

if(FP_SUPPORT_GMP_INT_TYPE)
  list(APPEND fparser_LIBRARIES ${GMP_LIBRARIES})
  list(APPEND fparser_SOURCES ${fparser_GMPINT_SOURCES})
  list(APPEND fparser_HEADERS ${fparser_GMPINT_MPFRDIR_HEADERS} ${fparser_GMPINT_ROOTDIR_HEADERS})
endif()

if(ENABLE_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# Create empty directory; needed since files in extrasrc refer to the source root by ".."
add_custom_target(extrasrcbuilddir ALL COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/extrasrc")

# Set the export macro
if(BUILD_SHARED)
  add_definitions(-DFPARSER_EXPORTS)  # Marks symbols for export in DLL
endif()

# Create the shared library
if(BUILD_SHARED)
  add_library(fparser SHARED ${fparser_SOURCES})
  add_dependencies(fparser extrasrcbuilddir)
  target_include_directories(fparser
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/extrasrc")
  target_link_libraries(fparser ${fparser_LIBRARIES})

  install(TARGETS fparser DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()

# Create the static library
if(BUILD_STATIC)
  add_library(fparser_static STATIC ${fparser_SOURCES})
  add_dependencies(fparser_static extrasrcbuilddir)
  target_include_directories(fparser_static
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/extrasrc")
  target_link_libraries(fparser_static ${fparser_LIBRARIES})

  install(TARGETS fparser_static DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()

if(BUILD_SHARED OR BUILD_STATIC)
  install(FILES ${fparser_CORE_HEADERS} DESTINATION include/fparser)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fpconfig.hh" DESTINATION include/fparser)
  if(FP_SUPPORT_MPFR_FLOAT_TYPE)
    install(FILES ${fparser_MPFRFLOAT_ROOTDIR_HEADERS} DESTINATION include/fparser)
    install(FILES ${fparser_MPFRFLOAT_HEADERS_MPFRDIR} DESTINATION include/fparser/mpfr)
  endif()
  if(FP_SUPPORT_GMP_INT_TYPE)
    install(FILES ${fparser_GMPINT_ROOTDIR_HEADERS} DESTINATION include/fparser)
    install(FILES ${fparser_GMPINT_HEADERS_MPFRDIR} DESTINATION include/fparser/mpfr)
  endif()

  configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${FPARSER_INSTALL_CMAKECONFIG_DIR}")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" DESTINATION "${FPARSER_INSTALL_CMAKECONFIG_DIR}")

  write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" DESTINATION "${FPARSER_INSTALL_CMAKECONFIG_DIR}")
endif()
