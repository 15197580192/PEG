//
// Created by KG on 2022/3/15.
//

#ifndef PEG_CONNECTOR_H
#define PEG_CONNECTOR_H

#include <curl/curl.h>
#include <string>
#include <cstring>
#include <iostream>

#define defaultServerIP "127.0.0.1"
#define defaultServerPort 22314

class PEG_Connector
{
public:
    std::string serverIP;
    int serverPort;
    std::string Url;
    std::string username;
    std::string password;

    PEG_Connector(void);
    PEG_Connector(std::string _ip, int _port, std::string _user, std::string _passwd);
    ~PEG_Connector(void);

    int Get(const std::string &strUrl, std::string &strResponse);

    std::string build(std::string db_name, std::string db_path, std::string part_path, std::string request_type = "GET");
    std::string load(std::string db_name, std::string request_type = "GET");
    std::string deleteDB(std::string db_name, bool is_backup, std::string request_type = "GET");
    std::string query(std::string db_name, std::string sparql, std::string format = "json", std::string request_type = "GET");

private:
    bool m_bDebug;

};

#endif //PEG_CONNECTOR_H
