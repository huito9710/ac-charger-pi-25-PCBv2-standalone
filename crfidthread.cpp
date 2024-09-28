#include "crfidthread.h"
#include <QtCore/QDebug>
#include "api_function.h"
#include "const.h"
#include "logconfig.h"
#include "hostplatform.h"

//[用法:
//CRfidThread    RFIDThread;
//RFIDThread.StartSlave("/dev/ttyS0", 500);
//RFIDThread.ReadCard();
//]

QT_USE_NAMESPACE

constexpr char _PREAMBLE = 0x00;
constexpr char _STARTCODE = 0xFF;
constexpr char _POSTAMBLE = 0x00;
constexpr char _HOSTto532 = 0xD4;
#define _PN532IC    "\xD5\x03\x32"      //查版本号命令返回是PN532
constexpr char _GetFirmwareVersion =0x02;
constexpr char _SAMConfig = 0x14;
constexpr char _InListPassiveTarget = 0x4A;
constexpr char _InDataExchange = 0x40;
constexpr char _KEYA = 0x60;   //授权码A
constexpr char _KEYB = 0x61;

static constexpr int _PN532ACK_LEN = 6;
static constexpr char _pn532Ack[_PN532ACK_LEN] = { '\0', '\0', '\xFF', '\0', '\xFF', '\0'}; //"\x00\x00\xFF\x00\xFF\x00"
static const QByteArray _PN532ACK = QByteArray::fromRawData(_pn532Ack, _PN532ACK_LEN);
static uint8_t keys[] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xd3, 0xf7, 0xd3, 0xf7, 0xd3, 0xf7,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5,
  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5,
  0x4d, 0x3a, 0x99, 0xc3, 0x51, 0xdd,
  0x1a, 0x98, 0x2c, 0x7e, 0x45, 0x9a,
  0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xab, 0xcd, 0xef, 0x12, 0x34, 0x56
};
static size_t num_keys = sizeof(keys) / 6;
#define MAX_FRAME_LEN 264
static uint8_t abtRx[MAX_FRAME_LEN];
static int szRxBits;
static bool unlocked = false;

CRfidThread::CRfidThread(QObject *parent)
    : QThread(parent), WaitTimeout(0), Stop(false)
{
}

CRfidThread::~CRfidThread()
{
    Mutex.lock();
    Stop =true;         //控制结束,清理线程
    Mutex.unlock();
    Read =0;
    wait();
    qCDebug(Rfid) << "RFID Exit";
}

void CRfidThread::StartSlave(int waitTimeout)
{
    QString nfcConnString;
    charger::Platform::asyncShellCommand(QStringList()<<"Charger_cmd.sh"<<"rfid"<<"-local",nfcConnString);
    nfcConnString = nfcConnString.trimmed();
    qWarning()<<nfcConnString;
    if(nfcConnString==""){
        charger::Platform::asyncShellCommand(QStringList()<<"Charger_cmd.sh"<<"rfid"<<"-global",nfcConnString);
        nfcConnString = nfcConnString.trimmed();
        qWarning()<<nfcConnString;
        if(nfcConnString=="") return;
    }

    StartSlave(nfcConnString, waitTimeout);
}

void CRfidThread::StartSlave(const QString &nfcctx, int waitTimeout)
{
    QMutexLocker locker(&Mutex);
    NfcConnString =nfcctx;         // /dev/ttyS0 ?
    WaitTimeout =waitTimeout;
    Stop    =false;
    Read    =0;
    Status  =1;
    start();
}

