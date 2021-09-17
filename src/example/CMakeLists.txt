add_executable(rest_test rest_test.cc)
target_link_libraries(rest_test ssl crypto rest util ${Boost_LIBRARIES})

add_executable(bitstamp_ws_test bitstamp_ws_test.cc)
target_link_libraries(bitstamp_ws_test ws ssl crypto util ${Boost_LIBRARIES})

add_executable(ws_test ws_test.cc)
target_link_libraries(ws_test ssl crypto ws util ${Boost_LIBRARIES})
