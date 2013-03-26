install(FILES notasseguras.svg fondo.svg 
        DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/)

kde4_install_icons( ${ICON_INSTALL_DIR} )
kde4_update_iconcache()