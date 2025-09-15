cmake_minimum_required(VERSION 3.14)

include(FetchContent)

project(
  TrigaMirror
  VERSION 0.1.0
  DESCRIPTION "Get the flux data of TrigaServer and share in network."
  HOMEPAGE_URL "https://github.com/TRIGA-IPR-R1/TrigaMirror"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0167 NEW)

find_package(PkgConfig REQUIRED)
find_package(Boost 1.81 REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(Crow REQUIRED)


pkg_search_module(SYSTEMD "systemd")
pkg_search_module(cxxopts REQUIRED IMPORTED_TARGET cxxopts)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES
    ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES})

set(TRIGAMIRROR_SRC ${PROJECT_SOURCE_DIR}/src/main.cpp
                    ${PROJECT_SOURCE_DIR}/src/TrigaMirror.cpp)

add_executable(trigamirror ${TRIGAMIRROR_SRC})

target_include_directories(
  trigamirror
  PRIVATE ${PROJECT_SOURCE_DIR}/include ${Boost_INCLUDE_DIRS})

target_link_libraries(
  trigamirror
  PRIVATE ${Boost_LIBRARIES}
          PkgConfig::cxxopts
          OpenSSL::SSL 
          OpenSSL::Crypto
  PUBLIC Crow::Crow)


target_compile_options(
  trigamirror
  PRIVATE ${CXXOPTS_CFLAGS_OTHER})

install(TARGETS trigamirror)
if("${DONT_INSTALL_CONF}" STREQUAL "")
  install(FILES ${PROJECT_SOURCE_DIR}/resources/trigamirror-EXEMPLE.conf
          DESTINATION /etc/trigamirror/)
endif()

if(SYSTEMD_FOUND)
  if("${SYSTEMD_SERVICES_INSTALL_DIR}" STREQUAL "")
    execute_process(
      COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=systemdsystemunitdir systemd
      OUTPUT_VARIABLE SYSTEMD_SERVICES_INSTALL_DIR)
    string(REGEX REPLACE "[ \t\n]+" "" SYSTEMD_SERVICES_INSTALL_DIR
                         "${SYSTEMD_SERVICES_INSTALL_DIR}")
  endif()
  configure_file(${PROJECT_SOURCE_DIR}/resources/trigamirror@.service
                 ${CMAKE_BINARY_DIR}/trigamirror@.service)
  install(FILES ${CMAKE_BINARY_DIR}/trigamirror@.service
          DESTINATION ${SYSTEMD_SERVICES_INSTALL_DIR})
endif()
