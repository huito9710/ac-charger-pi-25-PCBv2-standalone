#ifndef CALLRESULTMESSAGE_H
#define CALLRESULTMESSAGE_H

#include "OCPP/rpcmessage.h"

namespace ocpp {
    class CallResultMessage : public RpcMessage
    {
    public:
        static std::shared_ptr<ocpp::RpcMessage> fromMessageString(std::string recvStr);
    public:
        CallResultMessage(const std::string &id, const std::string &payload);
        CallResultMessage(const QString &id, const QString &payload);
        void print(std::ostream &out) const override;
    private:
        // Nothing to add
    };

    std::ostream& operator<<(std::ostream& out, const ocpp::CallResultMessage &msg);
}

#endif // CALLRESULTMESSAGE_H
