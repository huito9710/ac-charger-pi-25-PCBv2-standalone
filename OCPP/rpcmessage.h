#ifndef RPCMESSAGE_H
#define RPCMESSAGE_H

#include <string>
#include <regex>
#include "OCPP/ocppTypes.h"

namespace ocpp {

    class RpcMessage
    {
    public:
        virtual ~RpcMessage() = default;
        static bool findMatches(const std::string &inStr, const std::string &patternStr, std::smatch &matches);

        RpcMessageType getType() const { return type; };
        std::string getId() const  { return id; };
        bool hasPayload() const { return (payload.size() > 0); }
        std::string getPayload() const { return payload; };
        virtual void print(std::ostream &out) const;

    protected:
        RpcMessage(RpcMessageType type_, const std::string &id_, const std::string &payload_);
        static bool messageToItems(const std::string &msgStr, const int numItems, std::vector<std::pair<size_t, size_t>> &items);

    private:
        RpcMessageType type;
        std::string id;
        std::string payload;

    };

    std::ostream& operator<<(std::ostream& out, const ocpp::RpcMessage &msg);


    class MsgPatternMatcher {
        public:
        MsgPatternMatcher(std::string &msgStr);
        std::string subStrMatch(std::pair<size_t, size_t> idices, std::string patternStr);

        private:
        std::string s;
    };

}

#endif // RPCMESSAGE_H
