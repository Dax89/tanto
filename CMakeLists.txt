cmake_minimum_required(VERSION 3.22)

project(tanto
    DESCRIPTION "Project Description"
    LANGUAGES CXX
)

option(BACKEND_GTK "Enable GTK backend" ON)
option(BACKEND_QT "Enable Qt backend" ON)

if(UNIX AND NOT APPLE)
 find_package(CURL REQUIRED)
endif()

set(PROJECT_NAME_TEST "${PROJECT_NAME}_test")

include(cmake/Settings.cmake)
include(cmake/Dependencies.cmake)
setup_dependencies()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    configure_file(
        "${CMAKE_SOURCE_DIR}/leak.supp"
        "${CMAKE_BINARY_DIR}/leak.supp"
        COPYONLY
    )
endif()

add_executable(${PROJECT_NAME})

include(cmake/Compiler.cmake)

target_sources(${PROJECT_NAME}
    PRIVATE
        "main.cpp"
        "src/events.cpp"
        "src/tanto.cpp"
        "src/types.cpp"
        "src/backend.cpp"
)

if(BACKEND_QT)
    target_sources(${PROJECT_NAME}
        PRIVATE
            "src/backends/qt/picture.cpp"
            "src/backends/qt/mainwindow.cpp"
            "src/backends/qt/backendimpl.cpp"
    )

    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            BACKEND_QT
    )

    link_qt_libraries(${PROJECT_NAME})
endif()

if(BACKEND_GTK)
    target_sources(${PROJECT_NAME}
        PRIVATE
            "src/backends/gtk/backendimpl.cpp"
    )

    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            BACKEND_GTK
    )

    link_gtk_libraries(${PROJECT_NAME})
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        cl
        nlohmann_json
        spdlog
        fmt
)

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
            CURL::libcurl
    )
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
            wininet
    )
endif()
