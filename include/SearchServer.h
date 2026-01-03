#ifndef __SEARCH_SERVER_H__
#define __SEARCH_SERVER_H__

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "LRUCache.h"
#include "WebPageMeta.h"

using std::string;
using std::shared_ptr;
using std::vector;
using std::map;
using std::unordered_map;
using std::pair;

class InvertIndex;
class SplitTool;
class WebPage;
class DictProducer;
class KeywordRecommender;

// 搜索服务器：基于 wfrest 的 HTTP 服务
class SearchServer {
public:
    SearchServer(const string& ip, int port,
                 shared_ptr<InvertIndex> index,
                 SplitTool* splitTool);

    // 设置网页库引用（用于获取标题、摘要等）- 传统模式
    void setPageLib(const map<int, shared_ptr<WebPage>>& pageLib);

    // 设置轻量级网页库（内存优化模式）
    void setPageLibLite(const unordered_map<int, WebPageMeta>& pageMeta,
                        const string& contentFilePath);

    // 设置词典和推荐器（可选）
    void setDictProducer(shared_ptr<DictProducer> dictProducer);
    void setRecommender(shared_ptr<KeywordRecommender> recommender);

    // 设置缓存大小
    void setCacheCapacity(size_t capacity);

    // 启动服务
    void start();

    // 停止服务
    void stop();

private:
    // 处理搜索请求
    string handleSearch(const string& query);

    // 处理关键词推荐请求
    string handleSuggest(const string& query);

    // 生成 JSON 响应
    string generateResponse(const string& query,
                           const vector<pair<int, double>>& results,
                           const vector<string>& queryWords);

    // 生成推荐词 JSON 响应
    string generateSuggestResponse(const string& query,
                                   const vector<string>& suggestions);

private:
    string _ip;
    int _port;
    shared_ptr<InvertIndex> _index;
    SplitTool* _splitTool;

    // 网页库：docId -> WebPage（传统模式）
    map<int, shared_ptr<WebPage>> _pageLib;

    // 轻量级网页库（内存优化模式）
    unordered_map<int, WebPageMeta> _pageMetaLib;
    shared_ptr<ContentStore> _contentStore;
    bool _useLiteMode = false;

    // 词典生成器和关键词推荐器
    shared_ptr<DictProducer> _dictProducer;
    shared_ptr<KeywordRecommender> _recommender;

    // LRU 缓存
    shared_ptr<SearchCache> _cache;

    // 优雅退出控制
    std::mutex _shutdownMutex;
    std::condition_variable _shutdownCv;
    std::atomic<bool> _running{false};
};

#endif // __SEARCH_SERVER_H__
