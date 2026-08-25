file(READ "${MANIFEST}" MANIFEST_CONTENTS)
string(FIND "${MANIFEST_CONTENTS}" "hybridapi/src/rhlab/airportWindStation.cpp" SOURCE_INDEX)

if(SOURCE_INDEX EQUAL -1)
    message(FATAL_ERROR "airportWindStation.cpp is missing from the embedded simulation manifest")
endif()
