#ifndef OCPPCONST_H
#define OCPPCONST_H

//=============================================================================
//#define _MeterValues        "[2,\"%1\",\"MeterValues\",{\"connectorId\":%2,\"transactionId\":%3,\"meterValue\":{\"timestamp\":\"%4\",\"sampledValue\":{\"value\":\"%5\"}}}]"

#define _Authorize          "{\"idTag\":\"%1\"}"
#define _BootNotification   "{\"chargePointVendor\":\"%1\",\"chargePointModel\":\"%2\",\"chargePointSerialNumber\":\"%3\",\"firmwareVersion\":\"%4\"}"
//#define _MeterValues      "[2,\"%1\",\"MeterValues\",{\"connectorId\":%2,\"transactionId\":%3,\"meterValue\":[{\"timestamp\":\"%4\",\"sampledValue\":[{\"value\":\"%5\",\"unit\":\"kWh\"}]}]}]"
#define _MeterValues        "{\"connectorId\":%1,\"transactionId\":%2,\"meterValue\":[{\"timestamp\":\"%3\",\"sampledValue\":[    \
                            {\"value\":\"%4\",\"measurand\":\"Energy.Active.Import.Register\",\"unit\":\"%5\"},{\"value\":\"%6\",\"measurand\":\"Power.Active.Import\",\"unit\":\"W\"},    \
                            {\"value\":\"%7\",\"measurand\":\"Current.Import\",\"phase\":\"L1\",\"unit\":\"A\"},    \
                            {\"value\":\"%8\",\"measurand\":\"Current.Import\",\"phase\":\"L2\",\"unit\":\"A\"},    \
                            {\"value\":\"%9\",\"measurand\":\"Current.Import\",\"phase\":\"L3\",\"unit\":\"A\"},    \
                            {\"value\":\"%10\",\"measurand\":\"Voltage\",\"phase\":\"L1-N\",\"unit\":\"V\"},        \
                            {\"value\":\"%11\",\"measurand\":\"Voltage\",\"phase\":\"L2-N\",\"unit\":\"V\"},        \
                            {\"value\":\"%12\",\"measurand\":\"Voltage\",\"phase\":\"L3-N\",\"unit\":\"V\"},        \
                            {\"value\":\"%13\",\"measurand\":\"Power.Factor\"}, \
                            {\"value\":\"%14\",\"measurand\":\"Frequency\"}]}]}"
#define _MeterValues_w_Intr        "{\"connectorId\":%1,\"transactionId\":%2,\"meterValue\":[{\"timestamp\":\"%3\",\"sampledValue\":[    \
                            {\"value\":\"%4\",\"measurand\":\"Energy.Active.Import.Register\",\"unit\":\"%5\"},{\"value\":\"%6\",\"measurand\":\"Power.Active.Import\",\"unit\":\"W\"},    \
                            {\"value\":\"%7\",\"context\": \"Interruption\",\"measurand\":\"Current.Import\",\"phase\":\"L1\",\"unit\":\"A\"},    \
                            {\"value\":\"%8\",\"context\": \"Interruption\",\"measurand\":\"Current.Import\",\"phase\":\"L2\",\"unit\":\"A\"},    \
                            {\"value\":\"%9\",\"context\": \"Interruption\",\"measurand\":\"Current.Import\",\"phase\":\"L3\",\"unit\":\"A\"},    \
                            {\"value\":\"%10\",\"measurand\":\"Voltage\",\"phase\":\"L1-N\",\"unit\":\"V\"},        \
                            {\"value\":\"%11\",\"measurand\":\"Voltage\",\"phase\":\"L2-N\",\"unit\":\"V\"},        \
                            {\"value\":\"%12\",\"measurand\":\"Voltage\",\"phase\":\"L3-N\",\"unit\":\"V\"},        \
                            {\"value\":\"%13\",\"measurand\":\"Power.Factor\"}, \
                            {\"value\":\"%14\",\"measurand\":\"Frequency\"}]}]}"
#define _StartTransaction   "{\"connectorId\":%1,\"idTag\":\"%2\",\"meterStart\":%3,\"timestamp\":\"%4\"}"
#define _StartTransactionwithReserve   "{\"connectorId\":%1,\"idTag\":\"%2\",\"meterStart\":%3,\"timestamp\":\"%4\",\"reservationId\":%5}"
#define _StatusNotification "{\"connectorId\":%1,\"errorCode\":\"%2\",\"status\":\"%3\",\"vendorErrorCode\":\"%4\"}"
//#define _StatusNotification "[2,\"%1\",\"StatusNotification\",{\"connectorId\":%2,\"errorCode\":\"%3\",\"status\":\"%4\"}]"
#define _StopTransaction    "{\"idTag\":\"%1\",\"meterStop\":%2,\"timestamp\":\"%3\",\"transactionId\":%4,\"reason\":\"%5\"}"
#define _DataTransfer       "{\"vendorId\":\"evMega\",\"messageId\":\"chargeTime\",\"data\":{\"chargeTime\":\"%1mins\",\"idTag\":\"%2\"}}"
#define _DataTransfer_w_Parking     "{\"vendorId\":\"evMega\",\"messageId\":\"distanceSensing\",\"data\":{\"idTag\":\"%1\",\"parkingState\":\"%2\",\"distance\":\"%3\"}}"
#define _SimplyStatusJson   "{\"status\":\"%1\"}"
#define _R_GetLocalListVersion "{\"listVersion\":%1}"
//=============================================================================

#endif // OCPPCONST_H
