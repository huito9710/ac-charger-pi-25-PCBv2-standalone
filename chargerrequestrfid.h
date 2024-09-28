#ifndef CHARGERREQUESTRFID_H
#define CHARGERREQUESTRFID_H

#include <QObject>
#include "rfidreaderwindow.h"
#include "OCPP/ocppTypes.h"

///
/// \brief The ChargerRequestRfid class
/// Read RFID Tag, initiate OCPP Authorize, present Authorize result.
/// Make use of RfidReaderWindow as View
/// Note: The work for BeginCharge and EndCharge is different
/// BeginCharge: Check the UUID against Auth-Cache, Auth-Local-List and finally OCPP server
/// EndCharge: check the UUID against the UUID used in current transaction, then OCPP server
///
class ChargerRequestRfid : public QObject
{
    Q_OBJECT
public:
    explicit ChargerRequestRfid(QObject *parent = nullptr);
    ~ChargerRequestRfid();
    enum class Purpose {
        BeginCharge,
        EndCharge,
        SuspendCharge,
    };

    void setStartTxRfid(const QString uuid);
    void start(Purpose, bool showWin=true);
    void runBackground(); // hide window
    void stop(); // close window (stop reading RFID)

signals:
    void requestAccepted();
    void requestCanceled();

private slots:
    void onReaderResponded(RfidReaderWindow::ReaderStatus newStatus, const QByteArray &cardNum = QByteArray());
    // Ocpp Response
    void onOcppAuthorizeResponse(bool responseAvail, ocpp::AuthorizationStatus authStatus);
    void onOcppAuthorizeReqCannotSend(); // unable to send request

private:
    enum class State {
        Idle,
        ReadCard,
        LocalListAuthorize,
        OcppAuthorize,
        AuthSucceeded,
        AuthRejected,
    };
    void closeReaderWin() { if (readerWin) { readerWin->close(); readerWin = nullptr; } };
    void displayRejection(RfidReaderWindow::RejectReason reason = RfidReaderWindow::RejectReason::Nil);
    std::unique_ptr<ocpp::AuthorizationStatus> checkLocalAuthList(QString tagUid);
    void checkCardAgainstServer(const QString uuid, const bool useAuthCache = true);

private:
    State st;
    RfidReaderWindow *readerWin;
    Purpose purpose;
    QString startTxRfidUuid; // RFID UUID used for Start-Transaction, relevant when purpose is EndCharge
};

#endif // CHARGERREQUESTRFID_H
