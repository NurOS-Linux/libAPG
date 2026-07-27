find_package(PkgConfig)
if(NOT PkgConfig_FOUND)
    set(libapg_FOUND FALSE)
    return()
endif()

pkg_check_modules(PC_LIBAPG QUIET IMPORTED_TARGET libapg)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libapg
    REQUIRED_VARS PC_LIBAPG_LIBRARIES
    VERSION_VAR PC_LIBAPG_VERSION
)

if(libapg_FOUND AND NOT TARGET libapg::libapg)
    add_library(libapg::libapg ALIAS PkgConfig::PC_LIBAPG)
endif()
