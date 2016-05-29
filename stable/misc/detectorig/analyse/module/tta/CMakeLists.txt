set(SRC_LIST mod_tta.c)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/module)
include_directories(${PROJECT_SOURCE_DIR}/src)

ADD_LIBRARY(tta SHARED ${SRC_LIST})
TARGET_LINK_LIBRARIES(tta m)

SET_TARGET_PROPERTIES(tta
    PROPERTIES
    PREFIX ${MOD_PREFIX}
    )
install(TARGETS tta
    LIBRARY DESTINATION /usr/local/detectorig/module
    )
