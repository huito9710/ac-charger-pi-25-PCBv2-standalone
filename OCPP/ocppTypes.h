#ifndef OCPPTYPES_H
#define OCPPTYPES_H

#include <array>
#include <chrono>
#include <memory>
#include <sstream>
#include <map>
#include <string>

#include <QByteArray>
#include <QDebug>
#include <QJsonObject>
#include <QJsonValue>

namespace ocpp {

    enum class RpcMessageType : int {
        Call = 2,
        CallResult = 3,
        CallError = 4
    };

    enum class CallActionType {
        Nil,
        Authorize,
        BootNotification,
        DataTransferTimeValue,
        DiagnosticsStatusNotification,
        FirmwareStatusNotification,
        Heartbeat,
        MeterValues,
        StartTransaction,
        StopTransaction,
        StatusNotification,
        CancelReservation,
        ChangeAvailability,
        ChangeConfiguration,
        ClearCache,
        ClearChargingProfile,
        GetCompositeSchedule,
        GetConfiguration,
        GetDiagnostics,
        GetLocalListVersion,
        RemoteStartTransaction,
        RemoteStopTransaction,
        ReserveNow,
        Reset,
        SendLocalList,
        SetChargingProfile,
        TriggerMessage,
        UnlockConnector,
        UpdateFirmware,
        DistanceSensing,
    };

    const std::map<CallActionType, std::string> callActionStrs = {
        { CallActionType::Nil, "" },
        { CallActionType::Authorize, "Authorize" },
        { CallActionType::BootNotification, "BootNotification" },
        { CallActionType::DataTransferTimeValue, "DataTransfer" },
        { CallActionType::DiagnosticsStatusNotification, "DiagnosticsStatusNotification" },
        { CallActionType::FirmwareStatusNotification, "FirmwareStatusNotification" },
        { CallActionType::Heartbeat, "Heartbeat" },
        { CallActionType::MeterValues, "MeterValues" },
        { CallActionType::StartTransaction, "StartTransaction" },
        { CallActionType::StopTransaction, "StopTransaction" },
        { CallActionType::StatusNotification, "StatusNotification" },
        { CallActionType::CancelReservation, "CancelReservation" },
        { CallActionType::ChangeAvailability, "ChangeAvailability" },
        { CallActionType::ChangeConfiguration, "ChangeConfiguration" },
        { CallActionType::ClearCache, "ClearCache" },
        { CallActionType::ClearChargingProfile, "ClearChargingProfile" },
        { CallActionType::GetCompositeSchedule, "GetCompositeSchedule" },
        { CallActionType::GetConfiguration, "GetConfiguration" },
        { CallActionType::GetDiagnostics, "GetDiagnostics" },
        { CallActionType::GetLocalListVersion, "GetLocalListVersion" },
        { CallActionType::RemoteStartTransaction, "RemoteStartTransaction" },
        { CallActionType::RemoteStopTransaction, "RemoteStopTransaction" },
        { CallActionType::ReserveNow, "ReserveNow" },
        { CallActionType::Reset, "Reset" },
        { CallActionType::SendLocalList, "SendLocalList" },
        { CallActionType::SetChargingProfile, "SetChargingProfile" },
        { CallActionType::TriggerMessage, "TriggerMessage" },
        { CallActionType::UnlockConnector, "UnlockConnector" },
        { CallActionType::UpdateFirmware, "UpdateFirmware" },
        { CallActionType::DistanceSensing, "DistanceSensing" }
    };

    std::istream& operator>>(std::istream& in, CallActionType &value);
    std::ostream& operator<<(std::ostream& out, const CallActionType value);
    QDebug operator<<(QDebug debug, const CallActionType value);


    // name is the same as actual value for 'std::boolalpha' conversion.
    enum class JsonField {
        idTag,
        limit,
        duration,
        chargingProfile,
        chargingProfilePurpose,
        chargingSchedule,
        chargingSchedulePeriod,
        transactionId,
        key,
        value,
        type,
        expiryDate,
        reservationId,
        csChargingProfiles,
        status,
        currentTime,
        interval,
        idTagInfo,
        parentIdTag,
        data,
        listVersion,
        localAuthorizationList,
        updateType,
        location,
        retries,
        retrieveDate,
        retryInterval,
        configurationKey,
        unknownKey,
        readonly,
        requestMessage,
        messageId,
        title,
        content,
        en,
        zhhk,
        popUpMsgduration,
        fontsize,
        titleColor,
        contentColor,
        vendorId,
        popUpMsgType,
        exit,
        color
    };

