QT += widgets core gui

CONFIG += c++17

SOURCES += \
    source/components/alignmentpoint.cpp \
    source/components/display.cpp \
    source/components/frame.cpp \
    source/components/mediafile.cpp \
    source/concurrency/threadpool.cpp \
    source/main.cpp \
    source/mainwindow/mainwindow.cpp \
    source/modules/stacker.cpp \
    source/pages/homepage/homepage.cpp \
    source/pages/processpage/processpage.cpp \
    source/pages/stackpage/stackpage.cpp

HEADERS += \
    source/components/alignmentpoint.h \
    source/components/display.h \
    source/components/frame.h \
    source/components/helpers.h \
    source/components/mediafile.h \
    source/concurrency/threadpool.h \
    source/mainwindow/mainwindow.h \
    source/modules/stacker.h \
    source/pages/homepage/homepage.h \
    source/pages/processpage/processpage.h\
    source/pages/stackpage/stackpage.h

FORMS += \
    source/mainwindow/mainwindow.ui \
    source/pages/homepage/homepage.ui \
    source/pages/processpage/processpage.ui \
    source/pages/stackpage/stackpage.ui

INCLUDEPATH += \
    source \
    source/pages

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../libs/opencv/build/x64/vc16/lib/ -lopencv_world4110
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../libs/opencv/build/x64/vc16/lib/ -lopencv_world4110d
else:unix: LIBS += -L$$PWD/../../../../../libs/opencv/build/x64/vc16/lib/ -lopencv_world4110

INCLUDEPATH += $$PWD/../../../../../libs/opencv/build/include
DEPENDPATH += $$PWD/../../../../../libs/opencv/build/include
