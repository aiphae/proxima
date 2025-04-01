QT += widgets core gui

CONFIG += c++17

SOURCES += \
    source/components/display.cpp \
    source/components/mediafile.cpp \
    source/main.cpp \
    source/mainwindow/mainwindow.cpp \
    source/widgets/alignpage/alignpage.cpp \
    source/widgets/homepage/homepage.cpp \
    source/widgets/processpage/processpage.cpp \
    source/widgets/stackpage/stackpage.cpp

HEADERS += \
    source/components/display.h \
    source/components/mediafile.h \
    source/mainwindow/mainwindow.h \
    source/widgets/alignpage/alignpage.h \
    source/widgets/homepage/homepage.h \
    source/widgets/processpage/processpage.h\
    source/widgets/stackpage/stackpage.h

FORMS += \
    source/mainwindow/mainwindow.ui \
    source/widgets/alignpage/alignpage.ui \
    source/widgets/homepage/homepage.ui \
    source/widgets/processpage/processpage.ui \
    source/widgets/stackpage/stackpage.ui

INCLUDEPATH += \
    source/mainwindow \
    source/widgets/ \
    source/components

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../libs/opencv/build/x64/vc16/lib/ -lopencv_world4110
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../libs/opencv/build/x64/vc16/lib/ -lopencv_world4110d
else:unix: LIBS += -L$$PWD/../../../../../libs/opencv/build/x64/vc16/lib/ -lopencv_world4110

INCLUDEPATH += $$PWD/../../../../../libs/opencv/build/include
DEPENDPATH += $$PWD/../../../../../libs/opencv/build/include
