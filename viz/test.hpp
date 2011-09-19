#ifndef TEST_H__
#define TEST_H__

#include <QObject>
#include "StreamAlignerWidget.h"
#include <aggregator/StreamAlignerStatus.hpp>

class WidgetTester : public QObject {
    Q_OBJECT
public : 
    	WidgetTester(StreamAlignerWidget *saw) : saw(saw)
	{
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
	    sstatus.active = false;
	    status.streams.push_back(sstatus);
	    sstatus.active = true;
	    status.streams.push_back(sstatus);	    
	}

public slots:
    
	void nextVal() {
	    status.streams[2].buffer_fill++;
	    if(status.streams[2].buffer_fill > status.streams[2].buffer_size)
		status.streams[2].buffer_fill = 0;
	    
	    saw->updateData(status);
	};
private:
    StreamAlignerWidget *saw;
    aggregator::StreamAlignerStatus status;
};

#endif