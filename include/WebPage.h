#ifndef __WEB_PAGE_H__
#define __WEB_PAGE_H__

#include <string>
#include <vector>
#include <map>
#include <cstdint>

using std::string;
using std::vector;
using std::map;

class SplitTool;

// 网页类：表示一个文档
class WebPage {
public:
    WebPage(const string& doc, SplitTool* splitTool);

    int getDocId() const { return _docId; }
    string getTitle() const { return _title; }
    string getUrl() const { return _url; }
    string getContent() const { return _content; }

    // 获取词频统计
    map<string, int>& getWordsMap() { return _wordsMap; }

    // 计算 SimHash 用于去重（真正的 SimHash 实现）
    uint64_t getSimhash() const;

    // 处理文档：提取标题、内容、分词
    void processDoc(const string& doc);

    // 生成摘要（带查询词上下文）
    string getSummary(const vector<string>& queryWords) const;

    // SimHash 相关静态方法
    static int hammingDistance(uint64_t h1, uint64_t h2);
    static bool isSimilar(uint64_t h1, uint64_t h2, int threshold = 3);

private:
    static int _idGen;  // 文档 ID 生成器

    int _docId;
    string _title;
    string _url;
    string _content;

    SplitTool* _splitTool;
    map<string, int> _wordsMap;  // 词频统计
};

#endif // __WEB_PAGE_H__
