QT += widgets core gui

CONFIG += c++20

SOURCES += \
    source/components/display.cpp \
    source/components/frame.cpp \
    source/main.cpp \
    source/data/mediafile.cpp \
    source/data/mediamanager.cpp \
    source/stacking/alignment.cpp \
    source/stacking/sortingthread.cpp \
    source/stacking/stacker.cpp \
    source/stacking/stackingthread.cpp \
    source/widgets/homepage/homepage.cpp \
    source/widgets/mainwindow/mainwindow.cpp \
    source/widgets/processpage/processpage.cpp \
    source/widgets/stackpage/stackpage.cpp

HEADERS += \
    source/components/display.h \
    source/components/frame.h \
    source/stacking/alignment.h \
    source/stacking/sortingthread.h \
    source/stacking/stackingthread.h \
    source/concurrency/thread.h \
    source/concurrency/threadpool.h \
    source/data/mediafile.h \
    source/data/mediamanager.h \
    source/stacking/stacker.h \
    source/widgets/homepage/homepage.h \
    source/widgets/mainwindow/mainwindow.h \
    source/widgets/processpage/processpage.h \
    source/widgets/stackpage/stackpage.h

FORMS += \
    source/widgets/homepage/homepage.ui \
    source/widgets/mainwindow/mainwindow.ui \
    source/widgets/processpage/processpage.ui \
    source/widgets/stackpage/stackpage.ui

INCLUDEPATH += \
    source \
    source/widgets

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../libs/opencv/build/x64/vc16/lib/ -lopencv_world4110
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../libs/opencv/build/x64/vc16/lib/ -lopencv_world4110d
else:unix: LIBS += -L$$PWD/../../../../../libs/opencv/build/x64/vc16/lib/ -lopencv_world4110

INCLUDEPATH += $$PWD/../../../../../libs/opencv/build/include
DEPENDPATH += $$PWD/../../../../../libs/opencv/build/include
