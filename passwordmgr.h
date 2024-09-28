#ifndef PASSWORDMGR_H
#define PASSWORDMGR_H

#include <string>

class PasswordMgr
{
public:
    PasswordMgr();
    bool IsPasswordPresent();
    std::string GetPassword();
    bool SetPassword(const std::string &newPassword);
    bool DeletePassword();
private:
    bool loadPasswordFromFile();
    bool savePasswordToFile();
private:
    std::string password;
    const std::string passwordFilePath;
};

class PasswordMgrTest
{
public:
    static void Test1();
};


#endif // PASSWORDMGR_H
