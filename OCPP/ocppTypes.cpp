#include "OCPP/ocppTypes.h"
#include <iostream>
#include <unordered_map>

namespace ocpp {
    std::ostream& operator<<(std::ostream& out, const AuthorizationStatus value){
        static std::map<AuthorizationStatus, std::string> strs {
            {AuthorizationStatus::Accepted, "Accepted"},
            {AuthorizationStatus::Blocked, "Blocked"},
            {AuthorizationStatus::Expired, "Expired"},
            {AuthorizationStatus::Invalid, "Invalid"},
            {AuthorizationStatus::ConcurrentTx, "ConcurrentTx"},
        };

        return out << strs.at(value);
    }

    std::istream& operator>>(std::istream& in, AuthorizationStatus &value)
    {
        static std::unordered_map<std::string, AuthorizationStatus> statusTable {
            {"Accepted", AuthorizationStatus::Accepted},
            {"Blocked", AuthorizationStatus::Blocked},
            {"Expired", AuthorizationStatus::Expired},
            {"Invalid", AuthorizationStatus::Invalid},
            {"ConcurrentTx", AuthorizationStatus::ConcurrentTx}
        };
        std::string inStr;
        in >> inStr;
        value = statusTable.at(inStr);
        return in;
    }

    std::ostream& operator<<(std::ostream& out, const FirmwareStatus value){
        static std::map<FirmwareStatus, std::string> strs {
            {FirmwareStatus::Downloaded, "Downloaded"},
            {FirmwareStatus::DownloadFailed, "DownloadFailed"},
            {FirmwareStatus::Downloading, "Downloading"},
            {FirmwareStatus::Idle, "Idle"},
            {FirmwareStatus::InstallationFailed, "InstallationFailed"},
            {FirmwareStatus::Installing, "Installing"},
            {FirmwareStatus::Installed, "Installed"},
        };

        return out << strs.at(value);
    }

    std::istream& operator>>(std::istream& in, ChargingProfilePurposeType &value) {
        static std::unordered_map<std::string, ChargingProfilePurposeType> purposeTable = {
            { "ChargePointMaxProfile", ChargingProfilePurposeType::ChargePointMaxProfile },
            { "TxDefaultProfile", ChargingProfilePurposeType::TxDefaultProfile },
            { "TxProfile", ChargingProfilePurposeType::TxProfile }
        };
        std::string inStr;
        in >> inStr;
        value = purposeTable.at(inStr);
        return in;
    }

    std::istream& operator>>(std::istream& in, AvailabilityType &value) {
        static std::unordered_map<std::string, AvailabilityType> availTypeTable = {
            { "Inoperative", AvailabilityType::Inoperative },
            { "Operative", AvailabilityType::Operative }
        };
        std::string inStr;
        in >> inStr;
        value = availTypeTable.at(inStr);
        return in;
    }

    std::istream& operator>>(std::istream& in, CallActionType &value) {
       // handle not-matched?
       static std::unordered_map<std::string, CallActionType> actionTable = {
           { "Authorize", CallActionType::Authorize },
           { "BootNotification", CallActionType::BootNotification },
           { "DataTransfer", CallActionType::DataTransferTimeValue },
           { "DiagnosticsStatusNotification", CallActionType::DiagnosticsStatusNotification },
           { "FirmwareStatusNotification", CallActionType::FirmwareStatusNotification },
           { "Heartbeat", CallActionType::Heartbeat },
           { "MeterValues", CallActionType::MeterValues },
           { "StartTransaction", CallActionType::StartTransaction },
           { "StopTransaction", CallActionType::StopTransaction },
           { "StatusNotification", CallActionType::StatusNotification },
           { "CancelReservation", CallActionType::CancelReservation },
           { "ChangeAvailability", CallActionType::ChangeAvailability },
           { "ChangeConfiguration", CallActionType::ChangeConfiguration },
           { "ClearCache", CallActionType::ClearCache },
           { "ClearChargingProfile", CallActionType::ClearChargingProfile },
           { "GetCompositeSchedule", CallActionType::GetCompositeSchedule },
           { "GetConfiguration", CallActionType::GetConfiguration },
           { "GetDiagnostics", CallActionType::GetDiagnostics },
           { "GetLocalListVersion", CallActionType::GetLocalListVersion },
           { "RemoteStartTransaction", CallActionType::RemoteStartTransaction },
           { "RemoteStopTransaction", CallActionType::RemoteStopTransaction },
           { "ReserveNow", CallActionType::ReserveNow },
           { "Reset", CallActionType::Reset },
           { "SendLocalList", CallActionType::SendLocalList },
           { "SetChargingProfile", CallActionType::SetChargingProfile },
           { "TriggerMessage", CallActionType::TriggerMessage },
           { "UnlockConnector", CallActionType::UnlockConnector },
           { "UpdateFirmware", CallActionType::UpdateFirmware }

       };
       value = CallActionType::Nil;
       std::string inStr;
       in >> inStr;
       try {
           value = actionTable.at(inStr);
       }
       catch (std::out_of_range) {
           value = CallActionType::Nil;
       }

       return in;
    }

    std::ostream& operator<<(std::ostream& out, const CallActionType value)
    {
        try {
            out << callActionStrs.at(value);
        }
        catch (std::out_of_range &outRangeEx) {
            out << "Undefined string for Ocpp CallActionType enum value: " << static_cast<int>(value);
        }

        return out;
    }

    QDebug operator<<(QDebug debug, const CallActionType value)
    {
        QDebugStateSaver saver(debug);
        std::ostringstream oss;
        oss << value;
        debug.nospace() << QString::fromStdString(oss.str());
        return debug;
    }

    QJsonValue keyValueToJson(const KeyValue &kv)
    {
        QJsonObject jObj;
        jObj.insert(jsonFieldStr.at(JsonField::key).data(), kv.key);
        jObj.insert(jsonFieldStr.at(JsonField::value).data(), kv.value);
        jObj.insert(jsonFieldStr.at(JsonField::readonly).data(), kv.readonly);
        return jObj;
    }

}
