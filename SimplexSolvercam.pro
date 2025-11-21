QT += core gui widgets network multimedia multimediawidgets

CONFIG += c++11

TARGET = SimplexGUI
TEMPLATE = app

SOURCES += \
    dualsimplexsolver.cpp \
    dynamicpolyhedronsolver.cpp \
    main.cpp \
    MainWindow.cpp \
    polyhedronsolver.cpp \
    realpolyhedronwidget.cpp \
    simplexsolver.cpp \
    visualizationdialog.cpp \
    ocrprocessor.cpp \
    cameracapture.cpp

HEADERS += \
    MainWindow.h \
    dualsimplexsolver.h \
    dynamicpolyhedronsolver.h \
    polyhedronsolver.h \
    realpolyhedronwidget.h \
    simplexsolver.h \
    visualizationdialog.h \
    ocrprocessor.h \
    cameracapture.h

# Add C++17 features if needed
CONFIG += c++17

# For better debugging
CONFIG += debug
CONFIG += console

# Network for HTTP requests
QT += network

target.path = $$[QT_INSTALL_EXAMPLES]/widgets/SimplexGUI
INSTALLS += target
