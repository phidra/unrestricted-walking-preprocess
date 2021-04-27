include(FetchContent)

FetchContent_Declare(
    cppgtfs
    GIT_REPOSITORY "https://ad-git.informatik.uni-freiburg.de/ad/cppgtfs"
    GIT_TAG "master"
)


# Check if population has already been performed
FetchContent_GetProperties(cppgtfs)
if(NOT cppgtfs_POPULATED)
    # Fetch the content using previously declared details
    FetchContent_Populate(cppgtfs)

    # until I submit a PR with a fix, cppgtfs needs to be patched to rename pow10 :
    set(FILE_TO_PATCH "${cppgtfs_SOURCE_DIR}/src/ad/util/CsvParser.cpp")
    set(DESIRED_STRING "powersOf10")
    file(READ "${FILE_TO_PATCH}" TMP_CONTENT LIMIT 20000)
    string(FIND "${TMP_CONTENT}" "${DESIRED_STRING}" DESIRED_STRING_INDEX)
    if(${DESIRED_STRING_INDEX} EQUAL -1)
        message(STATUS "Patching file : ${FILE_TO_PATCH}")
        execute_process(COMMAND sed -i "s@pow10@${DESIRED_STRING}@g" "${FILE_TO_PATCH}")
    else()
        message(STATUS "No need to patch (already patched) file : ${FILE_TO_PATCH}")
    endif()


    # Bring the populated content into the build
    add_subdirectory(${cppgtfs_SOURCE_DIR} ${cppgtfs_BINARY_DIR})
endif()

set (CPPGTFS_INCLUDE_DIR "${cppgtfs_SOURCE_DIR}/src")
