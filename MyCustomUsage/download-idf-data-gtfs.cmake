include(ExternalProject)

set(IDF_GTFS_DESTINATION_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../DOWNLOADED_DATA/gtfs_idf")
set(IDF_GTFS_DOWNLOAD_URL "http://dataratp.download.opendatasoft.com/RATP_GTFS_FULL.zip")

message(STATUS "IDF GTFS download URL       = '${IDF_GTFS_DOWNLOAD_URL}'")
message(STATUS "IDF GTFS download directory = '${IDF_GTFS_DESTINATION_DIRECTORY}'")

# NOTE : it seems that if the target directory exists, the external project is not downloaded again

ExternalProject_Add(
    download_gtfs_idf
    PREFIX download_gtfs_idf
    URL "${IDF_GTFS_DOWNLOAD_URL}"
    DOWNLOAD_NAME "downloaded_gtfs_idf.zip"
    DOWNLOAD_DIR "${IDF_GTFS_DESTINATION_DIRECTORY}"
    SOURCE_DIR "${IDF_GTFS_DESTINATION_DIRECTORY}"
    # we only want to download the archive, thus we disable building :
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    )
