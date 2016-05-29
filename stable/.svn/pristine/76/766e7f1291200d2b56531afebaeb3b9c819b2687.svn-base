set(SRC_LIST dbg.c framework.c hashtable.c linkedlist.c main.c shortest_path.c)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/sbin)

add_executable(analyse ${SRC_LIST})
target_link_libraries(analyse ${CMAKE_DL_LIBS})

install(TARGETS analyse
    RUNTIME DESTINATION /usr/local/detectorig/sbin
    )
