cmake_minimum_required(VERSION 2.8)

project(loadtest)

add_executable(loadtest 
  loadtest.cpp
  httpclientworker.cpp
  root_certificates.cpp
  ../uri.cpp
)
set_target_properties(loadtest PROPERTIES LINK_FLAGS_RELEASE "-dead_strip")

if(UNIX)
  set(PLATFORM_DEPENDENT_LIBRARIES, "-lpthread")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pthread")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -pthread")
else()
  set(PLATFORM_DEPENDENT_LIBRARIES, "")
endif()

add_custom_command(TARGET loadtest 
  POST_BUILD
  COMMAND strip loadtest)

add_custom_target(rootcerts 
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND ./cert/getcerts.py)

add_dependencies(loadtest rootcerts)

target_include_directories(loadtest
	PRIVATE ${PROJECT_INCLUDE_DIRS}
	PUBLIC ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

target_link_libraries(loadtest
	pwned
	${OPENSSL_CRYPTO_LIBRARY}
	${OPENSSL_SSL_LIBRARY}
	${Boost_LIBRARIES}
)
