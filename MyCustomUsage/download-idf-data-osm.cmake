include(ExternalProject)

set(IDF_OSM_DESTINATION_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../DOWNLOADED_DATA/osm_idf")
set(IDF_OSM_DOWNLOAD_URL "http://download.geofabrik.de/europe/france/ile-de-france-latest.osm.pbf")

message(STATUS "IDF OSM download URL       = '${IDF_OSM_DOWNLOAD_URL}'")
message(STATUS "IDF OSM download directory = '${IDF_OSM_DESTINATION_DIRECTORY}'")

# NOTE : it seems that if the target directory exists, the external project is not downloaded again

ExternalProject_Add(
    download_osm_idf
    PREFIX download_osm_idf
    URL "${IDF_OSM_DOWNLOAD_URL}"
    DOWNLOAD_NO_EXTRACT TRUE
    DOWNLOAD_DIR "${IDF_OSM_DESTINATION_DIRECTORY}"
    SOURCE_DIR "${IDF_OSM_DESTINATION_DIRECTORY}"
    # we only want to download the archive, thus we disable building :
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    )
