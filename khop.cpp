// string 转化为 string
//输入格式 <?x,?y>,<label>,3
//输出格式
//select ?x ?y where {
//    ?x label ?z.
//    ?z label ?t.
//    ?t label ?y.
//}

#include  <bits/stdc++.h>

using namespace std;

//使用字符串进行分割
void Stringsplit(string str, const char split, vector<string> &res) {
    if (str == "") return;
    //在字符串末尾也加入分隔符，方便截取最后一段
    string strs = str + split;
    size_t pos = strs.find(split);

    // 若找不到内容则字符串搜索函数返回 npos
    while (pos != std::string::npos) {
        string temp = strs.substr(0, pos);
        res.push_back(temp);
        //去掉已分割的字符串,在剩下的字符串中进行分割
        strs = strs.substr(pos + 1, strs.size());
        pos = strs.find(split);
    }
}

//查询重写
/*
 * 思想：根据num生成边 添加到查询语句中
 */
string  add_label(string rewrite, string x, string y, string label, int num) {
    string variable = "?variable";
    if (num == 1) {
        rewrite = rewrite +  x + " " + label + " "  + y + ". \n}";
        return rewrite;
    }else {
        //按照循环添加边
        for (int i = 0; i < num; i++) {
            if (i == 0) {
                string str = to_string(i);
                rewrite = rewrite + x + " " + label + " " + variable + str + ". \n";
            }else if (i == num - 1) {
                string str = to_string(i-1);
                rewrite = rewrite + variable + str + " " + label + " " + y + ". \n}";
            }else {
                string str0 = to_string(i-1);
                string str1 = to_string(i);
                rewrite = rewrite + variable + str0 + " " + label + " " + variable + str1 + ". \n";
            }
        }
        return rewrite;

    }
}

string rewrite1(vector<string> &res, int num) {
    //res内容是 ?x ?y label

    string x, y, label;
    x = res[0];
    y = res[1];
    label = res[2];
    x = x.replace(x.find('<'), 1, "");
    y = y.replace(y.find('>'), 1, "");

    //对 ?x ?y是否是常量进行讨论
    //x是常量 y是变量
    if (x.find('?') == string::npos && y.find('?') != string::npos) {
        string rewrite = "select " + y + "where { \n";
        return add_label(rewrite, x, y, label, num);
    }


    //x是变量 y是常量
    if (x.find('?') != string::npos && y.find('?') == string::npos) {
        string rewrite = "select " + x + " where { \n";
        return add_label(rewrite, x, y, label, num);
    }

    //x y 均是变量
    if (x.find('?') != string::npos && y.find('?') != string::npos) {
        string rewrite = "select " + x + " " + y + " where { \n";
        return add_label(rewrite, x, y, label, num);
    }

    //x y 均是常量
    if (x.find('?') == string::npos && y.find('?') == string::npos) {
        string rewrite = "select ";
        string variable = "?variable";
        if (num == 1) {

            return "没想好！";
        } else {
            //按照循环添加边
            string res = "";
            for (int i = 0; i < num; i++) {
                if (i == 0) {
                    string str = to_string(i);
                    rewrite = rewrite + variable + str + " ";
                    res = res + x + " " + label + " " + variable + str + ". \n";
                } else if (i == num - 1) {
                    string str = to_string(i - 1);
                    rewrite = rewrite + " where {\n";
                    res = res + variable + str + " " + label + " " + y + ". \n}";
                } else {
                    string str0 = to_string(i - 1);
                    string str1 = to_string(i);
                    rewrite = rewrite + variable + str1 + " ";
                    res = res + variable + str0 + " " + label + " " + variable + str1 + ". \n";
                }
            }
            return rewrite + res;
        }

    }
}


int main(int argv, char* args[]) {
//    string input, result;

    vector<string> strList;

    string str("<<ttttt>,<mmmmm>>,<dfs>,5");
	if(argv > 1) {
		str = string(args[1]);
	}

    Stringsplit(str, ',', strList);

//    for (auto s: strList) { cout << s << " "; }

    //判别几个边
    string num = strList.back();

    int number;
    stringstream ss;
    ss << num;
    ss >> number;
    cout << rewrite1(strList, number) << endl;


}
