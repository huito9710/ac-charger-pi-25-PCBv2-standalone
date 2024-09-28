#include "rpcmessage.h"
#include "logconfig.h"

#include <iostream>
#include <string>
#include <regex>
#include <cassert>
#include <QDebug>

ocpp::RpcMessage::RpcMessage(RpcMessageType type_, const std::string &id_,
                             const std::string &payload_) :
    type(type_),
    id(id_),
    payload(payload_)
{}

bool ocpp::RpcMessage::findMatches(const std::string &inStr, const std::string &patternStr, std::smatch &matches)
{
    bool matched = false;
    std::regex pattern {patternStr};
    qCInfo(OcppComm) << "findMatches of " << QString::fromStdString(patternStr)
             << " in " << QString::fromStdString(inStr);
    if (std::regex_match(inStr, pattern)) {
        assert(std::regex_search(inStr, matches, pattern));
        qCInfo(OcppComm) << matches[1].str().c_str();
        matched = true;
    }

    return matched;
}

void ocpp::RpcMessage::print(std::ostream &out) const
{
    Q_UNUSED(out);
    Q_ASSERT(0); // not expected to be called here.
}

std::ostream& ocpp::operator<<(std::ostream& out, const ocpp::RpcMessage &msg)
{
    msg.print(out);
    return out;
}


ocpp::MsgPatternMatcher::MsgPatternMatcher(std::string &msgStr)
    : s(msgStr)
{}

std::string ocpp::MsgPatternMatcher::subStrMatch(std::pair<size_t, size_t> idices, std::string patternStr)
{
    std::smatch m;
    std::string ss = s.substr(idices.first, idices.second - idices.first + 1);
    std::string ms;
    if (patternStr.empty())
        ms = ss;
    else if (RpcMessage::findMatches(ss, patternStr, m))
        ms = m[1];
    return ms;
}

bool ocpp::RpcMessage::messageToItems(const std::string &msgStr, const int numItems, std::vector<std::pair<size_t, size_t>> &items)
{
    bool converted = false;

    try {
        // Match the begin and end square bracket
        size_t sqBegin = msgStr.find_first_of('[');
        size_t sqEnd = msgStr.find_last_of(']');
        if (sqBegin == std::string::npos || sqEnd == std::string::npos || sqEnd < sqBegin) {
            throw std::runtime_error("Square Bracket Matching Failed");
        }
        // Try to break down the string into items, separated by comma
        // e.g. [ msgType, msgId, action, json ]
        size_t commaSearchBeginPos = sqBegin+1;
        for (int i = 0; i < (numItems-1); i++) {
            size_t nextCommaPos = msgStr.find(',', commaSearchBeginPos);
            if (nextCommaPos == std::string::npos) {
                throw std::runtime_error("Unable to find next comma");
            }
            items.push_back(std::make_pair(commaSearchBeginPos, nextCommaPos-1));
            commaSearchBeginPos = nextCommaPos+1;
        }
        items.push_back(std::make_pair(commaSearchBeginPos, sqEnd-1));

        converted = (items.size() == static_cast<size_t>(numItems));
    }
    catch (std::exception &e) {
        qCInfo(OcppComm) << e.what(); // To-be-improved: down-priority this because of the way these functions are called.
    }

    return converted;
}
