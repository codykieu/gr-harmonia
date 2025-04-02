find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_HARMONIA gnuradio-harmonia)

FIND_PATH(
    GR_HARMONIA_INCLUDE_DIRS
    NAMES gnuradio/harmonia/api.h
    HINTS $ENV{HARMONIA_DIR}/include
        ${PC_HARMONIA_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_HARMONIA_LIBRARIES
    NAMES gnuradio-harmonia
    HINTS $ENV{HARMONIA_DIR}/lib
        ${PC_HARMONIA_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-harmoniaTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_HARMONIA DEFAULT_MSG GR_HARMONIA_LIBRARIES GR_HARMONIA_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_HARMONIA_LIBRARIES GR_HARMONIA_INCLUDE_DIRS)
