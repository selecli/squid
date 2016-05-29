set(SRC_LIST detect_core.c detect_epoll.c detect_global.c detect_misc.c detect_param_conf.c detect_write_anyhost.c event.c log.c main.c mempool.c)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/module)
include_directories(${PROJECT_SOURCE_DIR}/src)

ADD_LIBRARY(fc SHARED ${SRC_LIST})
TARGET_LINK_LIBRARIES(fc m)

SET_TARGET_PROPERTIES(fc
    PROPERTIES
    PREFIX ${MOD_PREFIX}
    )
install(TARGETS fc
    LIBRARY DESTINATION /usr/local/detectorig/module
    )
