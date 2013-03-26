# Project Needs a name ofcourse
project(plasma-notasseguras)
 
# Find the required Libaries
find_package(KDE4 REQUIRED)
include(KDE4Defaults)

 
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${KDE4_INCLUDES}
   )
 

# We add our source code here
set(notasseguras_SRCS notasseguras.cpp simplecrypt.cpp textedit.cpp)
kde4_add_ui_files(notasseguras_SRCS config.ui)
 
# Now make sure all files get to the right place
kde4_add_plugin(plasma_applet_notasseguras ${notasseguras_SRCS})
target_link_libraries(plasma_applet_notasseguras 
                      ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS})
 
install(TARGETS plasma_applet_notasseguras
        DESTINATION ${PLUGIN_INSTALL_DIR})
 
install(FILES plasma-applet-notasseguras.desktop
        DESTINATION ${SERVICES_INSTALL_DIR})



add_subdirectory(iconos)

