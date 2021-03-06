#include <iostream>
#include <set>
#include <vector>
#include <stack>
#include <deque>
#include <queue>
#include <map>
#include <algorithm>
#include <iomanip>
using namespace std;




//定义LR(0)项目的结构体
struct LRProject {
    char left;
    string right;
    int dot;

    bool operator==(const LRProject& lrProject) const {
        return left == lrProject.left && right == lrProject.right
               && dot == lrProject.dot;
    }
    LRProject(char ch, string str, int d): left{ch}, right{std::move(str)}, dot{d}{}
    void show() const{
        cout << left <<"->" << right<<' ' << dot<<endl;
    }
};

//定义LR(0)项目集结构体
struct ItemSet{
    vector<LRProject> proSet; //保存该项目集中的LR(0)项目
    set<char> nextCh; //保存所有可以转移的符号
    int id{}; //对应DFA中的序号

    bool operator==(const ItemSet & itemSet) const {
        if (proSet.size() != itemSet.proSet.size())
            return false;
        for (auto &pro: itemSet.proSet)
            if (find(proSet.begin(), proSet.end(), pro) == proSet.end())
                return false;
        return true;
    }

    void show() const {
        cout << "I"<<id<<":" <<endl;
        cout << "LRProjects:\n";
        for (auto &pro: proSet)
            pro.show();
        cout << "transfer character:\n";
        for (auto &ch: nextCh)
            cout << ch << ' ';
        cout << endl;
    }
};

ItemSet I; //开始状态
vector<ItemSet> C; //项目集规范族


//variable
deque<int> state; //状态栈
deque<char> symbol; //符号栈
deque<char> inputBuffer; //输入缓冲区 使用双端队列
map<pair<int, char>, pair<char, int>> action; //动作表 键为当前状态和输入符号，对应的值为
//操作  S: 转移状态 / R: 归约产生式
map<pair<int, char>, int> Goto; //状态转移表 根据状态栈顶和符号栈顶确定转移的状态

set<char> terminal, nonterminal; //终结符 非终结符集合
map<int, pair<char, string>> NumToProduction; //序号-产生式 用于移进归约时调用
map<pair<char, string>, int> ProductionToNum; //产生式-序号

map<char, vector<string>> production; //产生式左部-产生式右部 用于构建closure

map<char, vector<char>> FIRST, FOLLOW; //FIRST FOLLOW集合


//process
void createFIRST(); //构造FIRST集合
void createFOLLOW(); //构造FOLLOW集合
void createClosure(ItemSet & item); //构造闭包
ItemSet go(ItemSet & item, char x); //go函数
void createLRCanonicalCollection(ItemSet & item); //构造项目集规范族
void showSLRTable(); //展示SLR(1)分析表
void LRAnalysisProgram(); //LR分析程序
void init(); //初始化函数  初始化各变量


/* 构造FIRST FOLLOW 集合
 */

void createFIRST(){
    bool flag;
    do{
        flag = false;
//        对于每一个非终结符
        for (auto& item: production) {
//            每一个产生式
            for (auto &p: item.second) {
//                如果产生式第一个是终结符 且目前未被加入
                if (terminal.count(p[0]) &&
                    find(FIRST[item.first].begin(), FIRST[item.first].end(), p[0]) == FIRST[item.first].end()) {
                    FIRST[item.first].push_back(p[0]);
                    flag = true;
                }
//                如果是非终结符
                else {
                    for (auto &c: FIRST[p[0]]) {
                        if (find(FIRST[item.first].begin(),
                                 FIRST[item.first].end(), c) == FIRST[item.first].end()) {
                            FIRST[item.first].push_back(c);
                            flag = true;
                        }
                    }
                }
            }
        }
    }while (flag);
}

void createFOLLOW() {
    bool flag;

    FOLLOW['S'].push_back('$');
    do {
        flag = false;
//        每一个非终结符
        for (auto &nt: nonterminal) {
//    每一个产生式
            for (auto &p: production[nt]) {
//                产生式 A->αBβ 把β的first集合加入B的follow集合
                for (int i = 0; i < p.size() - 1; i++) {
//                    如果β是非终结符
                    if (nonterminal.count(p[i + 1])) {
                        for (auto &f: FIRST[p[i + 1]]) {
                            if (find(FOLLOW[p[i]].begin(), FOLLOW[p[i]].end(), f) == FOLLOW[p[i]].end()) {
                                FOLLOW[p[i]].push_back(f);
                                flag = true;
                            }
                        }
                    } else {
                        if (find(FOLLOW[p[i]].begin(), FOLLOW[p[i]].end(), p[i + 1]) == FOLLOW[p[i]].end()) {
                            FOLLOW[p[i]].push_back(p[i + 1]);
                            flag = true;
                        }
                    }
                }

//                产生式 A->αB 把A的follow集合加入B的follow集合 B为非终结符
                if (nonterminal.count(p.back())) {
                    for (auto &f: FOLLOW[nt])
                        if (find(FOLLOW[p.back()].begin(), FOLLOW[p.back()].end(), f) == FOLLOW[p.back()].end()) {
                            FOLLOW[p.back()].push_back(f);
                            flag = true;
                        }
                }
            }
        }
    } while (flag);
}


