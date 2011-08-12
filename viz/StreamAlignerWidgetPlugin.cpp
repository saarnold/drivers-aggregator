#include "StreamAlignerWidgetPlugin.h"
#include "StreamAlignerWidget.h"
#include <QtCore/qplugin.h>

Q_EXPORT_PLUGIN2(AggregatorCollection, AggregatorCollection)

AggregatorCollection::AggregatorCollection(QObject *parent)
       : QObject(parent)
{
   widgets.append(new StreamAlignerWidgetPlugin(this));

}

QList<QDesignerCustomWidgetInterface*> AggregatorCollection::customWidgets() const
{
   return widgets;
}

StreamAlignerWidgetPlugin::StreamAlignerWidgetPlugin(QObject *parent)
     : QObject(parent)
 {
     initialized = false;
 }

 StreamAlignerWidgetPlugin::~StreamAlignerWidgetPlugin()
 {
 }

 void StreamAlignerWidgetPlugin::initialize(QDesignerFormEditorInterface *formEditor)
 {
     if (initialized)
         return;
     initialized = true;
 }

 bool StreamAlignerWidgetPlugin::isInitialized() const
 {
     return initialized;
 }

 QWidget *StreamAlignerWidgetPlugin::createWidget(QWidget *parent)
 {
     return new StreamAlignerWidget(parent);
 }

 QString StreamAlignerWidgetPlugin::name() const
 {
     return "StreamAlignerWidget";
 }

 QString StreamAlignerWidgetPlugin::group() const
 {
     return "Rock-Robotics";
 }

 QIcon StreamAlignerWidgetPlugin::icon() const
 {
     return QIcon(":/viz/icon.png");
 }

 QString StreamAlignerWidgetPlugin::toolTip() const
 {
     return "Widget that shows the status of the Stream Aligners";
 }

 QString StreamAlignerWidgetPlugin::whatsThis() const
 {
     return "Widget that shows the status of the Stream Aligners";
 }

 bool StreamAlignerWidgetPlugin::isContainer() const
 {
     return false;
 }

 QString StreamAlignerWidgetPlugin::domXml() const
 {
     return QString("<widget class=\"StreamAlignerWidget\" name=\"") + name() +
	    QString("\">\n"
            " <property name=\"geometry\">\n"
            "  <rect>\n"
            "   <x>0</x>\n"
            "   <y>0</y>\n"
            "   <width>250</width>\n"
            "   <height>250</height>\n"
            "  </rect>\n"
            " </property>\n"
            " <property name=\"toolTip\" >\n"
	    "  <string>") + toolTip() +
	    QString("</string>\n"
	    "</property>\n"
            " <property name=\"whatsThis\" >\n"
            "  <string>")
	    + whatsThis() + QString("</string>\n"
            " </property>\n"
            "</widget>\n");
 }

 QString StreamAlignerWidgetPlugin::includeFile() const
 {
     return "aggregator/StreamAlignerWidget.h";
 }
