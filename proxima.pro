QT += widgets core gui

CONFIG += c++20

SOURCES += \
    source/core/components/display.cpp \
    source/core/components/frame.cpp \
    source/core/data/mediafile.cpp \
    source/core/data/mediamanager.cpp \
    source/core/processing/imageprocessor.cpp \
    source/core/processing/colorcorrection.cpp \
    source/core/processing/deconvolution.cpp \
    source/core/processing/wavelets.cpp \
    source/core/stacking/alignment.cpp \
    source/core/stacking/stacker.cpp \
    source/ui/dialogs/deconvolutiondialog/deconvolutiondialog.cpp \
    source/ui/dialogs/deconvolutiondialog/imageviewer.cpp \
    source/ui/dialogs/rgbaligndialog/rgbaligndialog.cpp \
    source/ui/widgets/homepage/homepage.cpp \
    source/ui/widgets/mainwindow/mainwindow.cpp \
    source/ui/widgets/processpage/processpage.cpp \
    source/ui/widgets/stackpage/stackpage.cpp \
    source/ui/widgets/stackpage/threads/sortingthread.cpp \
    source/ui/widgets/stackpage/threads/stackingthread.cpp \
    source/main.cpp

HEADERS += \
    source/core/components/display.h \
    source/core/components/frame.h \
    source/core/data/mediafile.h \
    source/core/data/mediamanager.h \
    source/core/processing/imageprocessor.h \
    source/core/processing/colorcorrection.h \
    source/core/processing/deconvolution.h \
    source/core/processing/wavelets.h \
    source/core/stacking/alignment.h \
    source/core/stacking/stacker.h \
    source/threading/thread.h \
    source/threading/threadpool.h \
    source/ui/dialogs/deconvolutiondialog/deconvolutiondialog.h \
    source/ui/dialogs/deconvolutiondialog/imageviewer.h \
    source/ui/dialogs/rgbaligndialog/rgbaligndialog.h \
    source/ui/widgets/homepage/homepage.h \
    source/ui/widgets/mainwindow/mainwindow.h \
    source/ui/widgets/processpage/processpage.h \
    source/ui/widgets/stackpage/stackpage.h \
    source/ui/widgets/stackpage/threads/sortingthread.h \
    source/ui/widgets/stackpage/threads/stackingthread.h

FORMS += \
    source/ui/dialogs/deconvolutiondialog/deconvolutiondialog.ui \
    source/ui/dialogs/rgbaligndialog/rgbaligndialog.ui \
    source/ui/widgets/homepage/homepage.ui \
    source/ui/widgets/mainwindow/mainwindow.ui \
    source/ui/widgets/processpage/processpage.ui \
    source/ui/widgets/stackpage/stackpage.ui \
    source/form.ui

INCLUDEPATH += \
    source \
    source/core \
    source/ui/widgets \
    source/ui/dialogs

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../libs/opencv/build/x64/vc16/lib/ -lopencv_world4110
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../libs/opencv/build/x64/vc16/lib/ -lopencv_world4110d
else:unix: LIBS += -L$$PWD/../../../../../libs/opencv/build/x64/vc16/lib/ -lopencv_world4110

INCLUDEPATH += $$PWD/../../../../../libs/opencv/build/include
DEPENDPATH += $$PWD/../../../../../libs/opencv/build/include

DISTFILES +=
