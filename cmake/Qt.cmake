set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Core Widgets)

if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Widgets)
endif()

function(link_qt_libraries projectname)
    target_compile_definitions(${projectname}
        PRIVATE
            QT_NO_KEYWORDS
    )

    target_link_libraries(${projectname}
        PRIVATE
            Qt::Core
            Qt::Widgets
    )
endfunction()
