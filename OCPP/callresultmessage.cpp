#include "callresultmessage.h"
#include "logconfig.h"
#include <QDebug>
#include <utility>
#include <algorithm>
#include <vector>
#include <functional>

std::shared_ptr<ocpp::RpcMessage> ocpp::CallResultMessage::fromMessageString(std::string recvStr)
{
     std::shared_ptr<ocpp::RpcMessage> msg(nullptr);

     const std::vector<std::string> itemPatterns = {
         R"(\s*)" R"((\d+))" R"(\s*)", // Message Type
         R"(\s*)" R"'("(\d+)")'" R"(\s*)", // Message Id
         "" // Json
     };

     try {
         // Try to break down the string into 3 items, separated by comma
         // [ msgType, msgId, json]
         const int numItems = 3;
         std::vector<std::pair<size_t,size_t>> items;
         if (! messageToItems(recvStr, numItems, items))
             throw std::runtime_error("Parse error in OCPP-Message");
         std::vector<std::string> matchedItems(4);
         MsgPatternMatcher mpm(recvStr);
         using namespace std::placeholders;
         auto ssmBound = std::bind(&MsgPatternMatcher::subStrMatch, &mpm, _1, _2);
         std::transform(items.begin(), items.end(), itemPatterns.begin(), matchedItems.begin(), ssmBound);
         int typeVal = -1;
         if (!matchedItems[0].empty())
            std::stringstream(matchedItems[0]) >> typeVal;
         else
             throw std::runtime_error("Cannot find valid Msg-Type field");
         std::string msgIdStr = matchedItems[1];
         if (msgIdStr.empty())
             throw std::runtime_error("Cannot find valid Msg-Id field");
         std::string payloadStr = matchedItems[2];
         if (payloadStr.empty())
             throw std::runtime_error("Cannot find valid Payload field");
         if(typeVal == static_cast<int>(ocpp::RpcMessageType::CallResult)) {
             // instantiate
             msg = std::make_shared<ocpp::CallResultMessage>(msgIdStr, payloadStr);
         }
         else {
             qCDebug(OcppComm) << msgIdStr.c_str() << ", " << payloadStr.c_str();
             throw std::runtime_error("Unexpected MessageType value: " + matchedItems[0]);
         }
     }
     catch (std::exception &e) {
         qCInfo(OcppComm) << e.what(); // // To-be-improved: down-priority this because of the way these functions are called.
     }
     return msg;
}

ocpp::CallResultMessage::CallResultMessage(const std::string &id, const std::string &payload)
    : RpcMessage(ocpp::RpcMessageType::CallResult, id, payload)
{}

ocpp::CallResultMessage::CallResultMessage(const QString &id, const QString &payload)
    : RpcMessage(ocpp::RpcMessageType::CallResult, id.toStdString(), payload.toStdString())
{ }


void ocpp::CallResultMessage::print(std::ostream& out) const
{
    out << "[" << static_cast<int>(getType()) << ",";
    out << "\"" << getId() << "\",";
    if (hasPayload())
        out << getPayload();
    else
        out << "{}"; // OCPP recommends empty-object (see OCPP-1.6 JSON spec)
    out << "]";
}

std::ostream& ocpp::operator<<(std::ostream& out, const ocpp::CallResultMessage &msg)
{
    msg.print(out);
    return out;
}
