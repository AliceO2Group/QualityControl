#Inspired by AliceO2/ROOTMacros (to be merged when we merge the two repo)

Function(Format _output input prefix suffix)

    # DevNotes - input should be put in quotes or the complete list does not get passed to the function
    set(format)
    foreach (arg ${input})
        set(item ${arg})
        if (prefix)
            string(REGEX MATCH "^${prefix}" pre ${arg})
        endif (prefix)
        if (suffix)
            string(REGEX MATCH "${suffix}$" suf ${arg})
        endif (suffix)
        if (NOT pre)
            set(item "${prefix}${item}")
        endif (NOT pre)
        if (NOT suf)
            set(item "${item}${suffix}")
        endif (NOT suf)
        list(APPEND format ${item})
    endforeach (arg)
    set(${_output} ${format} PARENT_SCOPE)

endfunction(Format)

# Parameters :
#    - LINKDEF : path to linkdef
#    - LIBRARY_NAME : ?
#    - DICTIONARY : (optional) output dir
Macro(ROOT_GENERATE_DICTIONARY)

    # All Arguments needed for this new version of the macro are defined
    # in the parent scope, namely in the CMakeLists.txt of the submodule
    set(Int_LINKDEF ${LINKDEF})
    set(Int_DICTIONARY ${DICTIONARY})
    set(Int_LIB ${LIBRARY_NAME})

    Set(DictName "G__${Int_LIB}Dict.cxx")
    If (NOT Int_DICTIONARY)
        Set(Int_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/${DictName})
    EndIf (NOT Int_DICTIONARY)
    If (IS_ABSOLUTE ${Int_DICTIONARY})
        Set(Int_DICTIONARY ${Int_DICTIONARY})
    Else (IS_ABSOLUTE ${Int_DICTIONARY})
        Set(Int_DICTIONARY ${CMAKE_CURRENT_SOURCE_DIR}/${Int_DICTIONARY})
    EndIf (IS_ABSOLUTE ${Int_DICTIONARY})

        message("Int_LIB ${Int_LIB}")
        message("Int_LINKDEF ${Int_LINKDEF}")
        message("Int_DICTIONARY ${Int_DICTIONARY}")

    #  Message("DEFINITIONS: ${DEFINITIONS}")
    set(Int_INC ${INCLUDE_DIRECTORIES} ${SYSTEM_INCLUDE_DIRECTORIES})
    set(Int_HDRS ${HDRS})
    set(Int_DEF ${DEFINITIONS})

    # Convert the values of the variable to a semi-colon separated list
    separate_arguments(Int_INC)
    separate_arguments(Int_HDRS)
    separate_arguments(Int_DEF)

    # Format neccesary arguments
    # Add -I and -D to include directories and definitions
    Format(Int_INC "${Int_INC}" "-I" "")
    Format(Int_DEF "${Int_DEF}" "-D" "")

    #---call rootcint / cling --------------------------------
    set(OUTPUT_FILES ${Int_DICTIONARY})
    set(EXTRA_DICT_PARAMETERS "")
    set(Int_ROOTMAPFILE ${LIBRARY_OUTPUT_PATH}/lib${Int_LIB}.rootmap)
    set(Int_PCMFILE G__${Int_LIB}Dict_rdict.pcm)
    set(OUTPUT_FILES ${OUTPUT_FILES} ${Int_PCMFILE} ${Int_ROOTMAPFILE})
    set(EXTRA_DICT_PARAMETERS ${EXTRA_DICT_PARAMETERS}
            -inlineInputHeader -rmf ${Int_ROOTMAPFILE}
            -rml ${Int_LIB}${CMAKE_SHARED_LIBRARY_SUFFIX})
    set_source_files_properties(${OUTPUT_FILES} PROPERTIES GENERATED TRUE)
    If (CMAKE_SYSTEM_NAME MATCHES Linux)
        add_custom_command(OUTPUT ${OUTPUT_FILES}
                COMMAND LD_LIBRARY_PATH=${ROOT_LIBRARY_DIR}:${_intel_lib_dirs}:$ENV{LD_LIBRARY_PATH} ROOTSYS=${ROOTSYS} ${ROOT_CINT_EXECUTABLE} -f ${Int_DICTIONARY} ${EXTRA_DICT_PARAMETERS} -c ${Int_DEF} ${Int_INC} ${Int_HDRS} ${Int_LINKDEF}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/${Int_PCMFILE} ${LIBRARY_OUTPUT_PATH}/${Int_PCMFILE}
                DEPENDS ${Int_HDRS} ${Int_LINKDEF}
                )
        message("AAAAAAAAAAAAAA OUTPUT ${OUTPUT_FILES}
                COMMAND LD_LIBRARY_PATH=${ROOT_LIBRARY_DIR}:${_intel_lib_dirs}:$ENV{LD_LIBRARY_PATH} ROOTSYS=${ROOTSYS} ${ROOT_CINT_EXECUTABLE} -f ${Int_DICTIONARY} ${EXTRA_DICT_PARAMETERS} -c ${Int_DEF} ${Int_INC} ${Int_HDRS} ${Int_LINKDEF}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/${Int_PCMFILE} ${LIBRARY_OUTPUT_PATH}/${Int_PCMFILE}
                DEPENDS ${Int_HDRS} ${Int_LINKDEF}")
    Else (CMAKE_SYSTEM_NAME MATCHES Linux)
        If (CMAKE_SYSTEM_NAME MATCHES Darwin)
            add_custom_command(OUTPUT ${OUTPUT_FILES}
                    COMMAND DYLD_LIBRARY_PATH=${ROOT_LIBRARY_DIR}:$ENV{DYLD_LIBRARY_PATH} ROOTSYS=${ROOTSYS} ${ROOT_CINT_EXECUTABLE} -f ${Int_DICTIONARY} ${EXTRA_DICT_PARAMETERS} -c ${Int_DEF} ${Int_INC} ${Int_HDRS} ${Int_LINKDEF}
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/${Int_PCMFILE} ${LIBRARY_OUTPUT_PATH}/${Int_PCMFILE}
                    DEPENDS ${Int_HDRS} ${Int_LINKDEF}
                    )
        EndIf (CMAKE_SYSTEM_NAME MATCHES Darwin)
    EndIf (CMAKE_SYSTEM_NAME MATCHES Linux)
    install(FILES ${LIBRARY_OUTPUT_PATH}/${Int_PCMFILE} ${Int_ROOTMAPFILE} DESTINATION lib)


    if (CMAKE_COMPILER_IS_GNUCXX)
        exec_program(${CMAKE_C_COMPILER} ARGS "-dumpversion" OUTPUT_VARIABLE _gcc_version_info)
        string(REGEX REPLACE "^([0-9]+).*$" "\\1" GCC_MAJOR ${_gcc_version_info})
        if (${GCC_MAJOR} GREATER 4)
            set_source_files_properties(${Int_DICTIONARY} PROPERTIES COMPILE_DEFINITIONS R__ACCESS_IN_SYMBOL)
        endif ()
    endif ()

endmacro(ROOT_GENERATE_DICTIONARY)