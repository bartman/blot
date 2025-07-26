SET(LIBBLOT_SRCS
    blot_axis.c
    blot_braille.c
    blot_canvas.c
    blot_color.c
    blot_figure.c
    blot_layer.c
    blot_screen.c
    blot_utils.c
    blot_terminal.c
)

# we need glib

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(GLIB2 REQUIRED glib-2.0>=2.44)

# build a libblot.so and a libblot.a

ADD_LIBRARY(blot_a STATIC ${LIBBLOT_SRCS})
ADD_LIBRARY(blot_so SHARED ${LIBBLOT_SRCS})

# each is configured exactly the same way

foreach(blot blot_a blot_so)

    SET_TARGET_PROPERTIES(${blot} PROPERTIES OUTPUT_NAME blot)

    SET_PROPERTY(TARGET ${blot} PROPERTY POSITION_INDEPENDENT_CODE ON)

    TARGET_INCLUDE_DIRECTORIES(${blot} INTERFACE
        ${CMAKE_SOURCE_DIR}/blot/include
    )

    TARGET_INCLUDE_DIRECTORIES(${blot} PUBLIC
        ${GLIB2_INCLUDE_DIRS}
    )

    TARGET_INCLUDE_DIRECTORIES(${blot} PRIVATE
        ${CMAKE_SOURCE_DIR}/blot/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    TARGET_LINK_DIRECTORIES(${blot} PUBLIC
        ${GLIB2_LIBRARY_DIRS}
    )

    TARGET_LINK_LIBRARIES(${blot}
        ${GLIB2_LIBRARIES}
    )

endforeach(blot)
