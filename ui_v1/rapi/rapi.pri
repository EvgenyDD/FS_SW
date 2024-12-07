HEADERS +=  $$RAPI_PATH/rapi.h \
            $$RAPI_PATH/rapi_version.h

HEADERS +=  $$RAPI_PATH/interface/abstract_interface_provider.h \
            $$RAPI_PATH/interface/abstract_interface.h \
            $$RAPI_PATH/interface/interface_collector.h \
            $$RAPI_PATH/interface/serial_port/serial_interface.h \
            $$RAPI_PATH/interface/serial_port/serial_interface_provider.h

HEADERS +=  $$RAPI_PATH/parsers/abstract_parser.h

HEADERS +=  $$RAPI_PATH/app/device_manager.h

HEADERS +=  $$RAPI_PATH/tools/rapi_names.hpp \
            $$RAPI_PATH/tools/concurrent_notifier.h \
            $$RAPI_PATH/tools/basic_thread.h \
            $$RAPI_PATH/tools/date.h \
            $$RAPI_PATH/tools/debug_tools.h \
            $$RAPI_PATH/tools/helper.h \
            $$RAPI_PATH/tools/logger.h \
            $$RAPI_PATH/tools/queue_threadsafe.h \
            $$RAPI_PATH/tools/thread_pool.h \
            $$RAPI_PATH/tools/event_counter.h

# Serial interface
HEADERS +=  $$RAPI_PATH/interface/serial_port/serial.hpp \
            $$RAPI_PATH/interface/serial_port/impl.hpp

SOURCES +=  $$RAPI_PATH/interface/serial_port/serial.cpp \
            $$RAPI_PATH/interface/serial_port/impl.cpp

INCLUDEPATH += $$RAPI_PATH


# UDP interface

DEFINES += UNICODE
DEFINES += _UNICODE

QMAKE_CXXFLAGS += -Wno-padded

win32:LIBS += -lhid -lsetupapi -lws2_32
win32:DEFINES += WIN32
