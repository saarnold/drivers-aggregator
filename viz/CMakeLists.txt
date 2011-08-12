rock_vizkit_widget(stream_aligner_widget
    SOURCES StreamAlignerWidget.cpp StreamAlignerWidgetPlugin.cpp
    HEADERS StreamAlignerWidget.h StreamAlignerWidgetPlugin.h
    MOC StreamAlignerWidget.h StreamAlignerWidgetPlugin.h
    DEPS_PKGCONFIG base-types QtCore QtGui
    )

rock_executable(testWidget 
		test.cpp
		DEPS stream_aligner_widget
		MOC test.hpp)