if(NOT DEFINED MANIFEST OR NOT DEFINED README OR NOT DEFINED RUNNER OR NOT DEFINED SOURCE_ROOT)
    message(FATAL_ERROR "MANIFEST, README, RUNNER, and SOURCE_ROOT are required")
endif()

file(READ "${MANIFEST}" MANIFEST_CONTENTS)
file(READ "${README}" README_CONTENTS)
file(READ "${RUNNER}" RUNNER_CONTENTS)

set(START_MARKER "<!-- public-simulation-catalog:start -->")
set(END_MARKER "<!-- public-simulation-catalog:end -->")
string(FIND "${README_CONTENTS}" "${START_MARKER}" START_INDEX)
string(FIND "${README_CONTENTS}" "${END_MARKER}" END_INDEX)

if(START_INDEX EQUAL -1 OR END_INDEX EQUAL -1 OR END_INDEX LESS START_INDEX)
    message(FATAL_ERROR "README public simulation catalog markers are missing or invalid")
endif()

string(LENGTH "${START_MARKER}" START_MARKER_LENGTH)
math(EXPR CATALOG_START "${START_INDEX} + ${START_MARKER_LENGTH}")
math(EXPR CATALOG_LENGTH "${END_INDEX} - ${CATALOG_START}")
string(SUBSTRING "${README_CONTENTS}" ${CATALOG_START} ${CATALOG_LENGTH} CATALOG_CONTENTS)

string(REGEX MATCHALL "hybridapi/src/[A-Za-z0-9_/]+\\.cpp" MANIFEST_SOURCES "${MANIFEST_CONTENTS}")
list(REMOVE_DUPLICATES MANIFEST_SOURCES)
list(REMOVE_ITEM MANIFEST_SOURCES "hybridapi/src/labsland/simulations/targetdevice.cpp")

string(REGEX MATCHALL "\\(src/[A-Za-z0-9_/]+\\.cpp\\)" DOCUMENTED_SOURCE_LINKS "${CATALOG_CONTENTS}")
set(DOCUMENTED_SOURCES "")
foreach(SOURCE_LINK IN LISTS DOCUMENTED_SOURCE_LINKS)
    string(REGEX REPLACE "^\\(|\\)$" "" DOCUMENTED_SOURCE "${SOURCE_LINK}")
    list(APPEND DOCUMENTED_SOURCES "${DOCUMENTED_SOURCE}")
endforeach()

list(LENGTH DOCUMENTED_SOURCES DOCUMENTED_SOURCE_COUNT)
set(UNIQUE_DOCUMENTED_SOURCES ${DOCUMENTED_SOURCES})
list(REMOVE_DUPLICATES UNIQUE_DOCUMENTED_SOURCES)
list(LENGTH UNIQUE_DOCUMENTED_SOURCES UNIQUE_DOCUMENTED_SOURCE_COUNT)
if(NOT DOCUMENTED_SOURCE_COUNT EQUAL UNIQUE_DOCUMENTED_SOURCE_COUNT)
    message(FATAL_ERROR "README public simulation catalog contains duplicate implementation links")
endif()

list(LENGTH MANIFEST_SOURCES MANIFEST_SOURCE_COUNT)
if(NOT DOCUMENTED_SOURCE_COUNT EQUAL MANIFEST_SOURCE_COUNT)
    message(FATAL_ERROR
        "README public simulation catalog has ${DOCUMENTED_SOURCE_COUNT} implementations; "
        "simulations.cmake has ${MANIFEST_SOURCE_COUNT}"
    )
endif()

string(REGEX MATCHALL "\\| `[A-Za-z0-9-]+` \\|" CATALOG_ROWS "${CATALOG_CONTENTS}")
list(LENGTH CATALOG_ROWS CATALOG_ROW_COUNT)
if(NOT CATALOG_ROW_COUNT EQUAL MANIFEST_SOURCE_COUNT)
    message(FATAL_ERROR
        "README public simulation catalog has ${CATALOG_ROW_COUNT} rows; "
        "simulations.cmake has ${MANIFEST_SOURCE_COUNT} implementations"
    )
endif()

foreach(CATALOG_ROW IN LISTS CATALOG_ROWS)
    string(REGEX REPLACE "^\\| `([A-Za-z0-9-]+)` \\|$" "\\1" RUNNER_ID "${CATALOG_ROW}")
    string(FIND "${RUNNER_CONTENTS}" "simulation == \"${RUNNER_ID}\"" RUNNER_INDEX)
    if(RUNNER_INDEX EQUAL -1)
        message(FATAL_ERROR "README catalog ID ${RUNNER_ID} is not handled by the public runner")
    endif()
endforeach()

foreach(MANIFEST_SOURCE IN LISTS MANIFEST_SOURCES)
    string(REPLACE "hybridapi/" "" PUBLIC_SOURCE "${MANIFEST_SOURCE}")
    list(FIND DOCUMENTED_SOURCES "${PUBLIC_SOURCE}" DOCUMENTED_INDEX)
    if(DOCUMENTED_INDEX EQUAL -1)
        message(FATAL_ERROR "README catalog is missing public implementation ${PUBLIC_SOURCE}")
    endif()
endforeach()

foreach(DOCUMENTED_SOURCE IN LISTS DOCUMENTED_SOURCES)
    string(FIND "${MANIFEST_CONTENTS}" "hybridapi/${DOCUMENTED_SOURCE}" MANIFEST_INDEX)
    if(MANIFEST_INDEX EQUAL -1)
        message(FATAL_ERROR
            "README catalog documents ${DOCUMENTED_SOURCE}, which is not in simulations.cmake"
        )
    endif()
endforeach()

string(REGEX MATCHALL
    "\\(server/simulations/[A-Za-z0-9_.-]+\\.yml\\)"
    DOCUMENTED_CONFIG_LINKS
    "${CATALOG_CONTENTS}"
)
foreach(CONFIG_LINK IN LISTS DOCUMENTED_CONFIG_LINKS)
    string(REGEX REPLACE "^\\(|\\)$" "" CONFIG_PATH "${CONFIG_LINK}")
    if(NOT EXISTS "${SOURCE_ROOT}/${CONFIG_PATH}")
        message(FATAL_ERROR "README catalog links to missing configuration ${CONFIG_PATH}")
    endif()
endforeach()
