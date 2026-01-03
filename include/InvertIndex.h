#ifndef __INVERT_INDEX_H__
#define __INVERT_INDEX_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map> // 优化: 引入哈希表
#include <memory>

using std::string;
using std::vector;
using std::map;
using std::unordered_map;
using std::pair;
using std::shared_ptr;

class WebPage;

// 倒排索引项：文档ID + 权重
// 优化: 保持 POD 结构，内存布局紧凑
struct InvertIndexItem {
     // BM25 权重
    double weight; //当前词（Term）在当前文档（Doc）中的重要程度评分
    int docId;
    int termFreq;   // 词频该词出现在该文档的次数（词频）
};

class InvertIndex {
public:
    InvertIndex();
    //为计算BM25做准备
    void build(vector<shared_ptr<WebPage>>& pages);

    //  增根据查询词搜索权重最大的前20个
    vector<pair<int, double>> search(const vector<string>& queryWords, int topK = 20);
    //存储 和 加载 网页
    void store(const string& filePath);
    void load(const string& filePath);

    int getTotalDocs() const { return _totalDocs; }

private:
    static constexpr double K1 = 1.2;
    static constexpr double B = 0.75;

    double calculateIDF(int docFreq, int totalDocs);
    double calculateBM25(int termFreq, int docLen, int docFreq);

private:
    // 优化: 使用 unordered_map 替代 map，查询速度提升至 O(1)
    unordered_map<string, vector<InvertIndexItem>> _invertIndex;

    map<int, int> _docLens; //每个文档（存储ID）对应的长度
    int _totalDocs;//总的文件数
    double _avgDocLen;//平均文件长度
};

#endif // __INVERT_INDEX_H__