void CRfidThread::run()
{
    //QSerialPort serial;
    nfc_device *pnd;
    nfc_context *context;

    qCWarning(Rfid) << "RFID run begins.";

    if(context == NULL){
        qCWarning(Rfid) << "RFID device not found! Please check the internal cables.";
        return;
    }else{
        qCWarning(Rfid) << context;

        do
        {
            nfc_init(&context);
            //if(NfcConnString !="")
            if(context != nullptr)
            {
                //serial.setPortName(Port);
                //serial.close();
                //serial.setBaudRate(QSerialPort::Baud115200);    //8 N 1stop
                pnd = nfc_open(context, NfcConnString.toLatin1().data());
                if(pnd != NULL)
                {
                    Status =2;
                    emit readerPortOpened(true);
                    if(RFID_Setup(*pnd)) {
                        emit readerModuleActivated(true);
                        LoopRFID(*pnd);
                    }
                    else {
                        emit readerModuleActivated(false);
                    }
                    //serial.close();
                    nfc_close(pnd);
                    nfc_exit(context);
                }
                else
                {
                    nfc_exit(context);
                    Status =1;
                    qCWarning(Rfid) <<"Can't open the default RFID dev "<<NfcConnString<<".";
                    emit readerPortOpened(false);
                }
            }
            if(!Stop) sleep(3);
        } while(!Stop);
    }
    qCWarning(Rfid) << "RFID run ends.";
}

void CRfidThread::LoopRFID(nfc_device &nfcdev)
{
    Status =0;
    do
    {
        if(Read ==1)    //有读卡命令
        {
            if(RFID_ReadID(nfcdev))
            {
                if(!CardNo.isEmpty())
                {
                    bool authOK = false;
                    switch (Card_ATQA) {
                    case 0x0004: // Mini, Classic-1K
                    case 0x0002: // Classic-4K
                        // The following KEY_A and KEY_B approach is referring to the
                        // classic-card-format example from libfreefare (libnfc project)
                        qWarning() << QString("Card ATQA = %1").arg(Card_ATQA, 4, 16, QLatin1Char('0'));
                        authOK = VerifyCard(nfcdev, _KEYA);
                        if (!authOK) {
                            // Try KEYB, but start from ReadID first
                            if (RFID_ReadID(nfcdev) && !CardNo.isEmpty()) {
                                authOK = VerifyCard(nfcdev, _KEYB);
                            }
                        }
                        qWarning() << "NFC Authentication: " << authOK;
                        break;
                    default:
                        qWarning() << "NFC Authentication skipped.";
                        authOK = true; // Authentication method TBD
                        break;
                    }
                    if (authOK)
                    {
                        QByteArray cardno;
                        Mutex.lock();
                        if(Read ==1) Read =255;
                        cardno =CardNo;
                        Mutex.unlock();
                        emit this->RFIDCard(cardno);
                    }
                    else {
                        qCWarning(Rfid) << "NFC Card number received. But Authentication Failed.";
                    }
                }
            }
            else break;     //NO ACK
        }
        msleep(300);

    }while(!Stop);
}

char CRfidThread::ReadCard(void)
{ //供外部调用启动读卡, 仅在待读卡状态下启动读卡,在读或己读完则无效
    QMutexLocker locker(&Mutex);
    if(Read ==0)
    {
        Read =1;
    }
    else if(Read ==255)
    {
        //读完卡
    }

    return Read;
}

void CRfidThread::ReadCardAgain(void)
{ //供外部调用启动读卡, 不管未读/在读/已读完
    QMutexLocker locker(&Mutex);
    Read =1;
}

char CRfidThread::GetStatus(void)
{ //供外部查询状态
    //QMutexLocker locker(&Mutex);
    return Status;
}

bool CRfidThread::RFID_Setup(nfc_device &nfcdev)
{
    bool retcode =false;
    QByteArray command(8, '\x00'), data;
    nfc_device * devptr = &nfcdev;

    /*
    command.append(_PN532ACK);  //取消旧命令, 没有返回数据
    serial.write(command);
    serial.waitForBytesWritten(50);
    serial.clear();
    */

    // 1._GetFirmwareVersion,  _PN532IC (有PN532读卡器, 进行SAM配置)
    //    GetFirmwareVersion command is used to set PN53x chips type (PN531, PN532 or PN533)
    // 2._SAMConfig, _PN532ACK ()
    //    pn532_sam_mode defined in pn53x.h

    int res = pn53x_decode_firmware_version(devptr);
    if(res > -1){
        retcode = true;

        //pnd->btSupportByte judge whether SUPPORT_ISO14443A or SUPPORT_ISO14443B
        //CHIP_DATA(pnd)->type judge if PN531
        if(CHIP_DATA(devptr)->type == PN532){
            if ((res = pn532_SAMConfiguration(devptr, PSM_NORMAL, 200)) < 0) {
                retcode = false;
            }
        }
    }//有PN532读卡器, 进行SAM配置

    return retcode;
}

