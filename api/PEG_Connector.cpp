//
// Created by KG on 2022/3/15.
//

#include "PEG_Connector.h"

PEG_Connector::PEG_Connector(void) :
        m_bDebug(false)
{

}

PEG_Connector::PEG_Connector(std::string _ip, int _port, std::string _user, std::string _passwd) :
        m_bDebug(false)
{
    if (_ip == "localhost")
        this->serverIP = defaultServerIP;
    else
        this->serverIP = _ip;
    this->serverPort = _port;
    this->Url = "http://" + this->serverIP + ":" + std::to_string(this->serverPort) + "/";
    this->username = _user;
    this->password = _passwd;
}
PEG_Connector::~PEG_Connector(void)
{

}

static const std::string UrlEncode(const std::string& s)
{
    std::string ret;
    unsigned char* ptr = (unsigned char*)s.c_str();
    ret.reserve(s.length());

    for (int i = 0; i < s.length(); ++i)
    {
        if ((int(ptr[i]) == 42) || (int(ptr[i]) == 45) || (int(ptr[i]) == 46) || (int(ptr[i]) == 47) || (int(ptr[i]) == 58) || (int(ptr[i]) == 95))
            ret += ptr[i];
        else if ((int(ptr[i]) >= 48) && (int(ptr[i]) <= 57))
            ret += ptr[i];
        else if ((int(ptr[i]) >= 65) && (int(ptr[i]) <= 90))
            ret += ptr[i];
        else if ((int(ptr[i]) >= 97) && (int(ptr[i]) <= 122))
            ret += ptr[i];
        else if (int(ptr[i]) == 32)
            ret += '+';
        else if ((int(ptr[i]) != 9) && (int(ptr[i]) != 10) && (int(ptr[i]) != 13))
        {
            char buf[5];
            memset(buf, 0, 5);
            snprintf(buf, 5, "%%%X", ptr[i]);
            ret.append(buf);
        }
    }
    return ret;
}

static size_t OnWriteData(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    std::string* str = dynamic_cast<std::string*>((std::string*)lpVoid);
    if (NULL == str || NULL == buffer)
    {
        return -1;
    }

    char* pData = (char*)buffer;
    str->append(pData, size * nmemb);
    return nmemb;
}

static int OnDebug(CURL*, curl_infotype itype, char* pData, size_t size, void*)
{
    if (itype == CURLINFO_TEXT)
    {
        printf("[TEXT]%s\n", pData);
    }
    else if (itype == CURLINFO_HEADER_IN)
    {
        printf("[HEADER_IN]%s\n", pData);
    }
    else if (itype == CURLINFO_HEADER_OUT)
    {
        printf("[HEADER_OUT]%s\n", pData);
    }
    else if (itype == CURLINFO_DATA_IN)
    {
        printf("[DATA_IN]%s\n", pData);
    }
    else if (itype == CURLINFO_DATA_OUT)
    {
        printf("[DATA_OUT]%s\n", pData);
    }
    return 0;
}

int PEG_Connector::Get(const std::string& strUrl, std::string& strResponse)
{
    strResponse.clear();
    CURLcode res;
    CURL* curl = curl_easy_init();
    if (NULL == curl)
    {
        return CURLE_FAILED_INIT;
    }
    if (m_bDebug)
    {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
    }
    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&strResponse);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    //curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
    //curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res;
}

std::string PEG_Connector::query(std::string db_name, std::string sparql, std::string format, std::string request_type)
{
    std::string res;
    if (request_type == "GET")
    {
        std::string strUrl = this->Url + "operation/query?dbname=" + db_name + "&sparql=" + UrlEncode(sparql);
        int ret = this->Get(strUrl, res);
    }
    else if (request_type == "POST")
    {
        std::string strPost = "{\"operation\": \"query\", \"username\": \"" + this->username + "\", \"password\": \"" + this->password + "\", \"db_name\": \"" + db_name + "\", \"format\": \"" + format + "\", \"sparql\": \"" + sparql + "\"}";
//        int ret = this->Post(this->Url, strPost, res);
    }
    return res;
}

std::string PEG_Connector::build(std::string db_name, std::string db_path, std::string part_path, std::string request_type)
{
    std::string res;
    if (request_type == "GET")
    {
        std::string strUrl = this->Url + "operation/build?dbname=" + db_name + "&ntpath=" + UrlEncode(db_path) + "&dpath=" + UrlEncode(part_path);
        int ret = this->Get(strUrl, res);
    }
    else if (request_type == "POST")
    {
        std::string strPost = "{\"operation\": \"build\", \"db_name\": \"" + db_name + "\", \"db_path\": \"" + db_path + "\", \"username\": \"" + this->username + "\", \"password\": \"" + this->password + "\"}";
//        int ret = this->Post(this->Url, strPost, res);
    }
    return res;
}

std::string PEG_Connector::deleteDB(std::string db_name, bool is_backup, std::string request_type)
{
    std::string res;
    if (request_type == "GET")
    {
        std::string strUrl;
        strUrl = this->Url + "operation/deleteDB?dbname=" + db_name;

        int ret = this->Get(strUrl, res);
    }
    else if (request_type == "POST")
    {
        std::string strPost;
        if (is_backup)
            strPost = "{\"operation\": \"drop\", \"db_name\": \"" + db_name + "\", \"username\": \"" + this->username + "\", \"password\": \"" + this->password + "\", \"is_backup\": \"true\"}";
        else
            strPost = "{\"operation\": \"drop\", \"db_name\": \"" + db_name + "\", \"username\": \"" + this->username + "\", \"password\": \"" + this->password + "\", \"is_backup\": \"false\"}";
//        int ret = this->Post(this->Url, strPost, res);
    }
    return res;
}

