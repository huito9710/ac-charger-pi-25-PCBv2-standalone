#include <string>
#include <iostream>
#include <fstream>
#include "passwordmgr.h"
#include "hostplatform.h"

PasswordMgr::PasswordMgr()
    : passwordFilePath(QString("%1password.lst").arg(charger::Platform::instance().passwordDir).toStdString())
{
    using namespace std;
    if (! loadPasswordFromFile())
    {
        cerr << "PasswordMgr instantiation failed: Error loading Password from File." << endl;
    }
}

bool PasswordMgr::IsPasswordPresent()
{
    return (password.length() > 0);
}

std::string PasswordMgr::GetPassword()
{
    return password;
}

bool PasswordMgr::SetPassword(const std::string &newPassword)
{
    password.assign(newPassword);

    /* Save to File */
    bool saved = savePasswordToFile();
    return saved;
}

bool PasswordMgr::DeletePassword()
{
    password.clear();
    bool deleted = savePasswordToFile();

    return deleted;
}

// Return false if file present but unable to read.
bool PasswordMgr::loadPasswordFromFile()
{
    bool readOK = true;
    using namespace std;
    // Only read the first line of the password file.
    ifstream inf(passwordFilePath, ios::in);
    if (inf.is_open())
    {
        string readStr;
        inf >> readStr; // Read consecutive characters from beginning of the file, stopped at whitespace or equivalence.
        if (inf.eof() || inf.good()) {
            password = readStr;
        }
        else {
            readOK = false;
            cerr << "Load password failed: " << strerror(errno) << endl;
        }
        inf.close();
    }
    else {
        cerr << "Unable to open password file for reading: " << strerror(errno) << endl;
    }
    return readOK;
}

// Return false if file present but unable to write, or unable to open file for writing
bool PasswordMgr::savePasswordToFile()
{
    bool writeOK = false;
    using namespace std;
    // Overwrite existing file, if already exist
    ofstream outf(passwordFilePath, ios::out|ios::trunc);
    if (outf.is_open()) {
        outf << password;
        if (outf.good())
            writeOK = true;
        else
            cerr << "Save password failed: " << strerror(errno) << endl;
        outf.close();
    }
    else {
        cerr << "Unable to open password file for writing: " << strerror(errno) << endl;
    }
    return writeOK;
}


void PasswordMgrTest::Test1()
{
    using namespace std;
    PasswordMgr passwordMgr;
    cout << ios::boolalpha << "PasswordMgr password present? " << passwordMgr.IsPasswordPresent() << endl;
    cout << "PasswordMgr current password: " << passwordMgr.GetPassword() << endl;
    cout << "PasswordMgr set new password: ";
    std::string newPassword = "1234abcd";
    //getline (cin, newPassword);
    cout << "Saving new password to file..." << endl;
    bool newPasswordSaved = passwordMgr.SetPassword(newPassword);
    cout << ios::boolalpha <<  "PasswordMgr set new password: " << newPasswordSaved << endl;
}

