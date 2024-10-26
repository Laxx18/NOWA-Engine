cmake_minimum_required(VERSION 3.16)

project(NOWALuaScript VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Option to specify the Qt bin directory
# Note: In Qt Creator goto project -> filter search for qt_bin_dir and specify the bin diretory there.
set(QT_BIN_DIR "E:/Qt/6.7.2/mingw_64/bin" CACHE STRING "Path to the Qt bin directory")

# Read the NOWA_ENGINE environment variable
set(NOWA_ENGINE_PATH $ENV{NOWA})

# Print it out (for debugging purposes)
message(STATUS "NOWA path is: ${NOWA_ENGINE_PATH}")

find_package(Qt6 REQUIRED COMPONENTS Quick Concurrent)

# Manually specifies the library path
set(LUA_LIBRARIES "${CMAKE_SOURCE_DIR}/lua-5_2/lib/lua.lib")
set(LUA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/lua-5_2/include")

qt_standard_project_setup()

qt_add_executable(NOWALuaScript
    main.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/qml)

set(QML_FILES
    qml_files/DetailsArea.qml
    qml_files/LuaEditor.qml
    qml_files/Main.qml
    qml_files/MainMenu.qml
    qml_files/SearchDialog.qml
    qml_files/IntelliSenseContextMenu.qml
)

qt_add_resources(NOWALuaScript "qml"
    PREFIX
        "/"
    FILES
        ${QML_FILES}
)

qt_add_qml_module(NOWALuaScript
    URI NOWALuaScript
    VERSION 1.0
    #QML_FILES qml_files/Main.qml
    #QML_FILES qml_files/LuaEditor.qml
    #QML_FILES qml_files/DetailsArea.qml
    #QML_FILES qml_files/MainMenu.qml
    SOURCES luascriptcontroller.h luascriptcontroller.cpp
    SOURCES luascriptqmladapter.h luascriptqmladapter.cpp
    SOURCES backend/luascript.h
    SOURCES backend/luascript.cpp
    SOURCES model/luaeditormodelitem.h model/luaeditormodelitem.cpp
    SOURCES model/luaeditormodel.h model/luaeditormodel.cpp
    SOURCES model/apimodel.h model/apimodel.cpp
    SOURCES model/matchclassworker.h model/matchclassworker.cpp
    SOURCES qml/luaeditorqml.h qml/luaeditorqml.cpp
    SOURCES qml/luahighlighter.h qml/luahighlighter.cpp
    SOURCES luascriptadapter.h luascriptadapter.cpp
    RESOURCES qml.qrc
)

set_target_properties(NOWALuaScript PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include_directories(${LUA_INCLUDE_DIR})

target_link_libraries(NOWALuaScript
    PRIVATE Qt6::Quick Qt6::Concurrent
    ${LUA_LIBRARIES}
)

include(GNUInstallDirs)
install(TARGETS NOWALuaScript
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Generate the deployment script for QML-based Qt6 applications
#qt_generate_deploy_qml_app_script(
#    TARGET NOWALuaScript
#    OUTPUT_SCRIPT deploy_script
#)

# Install the deploy script and resources
#install(SCRIPT ${deploy_script})

if(CMAKE_BUILD_TYPE STREQUAL "Release")

    # Copy DLLs based on the user-defined QT_BIN_DIR
    if(WIN32)
        # Find all .dll files in the source folder
        file(GLOB DLL_FILES "${QT_BIN_DIR}/*.dll")

        # Create a list to store the commands
        set(COPY_COMMANDS)

        # Iterate over each DLL file and append copy commands
        foreach(DLL_FILE ${DLL_FILES})
            list(APPEND COPY_COMMANDS
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DLL_FILE}" "${CMAKE_BINARY_DIR}/bin"
            )
        endforeach()

        # Add a post-build command to copy each .dll file
        add_custom_command(TARGET NOWALuaScript POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying DLL files for Release build..."
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
            ${COPY_COMMANDS}
            # Copy thirdparty like lua
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/lua-5_2/bin/Lua.dll" "${CMAKE_BINARY_DIR}/bin"
            # Copy exe from build/.../ to the bin dir
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:NOWALuaScript> "${CMAKE_BINARY_DIR}/bin"
            COMMENT "Copying necessary Qt libraries and executable to the output directory"
            # Copies missing necessary files via Qt script automatically
            COMMAND ${QT_BIN_DIR}/windeployqt --dir ${CMAKE_BINARY_DIR}/bin NOWALuaScript.exe --qmldir ${CMAKE_SOURCE_DIR}
            COMMENT "Deploying Qt DLLs and QML files using windeployqt"
        )

        # Source and destination directories
        set(SOURCE_DIR "${CMAKE_BINARY_DIR}/bin")
        set(DESTINATION_DIR "${NOWA_ENGINE_PATH}/external/NOWALuaScript/bin")

        add_custom_command(
            TARGET NOWALuaScript  # Replace with the name of your build target
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${SOURCE_DIR} ${DESTINATION_DIR}
            COMMENT "Copying all files and subdirectories from ${SOURCE_DIR} to ${DESTINATION_DIR} after build"
        )

    elseif(APPLE)
        # Similar setup for macOS
        add_custom_command(TARGET NOWALuaScript POST_BUILD
                COMMAND ${QT_BIN_DIR}/macdeployqt $<TARGET_BUNDLE_DIR:NOWALuaScript> -qmldir=${CMAKE_SOURCE_DIR}/qml
                COMMENT "Deploying Qt frameworks and QML files using macdeployqt"
        )
    endif()

endif()

message(STATUS "--->Executable path: ${CMAKE_BINARY_DIR}/NOWALuaScript.exe")
message(STATUS "--->Target file: $<TARGET_FILE:NOWALuaScript>")
message(STATUS "--->Bin path: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "--->Copy path: ${Qt6_DIR}")
message(STATUS "--->Source path: ${CMAKE_SOURCE_DIR}")
message(STATUS "--->Install Prefix path: ${Qt6Core_INSTALL_PREFIX}")
message(STATUS "--->Deploy script path: ${deploy_script}")
