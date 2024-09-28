#ifndef CRFIDTHREAD_H
#define CRFIDTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
//#include <QtSerialPort/QSerialPort>
#include "T_Type.h"
#include <nfc/nfc.h>
#include <nfc/nfc-types.h>
#include "libnfc/chips/pn53x.h"
#include "libnfc/utils/mifare.h"

class CRfidThread : public QThread
{
    Q_OBJECT

public:
    CRfidThread(QObject *parent =0);
    ~CRfidThread();

    void StartSlave(const QString &context, int waitTimeout);
    void StartSlave(int waitTimeout);
    char ReadCard(void);
    void ReadCardAgain(void);
    char GetStatus(void);

protected:
    void    run();

signals:
    void    readerPortOpened(bool succ);
    void    readerModuleActivated(bool succ);
    void    RFIDCard(const QByteArray);

private:
    void LoopRFID(nfc_device &nfcdev);
    bool RFID_Setup(nfc_device &nfcdev);
    QByteArray WriteCommand(QByteArray& data, u8 cmdlen);
    bool RFID_ReadID(nfc_device &nfcdev);
    bool VerifyCard(nfc_device &nfcdev, u8 keyType);
    bool RFID_AuthenticateRead(nfc_device &nfcdev, u8 keytype);
    bool authenticate(nfc_device *pnd, mifare_classic_tag &mtKeys, mifare_param &mp, nfc_target &nt, u8 keytype, uint32_t uiBlock, bool bUseKeyFile);

    bool RFID_AuthenticateBlock(nfc_device &nfcdev, u8 keytype, u8 blockno);
    bool RFID_ReadBlock(nfc_device &nfcdev, u8 blockno, QByteArray &data);

    bool unlock_card(nfc_device *pnd, nfc_context *context, nfc_target &nt, bool write);
    static bool transmit_bits(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTxBits);
    static bool transmit_bytes(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx);
    static uint32_t get_trailer_block(uint32_t uiFirstBlock);
    static bool is_trailer_block(uint32_t uiBlock);
    static void print_success_or_failure(bool bFailure, uint32_t *uiBlockCounter);

#if false // Not implemented yet
    void GetKeyCode(QByteArray &keycode);
    bool BlockDataIsAvailable(QByteArray &blockdata);
#endif

private:
    //QString Port;
    QString NfcConnString;
    int     WaitTimeout;
    QMutex  Mutex;
    int     Read;
    char    Status;     //0:OK 1:串口故障 2:无读卡器
    bool    Stop;
    QByteArray CardNo;
    uint16_t   Card_ATQA; // NXP: AN10833 Table.5

    mifare_param mp;
    mifare_classic_tag mtKeys;
    mifare_classic_tag mtDump;

    uint8_t abtHalt[4] = { 0x50, 0x00, 0x00, 0x00 };

    // special unlock command
    uint8_t abtUnlock1[1] = { 0x40 };
    uint8_t abtUnlock2[1] = { 0x43 };
};

static const nfc_modulation nmMifare = {
  .nmt = NMT_ISO14443A,
  .nbr = NBR_106,
};



#endif // CRFIDTHREAD_H

