
#include "StreamAlignerWidget.h"
#include "aggregator/StreamAlignerStatus.hpp"
#include <QApplication>
#include <QTimer>
#include "test.hpp"

int main(int argc, char **argv )
{
    QApplication app(argc, argv);
    StreamAlignerWidget widget(NULL);
    
    aggregator::StreamAlignerStatus status;
    status.samples_dropped_late_arriving = 57;    
    status.name = "TestTask";
    
    aggregator::StreamStatus sstatus;
    sstatus.active = true;
    sstatus.name = "Test";
    sstatus.buffer_size = 200;
    sstatus.buffer_fill = 87;
    sstatus.samples_dropped_buffer_full = 587;
    sstatus.samples_dropped_late_arriving = 317;
    
    status.streams.push_back(sstatus);
    status.streams.push_back(sstatus);
    status.streams.push_back(sstatus);
    

    widget.updateData(status);

    status.streams[0].buffer_fill = 2;
    status.name = "TestTask2";
    widget.updateData(status);
    
    widget.show();
    
    QTimer t;
    WidgetTester d(&widget);
    QObject::connect(&t, SIGNAL(timeout ()), &d, SLOT(nextVal()));
    
    t.start(100);
    
    return app.exec();
}