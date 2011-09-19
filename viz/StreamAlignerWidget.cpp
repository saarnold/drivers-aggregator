#include "StreamAlignerWidget.h"
#include "aggregator/StreamAlignerStatus.hpp"
#include <QStandardItemModel>
#include <QTreeView>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QAbstractItemModel>
#include <stdexcept>

enum CustomRoles {
    BufferSize = Qt::UserRole + 2,
    BufferFill
};

class StreamRepresentation {
public:
    StreamRepresentation(const aggregator::StreamStatus &status)
    {	
	name = QString::fromStdString(status.name);
    };
    
    void updateData(const aggregator::StreamStatus &status)
    {
	bufferStatusSize = QVariant::fromValue<int>(status.buffer_size);
	bufferStatusFill = QVariant::fromValue<int>(status.buffer_fill);
	droppedFull= QString("%0").arg(status.samples_dropped_buffer_full);
	droppedLate= QString("%0").arg(status.samples_dropped_late_arriving);
    }
    
    QVariant data(int col, int role)
    {
	if(role == Qt::DisplayRole || col == 1) 
	{
	    switch(col)
	    {
		case 0:
		    return name;
		    break;
		case 1:
		    if(role == BufferSize)
			return bufferStatusSize;
		    if(role == BufferFill)
			return bufferStatusFill;
		    break;
		case 2:
		    return droppedFull;
		    break;
		case 3:
		    return droppedLate;
		    break;
	    }
	}	
	return QVariant();
    }

    static QVariant headerData(int col, int role)
    {
	if(role == Qt::DisplayRole) 
	{
	    switch(col)
	    {
		case 0:
		    return QString("Name");
		    break;
		case 1:
		    return QString("BufferStatus");
		    break;
		case 2:
		    return QString("Dropped Full");
		    break;
		case 3:
		    return QString("Dropped Late");
		    break;
	    }
	}	
	return QVariant();
    }

    QVariant name;
    QVariant bufferStatusSize;
    QVariant bufferStatusFill;
    QVariant droppedFull;
    QVariant droppedLate;  
};

class TaskRepresentation {
public:
    TaskRepresentation()
    {
	childCount = 0;
    }
    
    ~TaskRepresentation()
    {
	for(std::vector<StreamRepresentation *>::const_iterator it = streams.begin(); it != streams.end(); it++)
	{
	    delete *it;
	}
    }
    
    void updateData(const aggregator::StreamAlignerStatus& status)
    {
	if(status.streams.size() != streams.size())
	    streams.resize(status.streams.size());

	taskName = QString::fromStdString(status.name);
	
	int i = 0;
	childCount = 0;
	for(std::vector<aggregator::StreamStatus>::const_iterator it = status.streams.begin(); it != status.streams.end(); it++)
	{

	    if(!streams[i])
	    {
		streams[i] = new StreamRepresentation(*it);
	    }

	    if(it->active) {
		streams[i]->updateData(*it);
		i++;
		childCount++;
	    }
	}
    }

    QVariant data(int col, int role)
    {
	if(role == Qt::DisplayRole && col == 0)
	    return taskName;
	
	return QVariant();
    };

    QVariant taskName;

    int getStreamCount() const
    {
	return childCount;
    };

    QVariant data(int row, int col, int role)
    {
	if(row >= childCount)
	{
	    throw std::runtime_error("Tried to access invalid stream");
	}
	return streams[row]->data(col, role);
    }
    
private:
    int childCount;
    std::vector<StreamRepresentation *> streams;
};

class StreamAlignerModel : public QAbstractItemModel
{
public:
    explicit StreamAlignerModel(QObject* parent = 0);
    ~StreamAlignerModel();
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex& child) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    void updateData(const aggregator::StreamAlignerStatus& status);
private:
    std::map<std::string, TaskRepresentation *> taskRepMap;
    std::vector<TaskRepresentation *> taskRep;
    
};