QByteArray CRfidThread::WriteCommand(QByteArray& data, u8 cmdlen)
{ //对命令与长度进行格式化
    char checksum =0;

    cmdlen++;
    data.insert(0, _HOSTto532);
    for(int i=0; i<cmdlen; i++)
    {
        checksum +=data.data()[i];
    }
    data.append((char)(~checksum)+1);
    data.append(_POSTAMBLE);

    data.insert(0, (char)(~cmdlen)+1);
    data.insert(0, (char)cmdlen);
    data.insert(0, _STARTCODE);
    data.insert(0, _PREAMBLE);
    data.insert(0, _PREAMBLE);

    return data;
}

//reimplemented with native UART, I2C, SPI and USB supported with LibNFC
bool CRfidThread::RFID_ReadID(nfc_device &nfcdev)
{
    bool retcode =false;

    nfc_device * devptr = &nfcdev;
    nfc_modulation nm;
    int MAX_DEVICE_COUNT = 16;

    int res;

    nfc_target ant[MAX_DEVICE_COUNT];

    nm.nmt = NMT_ISO14443A;
    nm.nbr = NBR_106;
    // List ISO14443A targets
    if ((res = nfc_initiator_list_passive_targets(devptr, nm, ant, MAX_DEVICE_COUNT)) >= 0) {
      if (res > 0) {
        qCInfo(Rfid) <<res<<" ISO14443A passive target(s) found.";
      }
      for (int n = 0; n < res; n++) {
          if(ant[n].nm.nmt == nfc_modulation_type::NMT_ISO14443A) {
            Card_ATQA = ant[n].nti.nai.abtAtqa[1]
                    + ant[n].nti.nai.abtAtqa[0] * 16;
            CardNo = QByteArray(reinterpret_cast<char*>(ant[n].nti.nai.abtUid)
                                , ant[n].nti.nai.szUidLen);
            retcode = true;
          }else{
              CardNo.clear();
              Card_ATQA = 0xFFFF;
          }
      }
    }
    return retcode;

    /*
    QByteArray command(3, '\x00');
    int i;

    command.data()[0] =_InListPassiveTarget;
    command.data()[1] =0x01;
    WriteCommand(command, 3);
    serial.write(command);
    qCInfo(Rfid) <<"RFID_ReadID command: "<<command.toHex().toUpper();
    if(serial.waitForReadyRead(50))
    {
        QByteArray RxData =serial.readAll();

        while(serial.waitForReadyRead(100)) RxData +=serial.readAll();
        qCInfo(Rfid) <<"RFID_ReadID (first-while) RxData: "<<RxData.toHex().toUpper();
        if(RxData.contains(_PN532ACK))      //有PN532 ACK
        {
            CardNo.clear();
            Card_ATQA = 0xFFFF;
            retcode =true;
            do
            {
                i =RxData.indexOf("\xD5\x4B\x01\x01");  //读卡号成功
                if((i >0) &&(RxData.length() >i+12))
                {
                    Card_ATQA = (RxData[i+4] << 8) | static_cast<unsigned char>(RxData[i+5]);
                    if(RxData.length() >i+8+RxData.data()[i+7])
                    {
                        CardNo =RxData.mid(i+8, RxData.data()[i+7]);
                        qCDebug(Rfid) <<"RFID_ReadID CardNo: "<<CardNo.toHex().toUpper();
                    }
                    break;
                }
                while(serial.waitForReadyRead(200)) RxData +=serial.readAll();
                qCInfo(Rfid) <<"RFID_ReadID (do-while) RxData: "<<RxData.toHex().toUpper();
            }while(Read ==1);
        }
    }*/
}

