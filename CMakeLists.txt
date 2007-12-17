# project name
PROJECT( metil )

# test
# SET( CTEST_COMMAND "/usr/local/bin/ctest -D Experimental" )
# SET (DROP_SITE "localhost")
# SET (DROP_LOCATION "/cgi-bin/UploadDart.cgi")
# SET (DROP_SITE_USER "anonymous")
# SET (DROP_SITE_PASSWORD "xxx")
# SET (TRIGGER_SITE "http://${DROP_SITE}/cgi-bin/TriggerDart.cgi")
# INCLUDE( Dart )

# add definitions, compiler switches, etc.
ADD_DEFINITIONS( -Wall )

# global variables
SET( CMAKE_METIL_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/lib/metil )
SET( METIL_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/CPP/metil ) # TODO -> find it from ADD_SUBDIRECTORY( CPP )

# subdir
ADD_SUBDIRECTORY( CPP )
ADD_SUBDIRECTORY( MET )
ADD_SUBDIRECTORY( TESTS )

INSTALL( FILES met.xml DESTINATION ~/.kde/share/apps/katepart/syntax/met.xml )
