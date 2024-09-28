#include "callmessage.h"
#include "logconfig.h"
#include <utility>
#include <algorithm>
#include <vector>
#include <functional>

std::shared_ptr<ocpp::RpcMessage> ocpp::CallMessage::fromMessageString(std::string recvStr)
{
     std::shared_ptr<ocpp::RpcMessage> msg(nullptr);

    const std::vector<std::string> itemPatterns = {
        R"(\s*)" R"((\d+))" R"(\s*)", // Message Type
//        R"(\s*)" R"'("(\d+)")'" R"(\s*)", // Message Id
        R"(\s*)" R"'("([A-Za-z\-0-9]+)")'" R"(\s*)", // Message Id
        R"(\s*)" R"(\"([^"]+)\")" R"(\s*)", // Action Id
        "" // Json
    };

     try {
         // Try to break down the string into 4 items, separated by comma
         // [ msgType, msgId, action, json]
         const int numItems = 4;
         std::vector<std::pair<size_t,size_t>> items;
         if (! messageToItems(recvStr, numItems, items))
             throw std::runtime_error("Parse error in OCPP-Message");
         std::vector<std::string> matchedItems(4);
         MsgPatternMatcher mpm(recvStr);
         using namespace std::placeholders;
         auto ssmBound = std::bind(&MsgPatternMatcher::subStrMatch, &mpm, _1, _2);
         std::transform(items.begin(), items.end(), itemPatterns.begin(), matchedItems.begin(), ssmBound);
         // Msg Type
         int typeVal = -1;
         if (!matchedItems[0].empty())
            std::stringstream(matchedItems[0]) >> typeVal;
         else
             throw std::runtime_error("Cannot find valid Msg-Type field");
         // Msg Id
         std::string msgIdStr = matchedItems[1];
         if (msgIdStr.empty())
             throw std::runtime_error("Cannot find valid Msg-Id field");
         // Action
         std::string actionStr = matchedItems[2];
         if (actionStr.empty())
             throw std::runtime_error("Cannot find valid Action field");
         // Json Payload
         std::string payloadStr = matchedItems[3];
         if (payloadStr.empty())
             throw std::runtime_error("Cannot find valid Payload field");
         if(typeVal == static_cast<int>(ocpp::RpcMessageType::Call)) {
             // instantiate
             msg = std::make_shared<ocpp::CallMessage>(msgIdStr, actionStr, payloadStr);
         }
         else {
             throw std::runtime_error("Unexpected MessageType value: " + std::string(msgIdStr));
         }

     }
     catch (std::exception &e) {
         qCInfo(OcppComm) << e.what(); // To-be-improved: down-priority this because of the way these functions are called.
     }

     return msg;
}


ocpp::CallMessage::CallMessage(const std::string &id,
                               const std::string &action_, const std::string &payload)
    : RpcMessage(ocpp::RpcMessageType::Call, id, payload),
      action(action_)
{ }

ocpp::CallMessage::CallMessage(const QString id, const CallActionType action, const QString &payload)
    : RpcMessage(ocpp::RpcMessageType::Call, id.toStdString(), payload.toStdString()),
      action(ocpp::callActionStrs.at(action))
{ }

void ocpp::CallMessage::print(std::ostream& out) const
{
    out << "[" << static_cast<int>(getType()) << ",";
    out << "\"" << getId() << "\",";
    out << "\"" << action << "\",";
    if (hasPayload())
        out << getPayload();
    else
        out << "{}"; // OCPP recommends empty-object (see OCPP-1.6 JSON spec)
    out << "]";
}

std::ostream& ocpp::operator<<(std::ostream& out, const ocpp::CallMessage &msg)
{
    msg.print(out);
    return out;
}