// 构造FIRST
// 构造FOLLOW集
/*closure(I) 的构造过程
  *parameter: I: vector<LRProject
  *output: I
 */
void createClosure(ItemSet& item) {
    bool flag;
    do {
        flag = false;
        for (auto pro: item.proSet) {
            for (const auto &string: production[pro.right[pro.dot]]) {
                LRProject lrProject = LRProject(pro.right[pro.dot], string, 0);
//                B->.η不属于当前项目  加入
                if (find(item.proSet.begin(), item.proSet.end(), lrProject) == item.proSet.end()) {
                    item.proSet.push_back(lrProject);
                    flag = true;
                }
            }
        }
    } while (flag);
//    构造nextCh集合
    for (auto &pro: item.proSet)
        if (pro.dot < pro.right.length())
            item.nextCh.insert(pro.right[pro.dot]);
}

/*go转移函数
 * parameter: I: vector<LRProduction> x: char
 * output: J: vector<LRProduction>
 */

ItemSet go(ItemSet & item, char x) {
     ItemSet J;
    for (auto& pro: item.proSet){
        if (pro.right[pro.dot] == x)
            J.proSet.emplace_back(pro.left, pro.right, pro.dot+1);
    }
    createClosure(J);

    return J;
}

/*构造文法项目集规范族
 * parameter: I: vector<LRProject> S`->.S
 * output: 项目集规范族C: vector<vector<LRProject>>
 *
 */

void createLRCanonicalCollection(ItemSet & item) {
    int id = 0;
    createClosure(item);
    item.id=id++;
    C.push_back(item);
    queue<ItemSet> que;
    que.push(item);


    while (!que.empty()) {
        auto nowitem = que.front();
        que.pop();

//            可以转移的字符
        for (auto &ch: nowitem.nextCh) {
            ItemSet J = go(nowitem, ch);
//                J非空
            if (!J.proSet.empty()) {
//                J未加入项目集规范族 则加入
                if (find(C.begin(), C.end(), J) == C.end()) {
                    J.id = id++;
                    que.push(J);
                    C.push_back(J);
                }

                // 找出J
                J = *find(C.begin(), C.end(), J);
//                    填写action/Goto表
//                    该字符是终结符 填入action表
                if (terminal.count(ch))
                    action[{nowitem.id, ch}] = {'S', J.id};

//                非终结符  填入goto表
                else
                    Goto[{nowitem.id, ch}] = J.id;
            }
        }
    }

    for (auto & itemSet: C){
        for (auto &pro: itemSet.proSet) {
//        如果是移进项目或者待约项目 即 dot < length 将移进字符保存

            if (pro.dot == pro.right.length()) {
//            接受项目
                if (pro.left == 'S') {
                    for (auto &ch: FOLLOW[pro.left])
                        action[{itemSet.id, ch}] = {'A', 0};
                }
//            归约
                else {
                    for (auto &ch: FOLLOW[pro.left])
                        action[{itemSet.id, ch}] = {'R', ProductionToNum[{pro.left, pro.right}]};
                }
            }
        }
    }
}


void test() {
//    检查FIRST的构造
    createFIRST();
    for (auto &i: nonterminal) {
        for (auto &j: FIRST[i])
            cout << j << ' ';
        cout << endl;
    }

//    检查FOLLOW的构造
    for (auto &i: nonterminal) {
        cout << i << ":";
        for (auto &j: FOLLOW[i])
            cout << j << ' ';
        cout << endl;
    }
    createFOLLOW();
    for (auto &i: nonterminal) {
        cout << i << ":";
        for (auto &j: FOLLOW[i])
            cout << j << ' ';
        cout << endl;
    }


//    ItemSet I;
//    I.proSet.emplace_back('S', "E", 0);
//
////    测试createClosure go函数
//
//    createClosure(I);
//    cout << "I:\n";
//    I.show();
//
//    ItemSet J = go(I, 'E');
//    cout << "J:\n";
//    J.show();
//
//    ItemSet K = go(I, '(');
//    cout << "K:\n";
//    K.show();
//
//
////    测试为产生式标号
//    for (auto &item: ProductionToNum)
//        cout << item.second << ": " << item.first.first
//             << "->" << item.first.second << endl;
//
//
//
//
////    测试构建项目集规范族函数
//    createLRCanonicalCollection(I);
//    for (auto& itemset: C)
//        itemset.show();
////    检查action表
////    for(auto & item: action)
////        cout << item.first.first<< ' ' << item.first.second<< ' '<<item.second.first<<' '<<item.second.second<<endl;
////    检查goto表
//for(auto& item: Goto)
//    cout << item.first.first<<' ' << item.first.second<<' ' << item.second<<endl;
}



