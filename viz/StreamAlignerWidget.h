#ifndef STREAMALIGNERWIDGET_H
#define STREAMALIGNERWIDGET_H
#include <QTreeView>

class QStandardItemModel;
class TaskRepresentation;
namespace aggregator {
    class StreamAlignerStatus;
}

class StreamAlignerWidget : public QTreeView
{
    Q_OBJECT
    public:
	StreamAlignerWidget(QWidget *parent);
	virtual ~StreamAlignerWidget();
    public slots:
	void updateData(const aggregator::StreamAlignerStatus &data);
    private:
	QAbstractItemModel *model;
};


#endif // STREAMALIGNERWIDGET_H
