include(ExternalProject)

set(BORDEAUX_GTFS_DESTINATION_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../DOWNLOADED_DATA/gtfs_bordeaux")
set(BORDEAUX_GTFS_DOWNLOAD_URL "https://www.data.gouv.fr/fr/datasets/r/13e7e219-b037-4d60-a3ab-e55d2d3e5291")

message(STATUS "Bordeaux GTFS download URL       = '${BORDEAUX_GTFS_DOWNLOAD_URL}'")
message(STATUS "Bordeaux GTFS download directory = '${BORDEAUX_GTFS_DESTINATION_DIRECTORY}'")

# NOTE : it seems that if the target directory exists, the external project is not downloaded again

ExternalProject_Add(
    download_gtfs_bordeaux
    PREFIX download_gtfs_bordeaux
    URL "${BORDEAUX_GTFS_DOWNLOAD_URL}"
    DOWNLOAD_NAME "downloaded_gtfs_bordeaux.zip"
    DOWNLOAD_DIR "${BORDEAUX_GTFS_DESTINATION_DIRECTORY}"
    SOURCE_DIR "${BORDEAUX_GTFS_DESTINATION_DIRECTORY}"
    # we only want to download the archive, thus we disable building :
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    )