bool CRfidThread::VerifyCard(nfc_device &nfcdev, u8 keyType)
{
    /*
    bool retcode =false;

    if(RFID_Authenticate(nfcdev, keyType))
    {
        QByteArray BlockData;
        if(RFID_ReadBlock(nfcdev, 5, BlockData))
        {
            qWarning() <<"block: "<<BlockData.toHex().toUpper();
#if false // function not implemented
            if(BlockDataIsAvailable(BlockData))
                retcode =true;
#else
            retcode = true;
#endif
        }
    }
    return retcode;
    */

    return RFID_AuthenticateRead(nfcdev, keyType);
}


bool CRfidThread::RFID_AuthenticateRead(nfc_device &nfcdev, u8 keytype)
{
    bool retcode =true;

    //NFC card settings
    bool bFailure = false;
    bool read_unlocked = false;

    uint8_t uiBlocks;
    uint32_t uiReadBlocks = 0;

    nfc_device * devptr = &nfcdev;
    nfc_context *context;
    nfc_target nt;

    // Try to find a MIFARE Classic tag
    int tags;
    char * cardNoPtr = CardNo.data();
    tags = nfc_initiator_select_passive_target(devptr, nmMifare, reinterpret_cast<unsigned char *>(cardNoPtr), CardNo.length(), &nt);
    if (tags <= 0) {
        qWarning("Error: no tag was found\n");
        return false;
    }

    // Test if we are dealing with a MIFARE compatible tag
    if (((nt.nti.nai.btSak & 0x08) == 0) && (nt.nti.nai.btSak != 0x01)) {
        qWarning("Warning: tag is probably not a MFC!\n");
    }

    //------------------------------------------------
    nfc_init(&context);
    if(read_unlocked){
        unlock_card(devptr, context, nt, false);
    }
    // Guessing size
    if ((nt.nti.nai.abtAtqa[1] & 0x02) == 0x02 || nt.nti.nai.btSak == 0x18)
    // 4K
        uiBlocks = 0xff;
    else if (nt.nti.nai.btSak == 0x09)
    // 320b
        uiBlocks = 0x13;
    else
    // 1K/2K, checked through RATS
        uiBlocks = 0x3f;

    for (int32_t iBlock = uiBlocks; iBlock >= 0; iBlock--) {
      // Authenticate everytime we reach a trailer block
      if (is_trailer_block(iBlock)) {
        if (bFailure) {
            // When a failure occured we need to redo the anti-collision
            if (nfc_initiator_select_passive_target(devptr, nmMifare, nullptr, 0, &nt) <= 0) {
                printf("!\nError: tag was removed\n");
                qWarning()<<"Error: tag was removed";
                return false;
            }
            bFailure = false;
        }

        // Try to authenticate for the current sector
        if (!authenticate(devptr, mtKeys, mp, nt, keytype, iBlock, false)) {// No Key File used, mp not initialized
            qWarning()<<QString("Error: authentication failed for block %1").arg(iBlock);
            return false;
        }

        // Try to read out the trailer
        if (nfc_initiator_mifare_cmd(devptr, MC_READ, iBlock, &mp)) {
            if (read_unlocked) {
                memcpy(mtDump.amb[iBlock].mbd.abtData, mp.mpd.abtData, sizeof(mtDump.amb[iBlock].mbd.abtData));
            }
            else {
                // Copy the keys over from our key dump and store the retrieved access bits
                memcpy(mtDump.amb[iBlock].mbt.abtKeyA, mtKeys.amb[iBlock].mbt.abtKeyA, sizeof(mtDump.amb[iBlock].mbt.abtKeyA));
                memcpy(mtDump.amb[iBlock].mbt.abtAccessBits, mp.mpt.abtAccessBits, sizeof(mtDump.amb[iBlock].mbt.abtAccessBits));
                memcpy(mtDump.amb[iBlock].mbt.abtKeyB, mtKeys.amb[iBlock].mbt.abtKeyB, sizeof(mtDump.amb[iBlock].mbt.abtKeyB));
            }
        } else {
            printf("!\nfailed to read trailer block 0x%02x\n", iBlock);
            qWarning()<<"failed to read trailer block"<<iBlock;
            bFailure = true;
        }
      } else {
        // Make sure a earlier readout did not fail
        if (!bFailure) {
          // Try to read out the data block
          if (nfc_initiator_mifare_cmd(devptr, MC_READ, iBlock, &mp)) {
            memcpy(mtDump.amb[iBlock].mbd.abtData, mp.mpd.abtData, sizeof(mtDump.amb[iBlock].mbd.abtData));
          } else {
            printf("!\nError: unable to read block 0x%02x\n", iBlock);
            qWarning()<<"Error: unable to read block"<<iBlock;
            bFailure = true;
          }
        }
      }
      // Show if the readout went well for each block
      CRfidThread::print_success_or_failure(bFailure, &uiReadBlocks);
      if (bFailure)
        return false;
      }
    return retcode;
}

