#ifndef DgaIMAGEPLUGIN_H
#define DgaIMAGEPLUGIN_H

#include <QImageIOPlugin>
class DgaImagePlugin : public QImageIOPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID QImageIOHandlerFactoryInterface_iid FILE "qdga.json")

public:
	DgaImagePlugin(QObject *parent = nullptr);

	// QImageIOPlugin interface
	Capabilities capabilities(QIODevice *device, const QByteArray &format) const final;
	QImageIOHandler *create(QIODevice *device, const QByteArray &format) const final;
};

#endif // DgaIMAGEPLUGIN_H
