#pragma once

#include <map>
#include <string>
#include <vector>
using namespace std;

// trie���ڵ�
struct ACNode {
    ACNode(int p, char c) : parent(p), ch(c), fail(-1) {}

    char ch;
    map<char, int> sons;
    int fail;
    vector<int> output;
    int parent;
};

// AC�㷨��
class ACSearcher {
  public:
    ACSearcher();
    ~ACSearcher();

    void LoadPattern(const vector<string> &paterns);
    void BuildGotoTable();
    void BuildFailTable();
    vector<int> ACSearch(const string &text); // ����ƥ�䵽��ģʽ������

  private:
    int maxState;           // ���״̬��
    vector<ACNode> nodes;   // trie��
    vector<string> paterns; // ��Ҫƥ���ģʽ

    void AddState(int parent, char ch); // ��ʼ����״̬
};
