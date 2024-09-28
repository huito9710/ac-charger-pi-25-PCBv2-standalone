#ifndef API_FUNCTION_H
#define API_FUNCTION_H
#include <QtCore/QString>
#include "T_Type.h"
#include <QFile>

bool GetSetting(const QString&, QString&);
u16	GetCRC16(u8 data[], int len);
bool IsValidCard(const QString& CardNo);
void RemoveOldRecords(QFile &file, const int minNumRecs);

#endif // API_FUNCTION_H
