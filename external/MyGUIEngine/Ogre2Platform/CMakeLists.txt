set (PROJECTNAME MyGUI.Ogre2Platform)

include_directories(
 include
 ${MYGUI_SOURCE_DIR}/MyGUIEngine/include
 ${OGRE_INCLUDE_DIR}
 ${OGRE_INCLUDE_DIR}/HLMS/Unlit
 ${OGRE_INCLUDE_DIR}/HLMS/Common
 ${OGRE_SOURCE_DIR}
)

include(${PROJECTNAME}.list)

add_library(${PROJECTNAME} ${HEADER_FILES} ${SOURCE_FILES})

add_dependencies(${PROJECTNAME} MyGUIEngine)

target_link_libraries(${PROJECTNAME} ${OGRE_LIBRARIES} ${OGRE_HlmsPbs_LIBRARIES} ${OGRE_HlmsUnlit_LIBRARIES} MyGUIEngine)
link_directories(${OGRE_LIB_DIR})

# installation rules
install(FILES ${HEADER_FILES}
	DESTINATION include/MYGUI
)
mygui_install_target(${PROJECTNAME} "")

# platform specific dependencies
if (APPLE)
	include_directories(/usr/local/include /usr/include)
	target_link_libraries(${PROJECTNAME} ${Carbon_LIBRARY})
endif ()
