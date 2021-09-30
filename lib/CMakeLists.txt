SET(LIBBLOT_SRCS
        blot_braille.c
        blot_canvas.c
        blot_figure.c
        blot_layer.c
        blot_screen.c
        blot_utils.c
        blot_terminal.c
)

ADD_LIBRARY(blot STATIC
        ${LIBBLOT_SRCS}
)

set_property(TARGET blot PROPERTY POSITION_INDEPENDENT_CODE ON)

#TARGET_COMPILE_DEFINITIONS(blot
#        PUBLIC var="val"
#)

TARGET_INCLUDE_DIRECTORIES(blot INTERFACE
        ${CMAKE_SOURCE_DIR}/blot/include
)

TARGET_INCLUDE_DIRECTORIES(blot PUBLIC
        ${GLIB2_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/thriver/include
)

TARGET_INCLUDE_DIRECTORIES(blot PRIVATE
        ${CMAKE_SOURCE_DIR}/blot/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${GIO_INCLUDE_DIRS}
        ${GIOUNIX_INCLUDE_DIRS}
)

TARGET_LINK_LIBRARIES(blot
        ${GLIB2_LIBRARIES}
        ${GIO_LIBRARIES}
        ${GIOUNIX_LIBRARIES}
)
