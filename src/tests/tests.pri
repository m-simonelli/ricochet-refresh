TEMPLATE = app
QT += testlib core network quick
CONFIG -= app_bundle
CONFIG += testcase

INCLUDEPATH +=\
    $${PWD}/../libtego/source\

QMAKE_INCLUDES = $${PWD}/../qmake_includes

include($${QMAKE_INCLUDES}/artifacts.pri)
include($${QMAKE_INCLUDES}/compiler_flags.pri)
include($${QMAKE_INCLUDES}/linker_flags.pri)

include($${PWD}/../libtego_ui/libtego_ui.pri)
include($${PWD}/../libtego/libtego.pri)

# puts all tests into one directory so they can be run with a script
DESTDIR = $${PWD}/../../build/tests/