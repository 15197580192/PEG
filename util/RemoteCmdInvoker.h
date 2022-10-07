#ifndef REMOTECMDINVOKER
#define REMOTECMDINVOKER

#include <cstdlib>
#include <string>
#include <iostream>

#include "RemoteInfo.h"
#include "../gStoreAPI/client.h"

#include "../queryDecompose/Util/Util.h"

using namespace std;

class RemoteCmdInvoker : public RemoteInfo
{
private:

public:
    RemoteCmdInvoker(std::string _ip, std::string _user);
    ~RemoteCmdInvoker();

    int execute(std::string _cmd);
};

RemoteCmdInvoker::RemoteCmdInvoker(std::string _ip, std::string _user)
 : RemoteInfo(_ip, _user)
{
}

RemoteCmdInvoker::~RemoteCmdInvoker()
{
}

int RemoteCmdInvoker::execute(std::string _cmd)
{
    std::string cmd = "ssh " + user+"@"+ip+" \"" + _cmd + "\"";
    std::cout << cmd << std::endl;
    
    int ret = system(cmd.c_str());
    return ret;
}

struct buildThreadArgs
{
    GstoreConnector *gcp;
    std::string dbname;
    std::string ntpath;
};


void *buildThread(void *args)
{
    buildThreadArgs *p = (buildThreadArgs*) args;
    std::cout << "ip: " 
              << p->gcp->serverIP << " 255.255.255.0"
              << " port: " 
              << p->gcp->serverPort 
              << " dbuser: " 
              << p->gcp->username 
              << " dbpasswd: " 
              << p->gcp->password 
              << " dbname: "
              << p->dbname
              << " ntpath: "
              << p->ntpath
              << std::endl;
    std::cout << p->gcp->build(p->dbname, p->ntpath) << std::endl
              << p->gcp->load(p->dbname) << std::endl;
    return nullptr;
}

void *loadThread(void *args)
{
    buildThreadArgs *p = (buildThreadArgs*) args;
    std::cout << p->gcp->load(p->dbname) << std::endl;
    return nullptr;
}

void *unloadThread(void *args)
{
    buildThreadArgs *p = (buildThreadArgs*) args;
    std::cout << p->gcp->unload(p->dbname) << std::endl;
    return nullptr;
}

void *deleteThread(void *args)
{
    buildThreadArgs *p = (buildThreadArgs*) args;
    p->gcp->unload(p->dbname);
    std::cout << p->gcp->drop(p->dbname, false) << std::endl;
    return nullptr;
}

typedef struct QueryNeedVars
{
    int queryId, serverId;
    GstoreConnector *server;
    string dbname;
    string format;
    string query;
    string res;
    long timeCost;
} QNV;

void *queryThread(void *args)
{
    long begin = Util::get_cur_time(), timeCost;
    QNV *vars = (QNV *)args;
    GstoreConnector &server = *vars->server;
    vars->res = server.query(vars->dbname, vars->format, vars->query);
    timeCost = Util::get_cur_time() - begin;
    cout << "query " << vars->queryId << " has finished on server " << vars->serverId << ". Take " << timeCost << "ms." << endl;
    cout << vars->query << endl;
    vars -> timeCost = timeCost;
    // cout << vars -> timeCost << endl;
    return NULL;
}

#endif