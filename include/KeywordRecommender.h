#ifndef __KEYWORD_RECOMMENDER_H__
#define __KEYWORD_RECOMMENDER_H__

#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>

using std::string;
using std::vector;
using std::map;
using std::set;
using std::priority_queue;
using std::pair;

class DictProducer;

// 候选词结构
struct CandidateWord {
    string word;
    int distance;   // 编辑距离
    int frequency;  // 词频

    // 优先级：距离小的优先，距离相同时词频高的优先
    bool operator<(const CandidateWord& other) const {
        if (distance != other.distance) {
            return distance > other.distance;  // 距离小的优先
        }
        return frequency < other.frequency;  // 词频高的优先
    }
};

// 关键词推荐器：基于最小编辑距离
class KeywordRecommender {
public:
    KeywordRecommender(const DictProducer* dictProducer);

    // 获取推荐词列表
    vector<string> recommend(const string& query, int topK = 5, int maxDistance = 3);

    // 计算最小编辑距离
    static int editDistance(const string& s1, const string& s2);

private:
    // 将字符串拆分为 UTF-8 字符数组
    static vector<string> splitToChars(const string& s);

private:
    const DictProducer* _dictProducer;
};

#endif // __KEYWORD_RECOMMENDER_H__
