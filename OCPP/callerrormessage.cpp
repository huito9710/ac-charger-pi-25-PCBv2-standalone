#include "callerrormessage.h"
#include "logconfig.h"
#include <utility>
#include <algorithm>
#include <vector>
#include <functional>

std::shared_ptr<ocpp::RpcMessage> ocpp::CallErrorMessage::fromMessageString(std::string recvStr)
{
     std::shared_ptr<ocpp::RpcMessage> msg(nullptr);
     const std::vector<std::string> itemPatterns = {
         R"(\s*)" R"((\d+))" R"(\s*)", // Message Type
         R"(\s*)" R"'("(\d+)")'" R"(\s*)", // Message Id
         R"(\s*)" R"'("([[:alpha:]]+)")'" R"(\s*)", // Error Code
         R"(\s*)" R"(\"([^"]+)\")" R"(\s*)", // Error Description
         "" // Json
     };
     try {
         // Try to break down the string into 5 items, separated by comma
         // [ msgType, msgId, error-code, error-description, json, ]
         const int numItems = 5;
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
         std::string errCodeStr = matchedItems[2];
         std::string errDescStr = matchedItems[3];
         std::string payloadStr = matchedItems[4];

         if(typeVal == static_cast<int>(ocpp::RpcMessageType::CallError)) {
             // instantiate
             msg = std::make_shared<ocpp::CallErrorMessage>(msgIdStr, errCodeStr, errDescStr, payloadStr);
         }
         else {
             throw std::runtime_error("Unexpected MessageType value: " + matchedItems[0]);
         }
     }
     catch (std::exception &e) {
         qCInfo(OcppComm) << e.what(); // To-be-improved: down-priority this because of the way these functions are called.
     }
     return msg;
}

ocpp::CallErrorMessage::CallErrorMessage(const std::string &id,
                 const std::string &errorCode,
                 const std::string &errorDesc,
                 const std::string &payload) :
    RpcMessage(ocpp::RpcMessageType::CallError, id, payload),
    errCode(errorCode),
    errDesc(errorDesc)
{}

void ocpp::CallErrorMessage::print(std::ostream& out) const
{
    out << "[" << static_cast<int>(getType()) << ",";
    out << "\"" << getId() << "\",";
    out << "\"" << errCode << "\",";
    out << "\"" << errDesc << "\",";
    if (hasPayload())
        out << getPayload();
    else
        out << "{}"; // OCPP recommends empty-object (see OCPP-1.6 JSON spec)
    out << "]";
}

std::ostream& ocpp::operator<<(std::ostream& out, const ocpp::CallErrorMessage &msg)
{
    msg.print(out);
    return out;
}
