#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <stack>

#include <pthread.h>

#include "util/FileTransfer.h"
#include "util/RemoteCmdInvoker.h"

#include "include/crow.h"

#include "gStoreAPI/client.h"
#include "queryDecompose/Query/QueryTree.h"
#include "queryDecompose/Query/GeneralEvaluation.h"
#include "joiner/joiner.h"
#include "util/util.h"
#include "handler/Handler.h"

using namespace std;

string conf;
crow::json::rvalue confJson;
vector<GstoreConnector*> servers;

//REFERENCE: C++ URL encoder and decoder
//http://blog.csdn.net/nanjunxiao/article/details/9974593
string UrlDecode(string& SRC)
{
    string ret;
    char ch;
    int ii;
    for(size_t i = 0; i < SRC.length(); ++i)
    {
        if(int(SRC[i]) == 37)
        {
            sscanf(SRC.substr(i+1,2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        }
        else if(SRC[i] == '+')
        {
            ret += ' ';
        }
        else
        {
            ret += SRC[i];
        }
    }
    return (ret);
}

int main() {
    crow::SimpleApp app;
    app.loglevel(crow::LogLevel::DEBUG);

    conf = readFile("conf/servers.json");
    confJson = crow::json::load(conf);

    for (int i = 0; i < confJson["sites"].size(); i++) {
        servers.push_back(new GstoreConnector(
                confJson["sites"][i]["ip"].s(),
                (int) confJson["sites"][i]["port"],
                confJson["sites"][i]["dbuser"].s(),
                confJson["sites"][i]["dbpasswd"].s()
        ));
    }

    CROW_ROUTE(app, "/operation/query")([](const crow::request& req) {
        string dbname = req.url_params.get("dbname");
        string sparql = req.url_params.get("sparql");
        sparql = UrlDecode(sparql);
        CROW_LOG_DEBUG << "sparql :" << sparql;
        CROW_LOG_DEBUG << "dbname :" << dbname;

        long long resNum;
        string finalRes = query(confJson, servers, dbname, sparql, resNum);

        crow::json::wvalue retv;
        retv["status"] = "success";
        retv["message"] = "success";
        retv["data"] = std::move(finalRes);
        return retv;
    });

    CROW_ROUTE(app, "/operation/build")([](const crow::request& req) {
        string dbname = req.url_params.get("dbname");
        string ntpath = req.url_params.get("ntpath");
        string dpath = req.url_params.get("dpath");
        ntpath = UrlDecode(ntpath);
        dpath = UrlDecode(dpath);
        int status = build(confJson, servers, dbname, ntpath, dpath);

        crow::json::wvalue retv;
        retv["status"] = "success";
        retv["message"] = "success";
        return retv;
    });

    CROW_ROUTE(app, "/operation/deleteDB")([](const crow::request& req){

        string dbname = req.url_params.get("dbname");

        int status = deleteDB(confJson, servers, dbname);

        crow::json::wvalue retv;
        retv["status"] = "success";
        retv["message"] = "success";
        return retv;
    });

    CROW_ROUTE(app, "/operation/test")([](const crow::request& req){
        string t = req.url_params.get("sparql");
        return t;
    });

    app.port(22314).multithreaded().run();

    CROW_LOG_INFO << "Deleting GstoreConnector...";
    for (int i = 0; i < confJson["sites"].size(); i++)
    {
        delete servers[i];
    }
    CROW_LOG_INFO << "Deleted GstoreConnector!";
    return 0;
}
