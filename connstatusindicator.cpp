#include "connstatusindicator.h"
#include "hostplatform.h"

ConnStatusIndicator::ConnStatusIndicator(OcppClient &cli, QObject *parent)
    : QObject(parent),
      offline_icon_path(QString("%1PNG/offline-32.png").arg(charger::Platform::instance().guiImageFileBaseDir)),
      online_icon_path(QString("%1PNG/online-32.png").arg(charger::Platform::instance().guiImageFileBaseDir)),
      ocppCli(cli),
      icon(nullptr)
{
    if(!charger::isStandaloneMode()){
        connect(&ocppCli, &OcppClient::connStatusOnlineIndicator, this, &ConnStatusIndicator::onConnected);
        connect(&ocppCli, &OcppClient::connStatusOfflineIndicator, this, &ConnStatusIndicator::onDisconnected);
    }
}

void ConnStatusIndicator::setIndicator(QLabel *label)
{
    this->icon = label;
    if (ocppCli.isOffline())
        onDisconnected();
    else
        onConnected();
}

void ConnStatusIndicator::onConnected()
{
    updateIcon(online_icon_path);
}

void ConnStatusIndicator::onDisconnected()
{
    updateIcon(offline_icon_path);
}

void ConnStatusIndicator::updateIcon(const QString graphicsFilePath)
{
    if (icon) {
        icon->setPixmap(graphicsFilePath);
        icon->setAlignment(Qt::AlignCenter);
    }
}