bool CRfidThread::authenticate(nfc_device *pnd, mifare_classic_tag &mtKeys, mifare_param &mp, nfc_target &nt, u8 keytype, uint32_t uiBlock, bool bUseKeyFile)
{
  mifare_cmd mc;

  // Set the authentication information (uid)
  memcpy(mp.mpa.abtAuthUid, nt.nti.nai.abtUid + nt.nti.nai.szUidLen - 4, 4);

  // Should we use key A or B?
  switch(keytype){
      case 0x60:
        mc = MC_AUTH_A;
      break;

      case 0x61:
        mc = MC_AUTH_B;
      break;

      default:
        mc = MC_AUTH_A;
  }

  // Key file authentication.
  if (bUseKeyFile) {

    // Locate the trailer (with the keys) used for this sector
    uint32_t uiTrailerBlock;
    uiTrailerBlock = get_trailer_block(uiBlock);

    // Extract the right key from dump file
    if (mc == MC_AUTH_A)
      memcpy(mp.mpa.abtKey, mtKeys.amb[uiTrailerBlock].mbt.abtKeyA, sizeof(mp.mpa.abtKey));
    else
      memcpy(mp.mpa.abtKey, mtKeys.amb[uiTrailerBlock].mbt.abtKeyB, sizeof(mp.mpa.abtKey));

    // Try to authenticate for the current sector
    if (nfc_initiator_mifare_cmd(pnd, mc, uiBlock, &mp))
      return true;

    // If formatting or not using key file, try to guess the right key
  } else{
    for (size_t key_index = 0; key_index < num_keys; key_index++) {
      memcpy(mp.mpa.abtKey, keys + (key_index * 6), 6);
      if (nfc_initiator_mifare_cmd(pnd, mc, uiBlock, &mp)) {
        if (mc == MC_AUTH_A)
          memcpy(mtKeys.amb[uiBlock].mbt.abtKeyA, &mp.mpa.abtKey, sizeof(mtKeys.amb[uiBlock].mbt.abtKeyA));
        else
          memcpy(mtKeys.amb[uiBlock].mbt.abtKeyB, &mp.mpa.abtKey, sizeof(mtKeys.amb[uiBlock].mbt.abtKeyB));
        return true;
      }
      if (nfc_initiator_select_passive_target(pnd, nmMifare, nt.nti.nai.abtUid, nt.nti.nai.szUidLen, nullptr) <= 0) {
        qWarning()<<"tag was removed";
        return false;
      }
    }
  }

  return false;
}

void CRfidThread::print_success_or_failure(bool bFailure, uint32_t *uiBlockCounter)
{
  printf("%c", (bFailure) ? 'x' : '.');
  if (uiBlockCounter && !bFailure)
    *uiBlockCounter += 1;
}

