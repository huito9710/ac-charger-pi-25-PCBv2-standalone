#ifndef CALLERRORMESSAGE_H
#define CALLERRORMESSAGE_H

#include "OCPP/rpcmessage.h"

namespace ocpp {
    class CallErrorMessage : public RpcMessage
    {
    public:
        static std::shared_ptr<ocpp::RpcMessage> fromMessageString(std::string recvStr);

    public:
        CallErrorMessage(const std::string &id,
                         const std::string &errorCode,
                         const std::string &errorDesc,
                         const std::string &payload);

        std::string getErrorCode() const { return errCode; };
        std::string getErrorDescription() const { return errDesc; };
        void print(std::ostream &out) const override;

    private:
        std::string errCode;
        std::string errDesc;
    };

    std::ostream& operator<<(std::ostream& out, const ocpp::CallErrorMessage &msg);
}

#endif // CALLERRORMESSAGE_H
