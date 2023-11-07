include(cmake/CPM.cmake)

if(BACKEND_QT)
    include(cmake/Qt.cmake)
endif()


if(BACKEND_GTK)
    include(cmake/GTK.cmake)
endif()

function(setup_dependencies)
    CPMAddPackage("gh:fmtlib/fmt#10.1.1")
    CPMAddPackage("gh:nlohmann/json@3.11.2")
    CPMAddPackage("gh:Dax89/cl#master")

    CPMAddPackage(
        NAME spdlog
        VERSION "1.12.0"
        GITHUB_REPOSITORY "gabime/spdlog"
        OPTIONS "SPDLOG_FMT_EXTERNAL ON"
    )
endfunction()