bool CRfidThread::transmit_bits(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTxBits)
{
  const char * c_pbtTx = reinterpret_cast<const char *>(pbtTx);  //(const_cast<uint8_t *>(pbtTx));
  QByteArray txData = QByteArray(c_pbtTx,szTxBits);
  // Show transmitted command
  qWarning()<<"Sent bits:     "<<txData;
  // Transmit the bit frame command, we don't use the arbitrary parity feature
  if ((szRxBits = nfc_initiator_transceive_bits(pnd, pbtTx, szTxBits, nullptr, abtRx, sizeof(abtRx), nullptr)) < 0)
    return false;

  const char * c_abtRx = reinterpret_cast<const char *>(abtRx);
  QByteArray rxData = QByteArray(c_abtRx,szRxBits);
  // Show received answer
  qWarning()<<"Received bits: "<<rxData;
  // Succesful transfer
  return true;
}

bool CRfidThread::transmit_bytes(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx)
{
    const char * c_pbtTx = reinterpret_cast<const char *>(pbtTx);
    QByteArray txData = QByteArray(c_pbtTx,szTx);
  // Show transmitted command
  qWarning()<<"Sent bits:     "<<txData;
  // Transmit the command bytes
  int res;
  if ((res = nfc_initiator_transceive_bytes(pnd, pbtTx, szTx, abtRx, sizeof(abtRx), 0)) < 0)
    return false;

  const char * c_abtRx = reinterpret_cast<const char *>(abtRx);  //(const_cast<uint8_t *>(pbtTx));
  QByteArray rxData = QByteArray(c_abtRx,res);
  // Show received answer
  qWarning()<<"Received bits: "<<rxData;
  // Succesful transfer
  return true;
}

uint32_t CRfidThread::get_trailer_block(uint32_t uiFirstBlock)
{
  // Test if we are in the small or big sectors
  uint32_t trailer_block = 0;
  if (uiFirstBlock < 128) {
    trailer_block = uiFirstBlock + (3 - (uiFirstBlock % 4));
  } else {
    trailer_block = uiFirstBlock + (15 - (uiFirstBlock % 16));
  }
  return trailer_block;
}

bool CRfidThread::is_trailer_block(uint32_t uiBlock)
{
  // Test if we are in the small or big sectors
  if (uiBlock < 128)
    return ((uiBlock + 1) % 4 == 0);
  else
    return ((uiBlock + 1) % 16 == 0);
}

bool CRfidThread::unlock_card(nfc_device *pnd, nfc_context *context, nfc_target &nt, bool write)
{
    bool dWrite = false;

    // Configure the CRC
    if (nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, false) < 0) {
        nfc_perror(pnd, "nfc_configure");
        return false;
    }
    // Use raw send/receive methods
    if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, false) < 0) {
        nfc_perror(pnd, "nfc_configure");
        return false;
    }

    iso14443a_crc_append(abtHalt, 2);
    CRfidThread::transmit_bytes(pnd, abtHalt, 4);
    // now send unlock
    if (!CRfidThread::transmit_bits(pnd, abtUnlock1, 7)) {
        qWarning()<<"Warning: Unlock command [1/2]: failed / not acknowledged.";

        dWrite = true;
        if (write) {
          qWarning()<<"Trying to rewrite block 0 on a direct write tag.";
        }
    } else {
        if (CRfidThread::transmit_bytes(pnd, abtUnlock2, 1)) {
          qWarning()<<"Card unlocked";
          unlocked = true;
        } else {
          qWarning()<<"Warning: Unlock command [2/2]: failed / not acknowledged.";
        }
    }

    // reset reader
    if (!unlocked) {
        if (nfc_initiator_select_passive_target(pnd, nmMifare, nt.nti.nai.abtUid, nt.nti.nai.szUidLen, nullptr) <= 0) {
          qWarning()<<"Error: tag was removed";
          nfc_close(pnd);
          nfc_exit(context);
          exit(EXIT_FAILURE);
        }
        return true;
    }
    // Configure the CRC
    if (nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, true) < 0) {
        qWarning()<<"NP_HANDLE_CRC"<<"nfc_device_set_property_bool";
        nfc_perror(pnd, "nfc_device_set_property_bool");
        return false;
    }
    // Switch off raw send/receive methods
    if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, true) < 0) {
        qWarning()<<"NP_EASY_FRAMING"<<"nfc_device_set_property_bool";
        nfc_perror(pnd, "nfc_device_set_property_bool");
        return false;
    }
    return true;
}

