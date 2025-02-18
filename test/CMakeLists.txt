cmake_minimum_required(VERSION 3.11)
find_package(GTest REQUIRED)

add_executable(test_timerservice "test_timerservice.cpp")
target_link_libraries(test_timerservice miningsim GTest::gtest GTest::gtest_main)
target_include_directories(test_timerservice
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../src
)

add_executable(test_stations "test_stations.cpp")
target_link_libraries(test_stations miningsim GTest::gtest GTest::gtest_main)
target_include_directories(test_stations
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../src
)

add_executable(test_trucks "test_trucks.cpp")
target_link_libraries(test_trucks miningsim GTest::gtest GTest::gtest_main)
target_include_directories(test_trucks
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../src
)