StreamAlignerModel::~StreamAlignerModel()
{
    for(std::vector<TaskRepresentation *>::const_iterator it = taskRep.begin(); it != taskRep.end(); it++)
    {
	delete *it;
    }
    taskRep.clear();
    taskRepMap.clear();
}

void StreamAlignerModel::updateData(const aggregator::StreamAlignerStatus& status)
{
    if(!taskRepMap.count(status.name))
    {
	TaskRepresentation *tp = new TaskRepresentation();
	taskRepMap[status.name] = tp;
	taskRep.push_back(tp);
    }

    taskRepMap[status.name]->updateData(status);
    
    //inform rest of the world that we got new data
    emit dataChanged(createIndex(0,0,0), createIndex(taskRep.size(), 5, 0));
}

StreamAlignerModel::StreamAlignerModel(QObject* parent): QAbstractItemModel(parent)
{
}

QVariant StreamAlignerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return StreamRepresentation::headerData(section, role);
}

int StreamAlignerModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid())
    {
	//parent is no task rep
	if(parent.internalId() != 1)
	    return 0;

	//return stream count of task rep
	return taskRep[parent.row()]->getStreamCount();
    }
    
    return taskRep.size();
}

int StreamAlignerModel::columnCount(const QModelIndex& parent) const
{
    return 4;    
}

QVariant StreamAlignerModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
	return QVariant();
	
    //task rep
    if(index.internalId() == 1) 
    {
	return taskRep[index.row()]->data(index.column(), role);
    }
    
    //this index points to a stream
    return taskRep[index.parent().row()]->data(index.row(), index.column(), role);
    
    return QVariant();
}

QModelIndex StreamAlignerModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
         return QModelIndex();

    if (!parent.isValid())
    {
	//index to root node
	const int taskCount = taskRep.size();
	if(row >= taskCount)
	    return QModelIndex();
	return createIndex(row, column, 1);	
    } else {
	if(parent.internalId() != 1)
	    throw std::runtime_error("Expected id 1 got something else");

	if(row < taskRep[parent.row()]->getStreamCount())
	    return createIndex(row, column, (void *) taskRep[parent.row()] );		
    }

    return QModelIndex();
}

QModelIndex StreamAlignerModel::parent(const QModelIndex& child) const
{
    if(!child.isValid()) {
	return QModelIndex();
    }
   
    //top level items have no valid parents
    if(child.internalId() == 1)
	return QModelIndex();
   
    int i = 0;
    for(std::vector<TaskRepresentation *>::const_iterator it = taskRep.begin(); it != taskRep.end(); it++) {
	if(*it == child.internalPointer())
	    return createIndex(i, 0, 1);
	i++;
    } 
    return QModelIndex();  
}

class StreamAlignerDelegate: public QStyledItemDelegate
{
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
    {
	if (index.column() == 1 && index.data(BufferSize).toInt()) {
	    int bSize = index.data(BufferSize).toInt();
	    int bFill = index.data(BufferFill).toInt();
	    
	    QStyleOptionProgressBar progressBarOption;
	    progressBarOption.rect = option.rect;
	    progressBarOption.minimum = 0;
	    progressBarOption.maximum = bSize;
	    progressBarOption.progress = bFill;
	    progressBarOption.text = QString::number(bFill) + " / " + QString::number(bSize);
	    progressBarOption.textVisible = true;

	    QApplication::style()->drawControl(QStyle::CE_ProgressBar,
					    &progressBarOption, painter);
	} else
	    QStyledItemDelegate::paint(painter, option, index);
    }
};


StreamAlignerWidget::StreamAlignerWidget(QWidget* parent): QTreeView(parent)
{
    model = new StreamAlignerModel;
    
    StreamAlignerDelegate *delegate = new StreamAlignerDelegate();
    setItemDelegate(delegate);
    setModel(model);
    update();
}





void StreamAlignerWidget::updateData(const aggregator::StreamAlignerStatus& status)
{
    StreamAlignerModel *m = dynamic_cast<StreamAlignerModel *>(model);
    m->updateData(status);
}

StreamAlignerWidget::~StreamAlignerWidget()
{
}

