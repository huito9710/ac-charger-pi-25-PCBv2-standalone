#ifndef CONNSTATUSINDICATOR_H
#define CONNSTATUSINDICATOR_H

#include <QObject>
#include <QLabel>
#include <OCPP/ocppclient.h>

class ConnStatusIndicator : public QObject
{
    Q_OBJECT
public:
    explicit ConnStatusIndicator(OcppClient &cli, QObject *parent = nullptr);
    void setIndicator(QLabel *label);

signals:

private slots:
    void onConnected();
    void onDisconnected();

private:
    const QString offline_icon_path;
    const QString online_icon_path;
    OcppClient &ocppCli;
    QLabel *icon;

    void updateIcon(const QString graphicsFilePath);
};

#endif // CONNSTATUSINDICATOR_H
