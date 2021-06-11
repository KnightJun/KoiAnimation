#include "Dgaimageplugin.h"
#include "DgaIOHandler.h"

DgaImagePlugin::DgaImagePlugin(QObject *parent) :
	QImageIOPlugin(parent)
{}

QImageIOPlugin::Capabilities DgaImagePlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
	if (format == "dga") {
		if(device && device->bytesAvailable() >= 8)
			return DgaIOHandler::canRead(device) ? CanRead : static_cast<Capability>(0);
		else
			return CanRead;
	} else
		return static_cast<Capability>(0);
}

QImageIOHandler *DgaImagePlugin::create(QIODevice *device, const QByteArray &format) const
{
	if(device && capabilities(device, format).testFlag(CanRead)) {
		auto handler = new DgaIOHandler();
		handler->setDevice(device);
		handler->setFormat(format);
		return handler;
	} else
		return nullptr;
}
