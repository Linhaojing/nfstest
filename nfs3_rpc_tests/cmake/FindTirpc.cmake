find_package(PkgConfig)

if(PkgConfig_FOUND)
    pkg_check_modules(TIRPC libtirpc)
endif()

if(TIRPC_FOUND)
    if(NOT TARGET Tirpc::tirpc)
        add_library(Tirpc::tirpc SHARED IMPORTED)
        set_target_properties(Tirpc::tirpc PROPERTIES
            IMPORTED_LOCATION "${TIRPC_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${TIRPC_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${TIRPC_LDFLAGS}"
        )
    endif()
endif()
