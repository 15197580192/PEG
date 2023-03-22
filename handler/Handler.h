#ifndef HANDLER_H
#define HANDLER_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <queue>
#include <unordered_map>

#include "crow/json.h"
#include "../util/FileTransfer.h"
#include "../util/RemoteCmdInvoker.h"
#include "../util/util.h"
#include "../gStoreAPI/client.h"
#include "../queryDecompose/Query/QueryTree.h"
#include "../queryDecompose/Query/GeneralEvaluation.h"
#include "../joiner/joiner.h"

int divide(std::string ntfile, std::string dividefile)
{
    std::ifstream ifs_dividefile;
    ifs_dividefile.open(dividefile);
    int num = 0;
    std::string entity;
    int partID;
    std::unordered_map<std::string, int> entity2id;

    while (ifs_dividefile >> entity && ifs_dividefile >> partID)
    {
        entity2id.insert(std::pair<std::string, int>(entity, partID));
        num = partID > num ? partID : num;
    }
    num++;
    
    std::ifstream ifs_ntfile;
    ifs_ntfile.open(ntfile);

    std::vector<std::ofstream> outfiles;
    outfiles.resize(num);
    std::string prefix = ntfile.substr(0, ntfile.find_last_of('.'));
    for (int i = 0; i < num; i++)
        outfiles[i].open(prefix + to_string(i) + ".nt");

    std::string line;
    std::vector<std::set<std::string>> s;
    s.resize(num);
    while (getline(ifs_ntfile, line))
    {
        std::string subject;
        std::string predict;
        std::string object;
        int lidx, ridx;
        
        lidx = line.find_first_of('<');
        ridx = line.find_first_of('>');
        subject = line.substr(lidx, ridx-lidx+1);

        lidx = line.find_first_of('<', ridx+1);
        ridx = line.find_first_of('>', ridx+1);
        predict = line.substr(lidx, ridx-lidx+1);

        lidx = line.find_first_of('<', ridx+1);
        ridx = line.find_first_of('>', ridx+1);

        int subPartID = entity2id[subject];
        outfiles[subPartID] << line << "\n";
        s[subPartID].insert(subject + "\t" + "<http://property/isInCurrPart>" + "\t" + "\"1\" .\n");
        
        if(lidx != std::string::npos)
        {
            object = line.substr(lidx, ridx-lidx+1);
            if(entity2id.find(object) != entity2id.end())
            {
                int objPartID = entity2id[object];
                if(subPartID == objPartID)
                {
                    s[subPartID].insert(object + "\t" + "<http://property/isInCurrPart>" + "\t" + "\"1\" .\n");
                }
                else
                {
                    s[subPartID].insert(object + "\t" + "<http://property/isInCurrPart>" + "\t" + "\"0\" .\n");

                    outfiles[objPartID] << line << "\n";
                    s[objPartID].insert(subject + "\t" + "<http://property/isInCurrPart>" + "\t" + "\"0\" .\n");
                    s[objPartID].insert(object + "\t" + "<http://property/isInCurrPart>" + "\t" + "\"1\" .\n");
                }
            }
        }
    }

    for (int i = 0; i < num; i++)
        for (auto iter = s[i].begin(); iter != s[i].end(); iter++)
            outfiles[i] << *iter << std::endl;

    for (int i = 0; i < num; i++)
        outfiles[i].close();
    ifs_ntfile.close();
    ifs_dividefile.close();

    return num;
}

int build(crow::json::rvalue &confJson, std::vector<GstoreConnector*> &servers, std::string &dbname, std::string &dbpath, std::string &divfilepath)
{
    // 数据图划分，得到一些nt文件的路径
    std::string txt_name = dbpath;                          // 源数据图路径
    std::string sign = ("2" == "1") ? " " : "\t";           // 分隔符
    std::string out_file = dbpath.substr(0, dbpath.find_last_of('.'));     // 输出文件的前缀

    int part = divide(dbpath, divfilepath);

    // 分片分发
    TransferThreadArgs tft[part];
    pthread_t thread[part];
    for (int i = 0; i < part; i++)
    {
        crow::json::rvalue site = confJson["sites"][i];
        
        tft[i].ft = new FileTransfer(site["ip"].s(), site["user"].s());
        tft[i].localFilePath = out_file + to_string(i) + ".nt";
        tft[i].remoteFilePath = "/tmp/"+dbname+to_string(i)+".nt";
        if (pthread_create(&thread[i], NULL, transferThread, &tft[i]) != 0)
        {
            throw runtime_error("creates transfer thread error!");
        }
    }
    for (int i = 0; i < part; i++)
    {
        pthread_join(thread[i], NULL);
    }

    // TODO 调用gStore http api 建库
    buildThreadArgs bta[part];
    pthread_t build_pthrd[part];
    for (int i = 0; i < part; i++)
    {
        crow::json::rvalue site = confJson["sites"][i];
        bta[i].gcp = servers[i];
        bta[i].dbname = dbname;
        bta[i].ntpath = "/tmp/"+dbname+to_string(i)+".nt";
        if (pthread_create(&build_pthrd[i], NULL, buildThread, &bta[i]) != 0)
        {
            throw runtime_error("creates build thread error!");
        }
    }
    for(int i = 0; i < part; i++)
        pthread_join(build_pthrd[i], NULL);
    
    for (int i = 0; i < part; i++)
    {
        delete tft[i].ft;
    }

    crow::json::rvalue rdb = crow::json::load(readFile("conf/dblist.json"));
    crow::json::wvalue wdb(rdb);
    std::vector<crow::json::wvalue> remainDB;
    for(int i = 0; i < rdb["dbs"].size(); i++)
        remainDB.push_back(std::move(wdb["dbs"][i]));
    crow::json::wvalue newDB;
    newDB["dbName"] = dbname;
    remainDB.push_back(std::move(newDB));
    wdb["dbs"] = std::move(remainDB);
    writeFile("conf/dblist.json", wdb.dump());

    return 0;
}

