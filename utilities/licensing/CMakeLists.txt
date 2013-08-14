project(licensing)

cmake_minimum_required(VERSION 2.6)

include(FindOpenSSL)

set(requiredlibs ${OPENSSL_LIBRARIES})

set(SOURCES
    src/LicenseCreator.cpp
)

ADD_EXECUTABLE(licensecreator ${SOURCES})
TARGET_LINK_LIBRARIES(licensecreator ${requiredlibs})