//reimplemented with native UART, I2C, SPI and USB supported from LibNFC
bool CRfidThread::RFID_AuthenticateBlock(nfc_device &nfcdev, u8 keytype, u8 blockno)
{
    bool retcode =true;
    int res;

    QByteArray command(4, '\x00'), keycode(6, '\xFF');

    command.data()[0] =_InDataExchange;
    command.data()[1] =0x01;
    command.data()[2] =keytype;
    command.data()[3] =blockno;
    //[Not-Implemented]GetKeyCode(keycode);
    command.append(keycode);
    command.append(CardNo.left(4));
    WriteCommand(command, 14);
    //serial.write(command);

    const u8 * pbtTx = reinterpret_cast<const u8 *>(command.data());
    size_t szTx = command.length();
    nfc_device * pnd = &nfcdev;
    if ((res = nfc_initiator_transceive_bytes(pnd, pbtTx, szTx, abtRx, sizeof(abtRx), 0)) < 0)
      return false;
    //if(serial.waitForReadyRead(50))
    //{
        const char * c_abtRx = reinterpret_cast<const char *>(abtRx);  //(const_cast<uint8_t *>(pbtTx));
        QByteArray RxData = QByteArray(c_abtRx,res);
        //QByteArray RxData =serial.readAll();
        //while(serial.waitForReadyRead(100)) RxData +=serial.readAll();
        command =command.left(3);
        command.data()[0] =0xD5;
        command.data()[1] =0x41;
        command.data()[2] =0x00;
        qCDebug(Rfid) << "RFID_AuthenticateBlock with KEY(" << QString("0x%1").arg(keytype, 2, 16, QLatin1Char('0')) << ") ReadResponse: 0x" << RxData.toHex().toUpper();
        retcode =RxData.contains(command);
        if (!retcode) {
            QByteArray cmdResp(2, 0x0);
            cmdResp[0] = 0xD5;
            cmdResp[1] = 0x41;
            int cmdRespIdx = RxData.indexOf(cmdResp);
            if (cmdRespIdx >= 0)
                qCWarning(Rfid) << "RFID_AuthenticationBlock fails. Status/ErrorCode=" << RxData.mid(cmdRespIdx + 2, 1).toHex().toUpper();
            else
                qCWarning(Rfid) << "RFID_AuthenticateBlock fails";
        }
    //}

    return retcode;
}

//reimplemented with native UART, I2C, SPI and USB supported from LibNFC
bool CRfidThread::RFID_ReadBlock(nfc_device &nfcdev, u8 blockno, QByteArray &data)
{
    bool retcode =false;
    nfc_device * devptr = &nfcdev;

    int res = nfc_initiator_mifare_cmd(devptr, MC_READ, blockno, &mp);
    if(res){
        data = QByteArray::fromRawData(reinterpret_cast<const char *>(mp.mpd.abtData),16);
        retcode = true;
        qWarning() <<"RFID_ReadBlock ReadResponse: " << data.toHex().toUpper();
    }else{
        qWarning() << "RFID_ReadBlock fails ";
    }
    return retcode;
}


#if false // comment-out to avoid compile-warnings
void CRfidThread::GetKeyCode(QByteArray &keycode)
{ //根据卡号与公匙获取授权KEY 6BYTE
  //TODO:
}

bool CRfidThread::BlockDataIsAvailable(QByteArray &blockdata)
{
    bool retcode =false;
    return retcode;
}
#endif
