cmake_minimum_required(VERSION 2.8)

project(pwned-index)

add_executable(pwned-index index.cpp)
set_target_properties(pwned-index PROPERTIES LINK_FLAGS_RELEASE "-dead_strip")

target_include_directories(pwned-index
	PRIVATE ${PROJECT_INCLUDE_DIRS}
	PUBLIC ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(pwned-index
  pwned
  ${OPENSSL_CRYPTO_LIBRARY}
  ${PLATFORM_DEPENDENT_LIBRARIES}
  ${Boost_LIBRARIES}
)

add_custom_command(TARGET pwned-index
  POST_BUILD
  COMMAND strip pwned-index)

install(TARGETS pwned-index RUNTIME DESTINATION bin)
