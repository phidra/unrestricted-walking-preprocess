include(ExternalProject)

set(BORDEAUX_OSM_DESTINATION_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../DOWNLOADED_DATA/osm_bordeaux")
set(BORDEAUX_OSM_DOWNLOAD_URL "https://download.geofabrik.de/europe/france/aquitaine-latest.osm.pbf")

message(STATUS "Bordeaux OSM download URL       = '${BORDEAUX_OSM_DOWNLOAD_URL}'")
message(STATUS "Bordeaux OSM download directory = '${BORDEAUX_OSM_DESTINATION_DIRECTORY}'")

# NOTE : it seems that if the target directory exists, the external project is not downloaded again

ExternalProject_Add(
    download_osm_bordeaux
    PREFIX download_osm_bordeaux
    URL "${BORDEAUX_OSM_DOWNLOAD_URL}"
    DOWNLOAD_NO_EXTRACT TRUE
    DOWNLOAD_DIR "${BORDEAUX_OSM_DESTINATION_DIRECTORY}"
    SOURCE_DIR "${BORDEAUX_OSM_DESTINATION_DIRECTORY}"
    # we only want to download the archive, thus we disable building :
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    )

