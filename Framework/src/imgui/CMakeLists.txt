# Copyright CERN and copyright holders of ALICE O2. This software is
# distributed under the terms of the GNU General Public License v3 (GPL
# Version 3), copied verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/ for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization
# or submit itself to any jurisdiction.




if (GLFW_FOUND)
    message(STATUS "GLFW_FOUND " )
    set(MODULE_BUCKET_NAME glfw_bucket)
    set(GUI_BACKEND
            imgui_impl_glfw_gl3.cpp
            gl3w.c
#            src/DebugGUI.cxx
            imgui_widgets.cpp
            test/imgui_demo.cpp
            )
else()
    set(MODULE_BUCKET_NAME headless_bucket)
    set(GUI_BACKEND
#            src/DummyDebugGUI.cxx
            )
endif()

set(MODULE_NAME "DebugGUI")
O2_SETUP(NAME ${MODULE_NAME})

set(SRCS
        imgui.cpp
        imgui_draw.cpp
#        src/imgui_extras.cpp
        ${GUI_BACKEND}
        )

set(HEADERS
        imconfig.h
        imgui.h
#        imgui_extras.h
        )

set(LIBRARY_NAME ${MODULE_NAME})
set(BUCKET_NAME ${MODULE_BUCKET_NAME})

O2_GENERATE_LIBRARY()

# TODO: feature of macro, it deletes the variables we pass to it, set them again
# this has to be fixed in the macro implementation
#set(LIBRARY_NAME ${MODULE_NAME})
#set(BUCKET_NAME ${MODULE_BUCKET_NAME})

#if (GLFW_FOUND)
#    O2_GENERATE_EXECUTABLE(
#            EXE_NAME "test_ImGUI"
#            SOURCES test/test_ImGUI.cpp test/imgui_demo.cpp
#            MODULE_LIBRARY_NAME ${LIBRARY_NAME}
#            BUCKET_NAME ${MODULE_BUCKET_NAME}
#    )
#endif()

target_compile_options(DebugGUI PUBLIC -O0 -g -fno-omit-frame-pointer)