    const std::map<JsonField, std::string> jsonFieldStr = {
        { JsonField::idTag, "idTag" },
        { JsonField::limit, "limit" },
        { JsonField::duration, "duration" },
        { JsonField::chargingProfile, "chargingProfile" },
        { JsonField::chargingProfilePurpose, "chargingProfilePurpose"},
        { JsonField::chargingSchedule, "chargingSchedule" },
        { JsonField::chargingSchedulePeriod, "chargingSchedulePeriod" },
        { JsonField::transactionId, "transactionId" },
        { JsonField::key, "key" },
        { JsonField::value, "value" },
        { JsonField::type, "type" },
        { JsonField::expiryDate, "expiryDate" },
        { JsonField::reservationId, "reservationId" },
        { JsonField::csChargingProfiles, "csChargingProfiles" },
        { JsonField::status, "status" },
        { JsonField::currentTime, "currentTime" },
        { JsonField::interval, "interval" },
        { JsonField::idTagInfo, "idTagInfo" },
        { JsonField::parentIdTag, "parentIdTag" },
        { JsonField::data, "data" },
        { JsonField::listVersion, "listVersion" },
        { JsonField::localAuthorizationList, "localAuthorizationList" },
        { JsonField::updateType, "updateType" },
        { JsonField::location, "location" },
        { JsonField::retries, "retries" },
        { JsonField::retrieveDate, "retrieveDate" },
        { JsonField::retryInterval, "retryInterval" },
        { JsonField::configurationKey, "configurationKey"},
        { JsonField::unknownKey, "unknownKey"},
        { JsonField::readonly, "readonly" },
        { JsonField::requestMessage, "requestedMessage" },
        { JsonField::messageId, "messageId" },
        { JsonField::title, "title" },
        { JsonField::content, "content" },
        { JsonField::en, "en" },
        { JsonField::zhhk, "zh-hk" },
        { JsonField::popUpMsgduration, "duration" },
        { JsonField::fontsize, "fontsize" },
        { JsonField::titleColor, "titleColor" },
        { JsonField::contentColor, "contentColor" },
        { JsonField::vendorId, "vendorId" },
        { JsonField::popUpMsgType, "type" },
        { JsonField::exit, "exit" },
        { JsonField::color, "color" },
    };

    enum class ChargePointErrorCode {
        ConnectorLockFailure,
        EVCommunicationError,
        GroundFailure,
        HighTemperature,
        InternalError,
        LocalListConflict,
        NoError,
        OtherError,
        OverCurrentFailure,
        OverVoltage,
        PowerMeterFailure,
        PowerSwitchFailure,
        ReaderFailure,
        ResetFailure,
        UnderVoltage,
        WeakSignal
    };

    const std::map<ChargePointErrorCode, std::string> chargePointErrorCodeStrs = {
        { ChargePointErrorCode::ConnectorLockFailure, "ConnectorLockFailure" },
        { ChargePointErrorCode::EVCommunicationError, "EVCommunicationError" },
        { ChargePointErrorCode::GroundFailure, "GroundFailure" },
        { ChargePointErrorCode::HighTemperature, "HighTemperature" },
        { ChargePointErrorCode::InternalError, "InternalError" },
        { ChargePointErrorCode::LocalListConflict, "LocalListConflict" },
        { ChargePointErrorCode::NoError, "NoError" },
        { ChargePointErrorCode::OtherError, "OtherError" },
        { ChargePointErrorCode::OverCurrentFailure, "OverCurrentFailure" },
        { ChargePointErrorCode::OverVoltage, "OverVoltage" },
        { ChargePointErrorCode::PowerMeterFailure, "PowerMeterFailure" },
        { ChargePointErrorCode::PowerSwitchFailure, "PowerSwitchFailure" },
        { ChargePointErrorCode::ReaderFailure, "ReaderFailure" },
        { ChargePointErrorCode::ResetFailure, "ResetFailure" },
        { ChargePointErrorCode::UnderVoltage, "UnderVoltage" },
        { ChargePointErrorCode::WeakSignal, "WeakSignal" }

    };

    enum class ConfigurationKeys : int {
        MeterValueSampleInterval
    };
    const std::map<ConfigurationKeys, std::string> configurationKeyStrs = {
        { ConfigurationKeys::MeterValueSampleInterval, "MeterValueSampleInterval" }
    };

    struct KeyValue {
        QString key;
        QString value;
        bool readonly;
        KeyValue(const QString &k, const QString &v, bool r) {
            key = k;
            value = v;
            readonly = r;
        }
    };

    QJsonValue keyValueToJson(const KeyValue &kv);

    // This is used for fields not defined by OCPP.
    enum class NotdefinedStatus: int {
        Accepted,
    };
    const std::map<NotdefinedStatus, std::string> notdefinedStatusStrs = {
        { NotdefinedStatus::Accepted, "Accepted" }
    };

