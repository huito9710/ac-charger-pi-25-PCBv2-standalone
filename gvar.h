#ifndef GVAR_H
#define GVAR_H

#include <QMainWindow>
#include <QKeyEvent>
#include "ccharger.h"
#include "const.h"
#include "chargerconfig.h"
#include "evchargingtransactionregistry.h"
#include "OCPP/ocppclient.h"

extern QMainWindow *pCurWin;
extern QMainWindow *pMsgWin;
extern QMainWindow *pVideoWin;
extern CCharger    &m_ChargerA;
extern OcppClient  &m_OcppClient;
extern QString      m_ErrCode;
extern u8           m_VideoNo;
extern ulong        m_DelayTimer;
extern uint         isSuspended;

#endif // GVAR_H
