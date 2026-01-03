#ifndef __DICT_PRODUCER_H__
#define __DICT_PRODUCER_H__

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

using std::string;
using std::vector;
using std::map;
using std::set;
using std::shared_ptr;

class SplitTool;
class WebPage;

// 词典生成器：从语料中提取词典和索引
class DictProducer {
public:
    DictProducer(SplitTool* splitTool);

    // 从网页库构建词典
    void build(const vector<shared_ptr<WebPage>>& pages);
    // 从文本文件构建词典（一行一个文档）
    void buildFromFile(const string& filePath);

    // 存储词典到文件
    void storeDict(const string& filePath);
    // 加载词典
    void loadDict(const string& filePath);
    
    // 存储字符索引到文件
    void storeIndex(const string& filePath);
    // 加载字符索引
    void loadIndex(const string& filePath);

    // 获取词典（word -> frequency）
    const map<string, int>& getDict() const { return _dict; }

    // 根据字符获取候选词
    vector<string> getCandidates(const string& prefix) const;

private:
    // 构建字符索引
    void buildIndex();

    // 判断是否为中文字符
    bool isChinese(const string& ch) const;

    // 提取字符串的所有字符（包括中文）
    vector<string> extractChars(const string& word) const;

private:
    SplitTool* _splitTool;

    // 词典：词 -> 词频
    map<string, int> _dict;

    // 字符索引：字符 -> 包含该字符的词列表
    map<string, set<string>> _charIndex;
};

#endif // __DICT_PRODUCER_H__
