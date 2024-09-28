#ifndef CALLMESSAGE_H
#define CALLMESSAGE_H

#include "OCPP/rpcmessage.h"

namespace ocpp {
    class CallMessage : public RpcMessage
    {
    public:
        static std::shared_ptr<ocpp::RpcMessage> fromMessageString(std::string recvStr);

    public:
        CallMessage(const std::string &id,
                    const std::string &action, const std::string &payload);
        CallMessage(const QString id, const CallActionType action, const QString &payload);
        std::string getAction() const { return action; }
        void print(std::ostream &out) const override;
    private:
        std::string action;
    };

    std::ostream& operator<<(std::ostream& out, const ocpp::CallMessage &msg);
}

#endif // CALLMESSAGE_H
