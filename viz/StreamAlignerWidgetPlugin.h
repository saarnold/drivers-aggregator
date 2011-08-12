#ifndef STREAMALIGNERWIDGETPLUGIN_H
#define STREAMALIGNERWIDGETPLUGIN_H

#include <QObject>
#include <QtDesigner/QDesignerCustomWidgetInterface>

class StreamAlignerWidgetPlugin : public QObject , public QDesignerCustomWidgetInterface
{
	Q_OBJECT
	Q_INTERFACES(QDesignerCustomWidgetInterface)

    public:
	StreamAlignerWidgetPlugin(QObject *parent = 0);
	virtual ~StreamAlignerWidgetPlugin();

	bool isContainer() const;
	bool isInitialized() const;
	QIcon icon() const;
	QString domXml() const;
	QString group() const;
	QString includeFile() const;
	QString name() const;
	QString toolTip() const;
	QString whatsThis() const;
	QWidget *createWidget(QWidget *parent);
	void initialize(QDesignerFormEditorInterface *core);

    private:
	bool initialized;  
};


class AggregatorCollection: public QObject, public QDesignerCustomWidgetCollectionInterface
{
   Q_OBJECT
   Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

public:
   AggregatorCollection(QObject *parent = 0);
   virtual QList<QDesignerCustomWidgetInterface*> customWidgets() const;

private:
   QList<QDesignerCustomWidgetInterface*> widgets;
};

#endif // STREAMALIGNERWIDGETPLUGIN_H
