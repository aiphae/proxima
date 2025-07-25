QT += widgets core gui

CONFIG += c++20

SOURCES += \
    source/core/components/display.cpp \
    source/core/components/frame.cpp \
    source/core/data/media_file.cpp \
    source/core/data/media_manager.cpp \
    source/core/processing/color_correction.cpp \
    source/core/processing/deconvolution.cpp \
    source/core/processing/image_processor.cpp \
    source/core/processing/wavelets.cpp \
    source/core/stacking/alignment.cpp \
    source/core/stacking/stacker.cpp \
    source/ui/dialogs/deconvolution_dialog/deconvolution_dialog.cpp \
    source/ui/dialogs/deconvolution_dialog/image_viewer.cpp \
    source/ui/dialogs/rgb_align_dialog/rgb_align_dialog.cpp \
    source/ui/widgets/home_page/home_page.cpp \
    source/ui/widgets/main_window/main_window.cpp \
    source/ui/widgets/process_page/process_page.cpp \
    source/main.cpp \
    source/ui/widgets/stack_page/stack_page.cpp \
    source/ui/widgets/stack_page/threads/sort_thread.cpp \
    source/ui/widgets/stack_page/threads/stack_thread.cpp \
    source/ui/workspace.cpp

HEADERS += \
    source/core/components/display.h \
    source/core/components/frame.h \
    source/core/data/media_file.h \
    source/core/data/media_manager.h \
    source/core/processing/color_correction.h \
    source/core/processing/deconvolution.h \
    source/core/processing/image_processor.h \
    source/core/processing/wavelets.h \
    source/core/stacking/alignment.h \
    source/core/stacking/stacker.h \
    source/threading/thread.h \
    source/threading/thread_pool.h \
    source/ui/dialogs/deconvolution_dialog/deconvolution_dialog.h \
    source/ui/dialogs/deconvolution_dialog/image_viewer.h \
    source/ui/dialogs/rgb_align_dialog/rgb_align_dialog.h \
    source/ui/widgets/home_page/home_page.h \
    source/ui/widgets/main_window/main_window.h \
    source/ui/widgets/process_page/process_page.h \
    source/ui/widgets/stack_page/stack_page.h \
    source/ui/widgets/stack_page/threads/sort_thread.h \
    source/ui/widgets/stack_page/threads/stack_thread.h \
    source/ui/workspace.h

FORMS += \
    source/ui/dialogs/deconvolution_dialog/deconvolution_dialog.ui \
    source/ui/dialogs/rgb_align_dialog/rgb_align_dialog.ui \
    source/ui/widgets/home_page/home_page.ui \
    source/ui/widgets/main_window/main_window.ui \
    source/ui/widgets/process_page/process_page.ui \
    source/form.ui \
    source/ui/widgets/stack_page/stack_page.ui

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
