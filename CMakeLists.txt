set (Tutorial_VERSION_MAJOR 1)
set (Tutorial_VERSION_MINOR 0)

project (PEG)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra")

# boost
# set(BOOST_ROOT /specific/boost/path)
find_package(Boost COMPONENTS system thread chrono date_time atomic REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

#crow and nt partition
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/partition)

# antlr3
include_directories(${PROJECT_SOURCE_DIR}/queryDecompose/tools/libantlr3c-3.4)
include_directories(${PROJECT_SOURCE_DIR}/queryDecompose/tools/libantlr3c-3.4/include)
link_directories(${PROJECT_SOURCE_DIR}/queryDecompose/lib)

aux_source_directory(gStoreAPI gstoreapiSRC)
aux_source_directory(queryDecompose/Parser parserSRC)
aux_source_directory(queryDecompose/Query querySRC)
aux_source_directory(queryDecompose/Util utilSRC)
aux_source_directory(joiner joinerSRC)

# server
add_executable(PEG_Server PEG_Server.cpp ${gstoreapiSRC} ${parserSRC} ${querySRC} ${utilSRC} ${joinerSRC})
target_link_libraries(PEG_Server pthread)
target_link_libraries(PEG_Server ${Boost_LIBRARIES})
target_link_libraries(PEG_Server curl)
target_link_libraries(PEG_Server libantlr.a)

# load
add_executable(PEG_Load PEG_Load.cpp ${gstoreapiSRC} ${parserSRC} ${querySRC} ${utilSRC} ${joinerSRC})
target_link_libraries(PEG_Load pthread)
target_link_libraries(PEG_Load ${Boost_LIBRARIES})
target_link_libraries(PEG_Load curl)
target_link_libraries(PEG_Load libantlr.a)

# query
add_executable(PEG_Query PEG_Query.cpp ${gstoreapiSRC} ${parserSRC} ${querySRC} ${utilSRC} ${joinerSRC})
target_link_libraries(PEG_Query pthread)
target_link_libraries(PEG_Query ${Boost_LIBRARIES})
target_link_libraries(PEG_Query curl)
target_link_libraries(PEG_Query libantlr.a)

# delete
add_executable(PEG_Delete PEG_Delete.cpp ${gstoreapiSRC} ${parserSRC} ${querySRC} ${utilSRC} ${joinerSRC})
target_link_libraries(PEG_Delete pthread)
target_link_libraries(PEG_Delete ${Boost_LIBRARIES})
target_link_libraries(PEG_Delete curl)
target_link_libraries(PEG_Delete libantlr.a)

# api
add_executable(PEG_Service PEG_Service.cpp ${gstoreapiSRC} ${parserSRC} ${querySRC} ${utilSRC} ${joinerSRC})
target_link_libraries(PEG_Service pthread)
target_link_libraries(PEG_Service ${Boost_LIBRARIES})
target_link_libraries(PEG_Service curl)
target_link_libraries(PEG_Service libantlr.a)