/*
 * 构造SLR(1)分析表
 *
 */

void showSLRTable() {
    cout << "\t\t\t action\t\t\t\t\t\t\tgoto\n";
    cout << "-------------------------------------------------------------------------------------------\n";
    cout << "state\t";
    for (auto &i: terminal)
        cout << i << '\t';
    for (auto &i: nonterminal)
        if (i != 'S')
            cout << i << '\t';
    cout << endl;

    for (int i = 0; i < C.size(); i++) {
        cout << "-------------------------------------------------------------------------------------------\n";
        cout << i << '\t';
        for (auto &ch: terminal) {
            if (action.count({i, ch}))
                cout << action[{i, ch}].first << action[{i, ch}].second << "\t";
            else
                cout << "\t";
        }
        for (auto &ch: nonterminal) {
            if (ch != 'S') {
                if (Goto.count({i, ch}))
                    cout << Goto[{i, ch}] << '\t';
                else
                    cout << '\t';
            }
        }
        cout << endl;
    }
    cout << "-------------------------------------------------------------------------------------------\n";
}

/*
 * LR分析程序
 */

void LRAnalysisProgram() {
    bool flag = true;
    state.push_back(0);
    symbol.push_back('-');
    int len;
    int blanks = inputBuffer.size() * 3/2;

    cout << "Analysis Process:\n";
    cout
            << "----------------------------------------------------------------------------------------------------------\n";
    cout << " stack\t\t\t\t\t\t\t\tinput\t\t\tAnalysis action" << endl;
    cout
            << "----------------------------------------------------------------------------------------------------------\n";

    do {
//        展示当前分析情况
//栈
        cout << " State: ";
        for (auto &item: state)
            cout << setw(3) << item;
        cout << endl;
        cout << "Symbol: ";
        for (auto &item: symbol)
            cout << setw(3) << item;
        len = 3 * (blanks - symbol.size() - inputBuffer.size());
        while (len--)
            cout << ' ';

//        cout << "\t\t";
//        len = 3 * (blanks - inputBuffer.size());
//        while (len--)
//            cout << ' ';
        for (auto &item: inputBuffer)
            cout << setw(3) << item;

        cout << "\t\t\t";


//        s是state栈顶状态 a是ip所指向的符号
        int s = state.back();
        char a = inputBuffer.front();
        switch (action[{s, a}].first) {
            case 'S': {
                cout << "Shift " << action[{s, a}].second;
//             把a和 shift s`压入symbol栈和state栈
                symbol.push_back(a);
                state.push_back(action[{s, a}].second);
//            指针前移
                inputBuffer.pop_front();
            }
                break;
            case 'R': {
//                取出归约式
                int number = action[{s, a}].second;
                auto p = NumToProduction[number];
//                弹出|β|个字符
                int sz = p.second.size();
                while (sz--) {
                    symbol.pop_back();
                    state.pop_back();
                }
                symbol.push_back(p.first);
                state.push_back(Goto[{state.back(), symbol.back()}]);
                cout << "reduce by " << p.first << "->" << p.second;
            }
                break;
            case 'A': {
                cout << "ACC";
                flag = false;
            }
                break;
//            出错
            default: {
                flag = false;
                cout << "error";
            }
                break;
        }
        cout << endl;
        cout
                << "----------------------------------------------------------------------------------------------------------\n";
    } while (flag);
}


void init() {
//    初始化终结符集合
    terminal.insert('+');
    terminal.insert('-');
    terminal.insert('*');
    terminal.insert('/');
    terminal.insert('(');
    terminal.insert(')');
    terminal.insert('n');
    terminal.insert('$');

//    初始化非终结符集合
    nonterminal.insert('E');
    nonterminal.insert('S');
    nonterminal.insert('T');
    nonterminal.insert('F');


//    产生式左右对应map初始化
    production['S'] = {"E"};
    production['E'] = {"E+T", "E-T", "T"};
    production['T'] = {"T*F", "T/F", "F"};
    production['F'] = {"(E)", "n"};

//    产生式-序号  序号-产生式初始化

    int id = 1;
    for (auto &left: nonterminal)
        for (auto &p: production[left]) {
            ProductionToNum[{left, p}] = id++;
            NumToProduction[id - 1] = {left, p};
        }

    for (auto& item: NumToProduction)
        cout << item.first<<' ' << item.second.first<<"->"<<item.second.second<<endl;
    createFIRST();
    createFOLLOW();
    I.proSet.emplace_back('S', "E", 0);
    createLRCanonicalCollection(I);
}

int main() {
    init();
    test();
    showSLRTable();

//    输入串存入缓冲区
   cout << "Enter the string:";
   string str;
   cin >> str;
   for (auto & ch: str)
       inputBuffer.push_back(ch);
   inputBuffer.push_back('$');
//   分析过程
   LRAnalysisProgram();

    return 0;
}