    enum class ChargingProfileStatus : int {
        Accepted,
        Rejected
    };
    const std::map<ChargingProfileStatus, std::string> chargingProfileStatusStrs = {
        { ChargingProfileStatus::Accepted, "Accepted" },
        { ChargingProfileStatus::Rejected, "Rejected" }
    };

    enum class RegistrationStatus : int {
        Accepted,
        Pending,
        Rejected,
        Unknown, // 'Unknown is not defined in OCPP. added for the sake of programming
    };

    enum class ReservationStatus: int {
        Accepted,
        Faulted,
        Occupied,
        Rejected,
        Unavailable
    };
    const std::map<ReservationStatus, std::string> reservationStatusStrs = {
        { ReservationStatus::Accepted, "Accepted" },
        { ReservationStatus::Faulted, "Faulted" },
        { ReservationStatus::Occupied, "Occupied" },
        { ReservationStatus::Rejected, "Rejected" },
        { ReservationStatus::Unavailable, "Unavailable" }
    };

    enum class CancelReservationStatus : int {
        Accepted,
        Rejected
    };
    const std::map<CancelReservationStatus, std::string> cancelReservationStatusStrs = {
        { CancelReservationStatus::Accepted, "Accepted" },
        { CancelReservationStatus::Rejected, "Rejected" }
    };

    enum class ResetStatus : int {
        Accepted,
        Rejected
    };
    const std::map<ResetStatus, std::string> resetStatusStrs = {
        { ResetStatus::Accepted, "Accepted" },
        { ResetStatus::Rejected, "Rejected" }
    };

    enum class ConfigurationStatus : int {
        Accepted,
        Rejected,
        RebootRequired,
        NotSupported
    };
    const std::map<ConfigurationStatus, std::string> configurationStatusStrs = {
        { ConfigurationStatus::Accepted, "Accepted" },
        { ConfigurationStatus::Rejected, "Rejected" },
        { ConfigurationStatus::RebootRequired, "RebootRequired" },
        { ConfigurationStatus::NotSupported, "NotSupported" }
    };

    enum class ClearCacheStatus : int {
        Accepted,
        Rejected
    };
    const std::map<ClearCacheStatus, std::string> clearCacheStatusStrs = {
        { ClearCacheStatus::Accepted, "Accepted" },
        { ClearCacheStatus::Rejected, "Rejected" }
    };

    enum class RemoteStartStopStatus : int {
        Accepted,
        Rejected
    };
    const std::map<RemoteStartStopStatus, std::string> remoteStartStopStatusStrs = {
        { RemoteStartStopStatus::Accepted, "Accepted" },
        { RemoteStartStopStatus::Rejected, "Rejected" }
    };

    enum class UnlockStatus : int {
        Unlocked,
        UnlockFailed,
        NotSupported
    };
    const std::map<UnlockStatus, std::string> unlockStatusStrs = {
        { UnlockStatus::Unlocked, "Unlocked" },
        { UnlockStatus::UnlockFailed, "UnlockedFailed" },
        { UnlockStatus::NotSupported, "NotSupported" }
    };

    enum class UpdateStatus: int {
        Accepted,
        Failed,
        NotSupported,
        VersionMismatch
    };
    const std::map<UpdateStatus, std::string> updateStatusStrs = {
        { UpdateStatus::Accepted, "Accepted" },
        { UpdateStatus::Failed, "Failed" },
        { UpdateStatus::NotSupported, "NotSupported" },
        { UpdateStatus::VersionMismatch, "VersionMismatch" },
    };

    enum class ChargingProfilePurposeType: int {
        ChargePointMaxProfile,
        TxDefaultProfile,
        TxProfile
    };

    std::istream& operator>>(std::istream& in, ChargingProfilePurposeType &value);

    enum class AvailabilityType: int {
        Inoperative,
        Operative,
    };
    std::istream& operator>>(std::istream& in, AvailabilityType &value);


    enum class AvailabilityStatus: int {
        Accepted,
        Rejected,
        Scheduled
    };
    const std::map<AvailabilityStatus, std::string> availabilityStatusStrs = {
      { AvailabilityStatus::Accepted, "Accepted" },
      { AvailabilityStatus::Rejected, "Rejected" },
      { AvailabilityStatus::Scheduled, "Scheduled" }
    };


    enum class ChargerStatus: int {
        Available,
        Preparing,
        Charging,
        SuspendedEV,
        SuspendedEVSE,
        Finishing,
        Reserved,
        Unavailable,
        Faulted
    };

