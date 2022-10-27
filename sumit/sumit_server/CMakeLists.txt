#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>

set(SERVER_SOURCES 
    Logger.java
    OrphanHandler.java
    RequestHandler.java
    Session.java
    Sessions.java
    UWECellSuppressionServer.java)

# ##############################################################################
# target: cell_suppression_server - stand-alone jvm  execution
# ##############################################################################

add_jar(cell_suppression_server ${SERVER_SOURCES})

file(COPY __init__.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
