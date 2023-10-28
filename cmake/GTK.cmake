find_package(PkgConfig REQUIRED)

# pkg_check_modules(GTK gtk4)

if(NOT GTK_FOUND)
    pkg_check_modules(GTK REQUIRED gtk+-3.0)
endif()

function(link_gtk_libraries projectname)
    target_include_directories(${projectname}
        PRIVATE
            ${GTK_INCLUDE_DIRS}
    )

    target_link_libraries(${projectname}
        PRIVATE
            ${GTK_LIBRARIES}
    )
endfunction()