std::string query(crow::json::rvalue &confJson, std::vector<GstoreConnector*> &servers, std::string &dbname, std::string query, long long &resNum)
{

    int part = confJson["sites"].size();

    // load
    // buildThreadArgs bta[part];
    // pthread_t load_pthrds[part];
    // for (int i = 0; i < part; i++)
    // {
    //     crow::json::rvalue site = confJson["sites"][i];
    //     bta[i].gcp = servers[i];
    //     bta[i].dbname = dbname;
    //     if (pthread_create(&load_pthrds[i], NULL, loadThread, &bta[i]) != 0)
    //     {
    //         throw runtime_error("creates load thread error!");
    //     }
    // }
    // for (int i = 0; i < part; i++)
    //     pthread_join(load_pthrds[i], NULL);
    // std::cout << "all dbs invoked load." << std::endl;
    

    // query
    // 查询分解、组合
    long querytime = Util::get_cur_time();
    std::string _query_str = query;
    int PPQueryVertexCount = -1, vec_size = 0, star_tag = 0;
    QueryTree::QueryForm query_form = QueryTree::Ask_Query;
    GeneralEvaluation parser_evaluation;

    parser_evaluation.parseGetQuery(_query_str, PPQueryVertexCount, query_form, star_tag);
    printf("PPQueryVertexCount = %d\n", PPQueryVertexCount);
    std::vector<std::vector<std::string>> &total_queries = parser_evaluation.queriesDecomposed;
    long querydividetime = Util::get_cur_time();
    cout << "Divide Query cost " << querydividetime - querytime << " ms." << std::endl;
    
    // query on each site.
    std::vector<std::vector<std::string>> total_results;
    joiner joinobj;
    std::map<std::string, std::vector<std::string>*> query2res;
    int queryCnt = 0;
    for (int k = 0; k < total_queries.size(); k++)
        queryCnt += total_queries[k].size();
    std::cout << "The number of queries decomposed is " << queryCnt << std::endl;
    for (int k = 0; k < total_queries.size(); k++)
    {
        std::cout << "===================== Query partition method " << k << " =====================" << std::endl;
        long querybegin = Util::get_cur_time();
        std::vector<std::string> &queries = total_queries[k];
        // 执行查询
        std::queue<std::vector<std::string>*> results;
        // 并行查询
        pthread_t thread[queries.size()][servers.size()];
        QNV qnv[queries.size()][servers.size()];
        long queryTime[servers.size()];
        // vector<long> queryTime[servers.size()];
        memset(queryTime, 0, sizeof queryTime);
        std::set<int> skipQueryNum;
        for (int i = 0; i < queries.size(); i++)
        {
            if(query2res.find(queries[i]) == query2res.end())
                for (int j = 0; j < servers.size(); j++)
                {
                    qnv[i][j].queryId = i;
                    qnv[i][j].serverId = j;
                    qnv[i][j].server = servers[j];
                    qnv[i][j].dbname = dbname;
                    qnv[i][j].format = "text";
                    qnv[i][j].query = queries[i];
                    std::cout << "query " << i << " on server " << j << " ";
                    if (pthread_create(&thread[i][j], NULL, queryThread, &qnv[i][j]) != 0)
                    {
                        throw runtime_error("creates thread error!");
                    }
                    else
                        std::cout << "creates thread success!" << std::endl;
                }
            else
            {
                skipQueryNum.insert(i);
                std::cout << "query " << i <<" has already executed." << std::endl;
            }
        }
        for (int i = 0; i < queries.size(); i++)
        {
            if(skipQueryNum.find(i) == skipQueryNum.end())
            for (int j = 0; j < servers.size(); j++)
                pthread_join(thread[i][j], NULL);
        }
        // 合并并行查询后的结果
        std::cout << "Now merge the results..." << std::endl;
        for (int i = 0; i < queries.size(); i++) {
            if (query2res.find(queries[i]) == query2res.end()) {
                bool firstRes = true;
                std::set<std::string> s; // 去掉重复的结果
                std::string head;
                for (int j = 0; j < servers.size(); j++) {
                    std::string &temp_res = qnv[i][j].res;
                    // std::cout << temp_res ;
                    if (temp_res.empty()){
                        std::cout << "query " << i << " server " << j << " No response " << std::endl;
                        continue;
                    }
                    else if (temp_res.find("[empty result]") != std::string::npos) // 查出的结果是空集
                    {
                        std::cout << "query " << i << " server " << j << " Empty" << std::endl;
                        continue;
                    } else {
                        std::cout << "query " << i << " server " << j << " Get Answer ";
                    }

                    std::vector<std::string> lines;
                    joiner::split(temp_res, lines, "\n");
                    std::cout << lines.size() - 1 << std::endl;
                    std::vector<std::string>::iterator iter = lines.begin();
                    head = *iter;
                    // queryTime[j] += stol(*iter);
                    // queryTime[j].push_back(stol(*iter));
                    ++iter;
                    if (firstRes) // 第一次添加结果
                    {
                        firstRes = false;
                    }
                    for (; iter != lines.end(); iter++)
                        s.insert(*iter);
                }
                std::vector<std::string> *resOfQuery = new std::vector<std::string>; // 是否要new
                if (!firstRes) // 有结果
                {
                    resOfQuery->push_back(head);
                    resOfQuery->insert(resOfQuery->end(), s.begin(), s.end());
                }
                results.push(resOfQuery);
                query2res[queries[i]] = resOfQuery;
            } else {
                results.push(query2res[queries[i]]);
                std::cout << "Get result of executed query " << i << "." << std::endl;
            }

        }
        std::cout << "Before Join results size is " << results.size() << std::endl;

        // join
        long joinbegin = Util::get_cur_time();
        std::vector<std::string> finalRes = joinobj.join(results);
        std::cout<< "joinobj.join " << finalRes.size() << std::endl;
        
        long queryend = Util::get_cur_time();
        std::cout << "Join cost " << queryend - joinbegin << " ms." << std::endl;
        std::cout << "Execute queries and join, cost " << queryend - querybegin << " ms." << std::endl;
        std::cout << "===================== Query partition method " << k << " =====================" << std::endl << std::endl;

        total_results.push_back(finalRes);
    }
    
    // unload
    // pthread_t unload_pthrds[part];
    // for (int i = 0; i < part; i++)
    //     if (pthread_create(&unload_pthrds[i], NULL, unloadThread, &bta[i]) != 0)
    //         throw runtime_error("creates unload thread error!");
    // for (int i = 0; i < part; i++)
    //     pthread_join(unload_pthrds[i], NULL);
    
    // union
    long unionbegin = Util::get_cur_time();
    std::vector<std::string> unionRes = total_results[0];
    for(int i = 1; i < total_results.size(); i++)
    {
        unionRes = joinobj.Union(unionRes, total_results[i]);
    }
    std::cout << "Begin to Union." << std::endl;
    long unionend = Util::get_cur_time();
    std::cout << "Union res of all method, cost " << unionend - unionbegin << " ms." << std::endl;
    for (std::map<std::string, std::vector<std::string>*>::iterator it = query2res.begin(); it != query2res.end(); it++) // 要改
        delete it->second;

    resNum = unionRes.size() > 1 ? unionRes.size() - 1 : 0;
    std::string finalResult;
    for (int i = 0; i < unionRes.size(); i++)
    {
        finalResult += (unionRes.at(i) + "\n");
    }
    
    return finalResult;
}

int deleteDB(crow::json::rvalue &confJson, std::vector<GstoreConnector*> &servers, const std::string &dbname)
{
    int part = confJson["sites"].size();
    buildThreadArgs bta[part];
    pthread_t delete_pthrds[part];
    for (int i = 0; i < part; i++)
    {
        crow::json::rvalue site = confJson["sites"][i];
        bta[i].gcp = servers[i];
        bta[i].dbname = dbname;
        if (pthread_create(&delete_pthrds[i], NULL, deleteThread, &bta[i]) != 0)
            throw runtime_error("creates delete thread error!");
    }
    for (int i = 0; i < part; i++)
        pthread_join(delete_pthrds[i], NULL);
    
    crow::json::rvalue rdb = crow::json::load(readFile("conf/dblist.json"));
    crow::json::wvalue wdb(rdb);
    std::vector<crow::json::wvalue> remainDB;
    for(int i = 0; i < rdb["dbs"].size(); i++)
    {
        if(rdb["dbs"][i]["dbName"] == dbname)
            continue;
        remainDB.push_back(std::move(wdb["dbs"][i]));
    }
    wdb["dbs"] = std::move(remainDB);
    writeFile("conf/dblist.json", wdb.dump());

    return 0;
}

#endif