    const std::map<ChargerStatus, std::string> ChargerStatusStrs = {
        { ChargerStatus::Available, "Available" },
        { ChargerStatus::Preparing, "Preparing" },
        { ChargerStatus::Charging, "Charging" },
        { ChargerStatus::SuspendedEV, "SuspendedEV" },
        { ChargerStatus::SuspendedEVSE, "SuspendedEVSE" },
        { ChargerStatus::Finishing, "Finishing" },
        { ChargerStatus::Reserved, "Reserved" },
        { ChargerStatus::Unavailable, "Unavailable" },
        { ChargerStatus::Faulted, "Faulted" },
    };

    enum class Reason : int {
        DeAuthorized,
        EmergencyStop,
        EVDisconnected,
        HardReset,
        Local,
        Other,
        PowerLoss,
        Reboot,
        Remote,
        SoftReset,
        UnlockCommand
    };

    const std::map<Reason, std::string> reasonStrs = {
        { Reason::DeAuthorized, "DeAuthorized" },
        { Reason::EmergencyStop, "EmergencyStop" },
        { Reason::EVDisconnected, "EVDisconnected" },
        { Reason::HardReset, "HardReset" },
        { Reason::Local, "Local" },
        { Reason::Other, "Other" },
        { Reason::PowerLoss, "PowerLoss" },
        { Reason::Reboot, "Reboot" },
        { Reason::Remote, "Remote" },
        { Reason::SoftReset, "SoftReset" },
        { Reason::UnlockCommand, "UnlockCommand" }
    };

    using Clock = std::chrono::system_clock;
    using DateTime = std::chrono::time_point<ocpp::Clock>;

    enum class AuthorizationStatus : int
    {
        Accepted = 1,
        Blocked,
        Expired,
        Invalid,
        ConcurrentTx,
        // Special purpose enum to iterate the status values
        _First = Accepted,
        _Last = ConcurrentTx
    };

    std::ostream& operator<<(std::ostream& out, const AuthorizationStatus value);
    std::istream& operator>>(std::istream& in, AuthorizationStatus &value);

    using IdToken = std::array<char,20>; // 20 characters representing hexa-decimal
    using LocalAuthorizationListVersion = int;
    constexpr LocalAuthorizationListVersion LocalAuthListNotAvail = 0;
    constexpr LocalAuthorizationListVersion LocalAuthListNotSupported = -1;
    struct IdTagInfo {
        IdTagInfo() {
            dateTime = DateTime::max(); // never-expired by default
            parentId.fill(0);
            authStatus = AuthorizationStatus::Invalid;
        }

        IdTagInfo(const IdTagInfo &other) {
            dateTime = other.dateTime;
            parentId = other.parentId;
            authStatus = other.authStatus;
        }

        bool isDateTimeUnset() const { return dateTime == DateTime::max(); }
        DateTime    dateTime;
        IdToken     parentId;
        AuthorizationStatus authStatus;
    };

    struct AuthorizationData {
        /** Copy Constructor **/
        AuthorizationData(AuthorizationData &ad) {
            this->idTag = ad.idTag;
            this->idTagInfo = std::make_unique<IdTagInfo>(*ad.idTagInfo);
        }

        /** Default Constructor **/
        AuthorizationData()
            :idTagInfo(nullptr)
        {}

        AuthorizationData(const QByteArray &id_)
            :idTagInfo(nullptr)
        {
            idTag.fill(0);
            std::copy(id_.begin(), id_.end(), idTag.begin());
        }

        /** Move Constructor **/
        AuthorizationData(AuthorizationData &&ad)
            : idTag(ad.idTag),
              idTagInfo(std::move(ad.idTagInfo))
        {}

        IdToken                      idTag;
        std::unique_ptr<IdTagInfo>   idTagInfo;
    };

    inline std::string idTokenToStdString(const IdToken &idToken) {
        // add a null-termination
        return std::string(idToken.data(), idToken.size());
    }


    enum class FirmwareStatus : int {
        Downloaded,
        DownloadFailed,
        Downloading,
        Idle,
        InstallationFailed,
        Installing,
        Installed
    };

    enum class TriggerMessageStatus: int {
        Accepted,
        Rejected,
        NotImplemented
    };
    const std::map<TriggerMessageStatus, std::string> TriggerMessageStatusStrs = {
        { TriggerMessageStatus::Accepted, "Accepted" },
        { TriggerMessageStatus::Rejected, "Rejected" },
        { TriggerMessageStatus::NotImplemented, "NotImplemented" }
    };

    std::ostream& operator<<(std::ostream& out, const FirmwareStatus value);

    enum class DataTransferStatus: int {
        Accepted,
        Rejected,
        NotImplemented
    };
    const std::map<DataTransferStatus, std::string> DataTransferStatusStrs = {
        { DataTransferStatus::Accepted, "Accepted" },
        { DataTransferStatus::Rejected, "Rejected" },
        { DataTransferStatus::NotImplemented, "NotImplemented" }
    };
}


#endif // OCPPTYPES_H
