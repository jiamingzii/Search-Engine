#include "SearchServer.h"
#include "InvertIndex.h"
#include "SplitTool.h"
#include "WebPage.h"
#include "DictProducer.h"
#include "KeywordRecommender.h"
#include "Logger.h"
#include "wfrest/HttpServer.h"
#include "wfrest/json.hpp"
#include <sstream>
#include <iomanip>

// URL 解码函数
static string urlDecode(const string& encoded) {
    string decoded;
    decoded.reserve(encoded.size());

    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            int hex = 0;
            std::istringstream iss(encoded.substr(i + 1, 2));
            if (iss >> std::hex >> hex) {
                decoded += static_cast<char>(hex);
                i += 2;
            } else {
                decoded += encoded[i];
            }
        } else if (encoded[i] == '+') {
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }
    return decoded;
}

// 清理无效 UTF-8 字节，防止 JSON 序列化崩溃
static string cleanUtf8(const string& str) {
    string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ) {
        unsigned char c = str[i];
        size_t len = 1;
        if ((c & 0x80) == 0) len = 1;           // ASCII
        else if ((c & 0xE0) == 0xC0) len = 2;   // 2字节UTF-8
        else if ((c & 0xF0) == 0xE0) len = 3;   // 3字节UTF-8
        else if ((c & 0xF8) == 0xF0) len = 4;   // 4字节UTF-8
        else { i++; continue; }                  // 无效起始字节，跳过

        // 验证后续字节
        bool valid = true;
        if (i + len > str.size()) valid = false;
        for (size_t j = 1; valid && j < len; j++) {
            if ((str[i + j] & 0xC0) != 0x80) valid = false;
        }

        if (valid) {
            result.append(str, i, len);
            i += len;
        } else {
            i++;
        }
    }
    return result;
}

using wfrest::HttpServer;
using wfrest::HttpReq;
using wfrest::HttpResp;
using nlohmann::json;

SearchServer::SearchServer(const string& ip, int port,
                           shared_ptr<InvertIndex> index,
                           SplitTool* splitTool)
    : _ip(ip)
    , _port(port)
    , _index(index)
    , _splitTool(splitTool)
    , _cache(std::make_shared<SearchCache>(1000)) {
}

void SearchServer::setPageLib(const map<int, shared_ptr<WebPage>>& pageLib) {
    _pageLib = pageLib;
    _useLiteMode = false;
}

void SearchServer::setPageLibLite(const unordered_map<int, WebPageMeta>& pageMeta,
                                   const string& contentFilePath) {
    _pageMetaLib = pageMeta;
    _contentStore = std::make_shared<ContentStore>(contentFilePath);
    _useLiteMode = true;
}

void SearchServer::setDictProducer(shared_ptr<DictProducer> dictProducer) {
    _dictProducer = dictProducer;
}

void SearchServer::setRecommender(shared_ptr<KeywordRecommender> recommender) {
    _recommender = recommender;
}

void SearchServer::setCacheCapacity(size_t capacity) {
    _cache = std::make_shared<SearchCache>(capacity);
}

void SearchServer::start() {
    HttpServer server;

    // 搜索接口
    server.GET("/search", [this](const HttpReq* req, HttpResp* resp) {
        string query = req->query("q");
        if (query.empty()) {
            json error;
            error["error"] = "Missing query parameter 'q'";
            resp->set_header_pair("Content-Type", "application/json; charset=utf-8");
            resp->String(error.dump());
            return;
        }
        query = urlDecode(query);
        string result = handleSearch(query);
        resp->set_header_pair("Content-Type", "application/json; charset=utf-8");
        resp->set_header_pair("Access-Control-Allow-Origin", "*");
        resp->String(result);
    });

    // 关键词推荐接口
    server.GET("/suggest", [this](const HttpReq* req, HttpResp* resp) {
        string query = req->query("q");
        if (query.empty()) {
            json error;
            error["error"] = "Missing query parameter 'q'";
            resp->set_header_pair("Content-Type", "application/json; charset=utf-8");
            resp->String(error.dump());
            return;
        }
        query = urlDecode(query);
        string result = handleSuggest(query);
        resp->set_header_pair("Content-Type", "application/json; charset=utf-8");
        resp->set_header_pair("Access-Control-Allow-Origin", "*");
        resp->String(result);
    });

    // 健康检查
    server.GET("/health", [this](const HttpReq* req, HttpResp* resp) {
        json health;
        health["status"] = "ok";
        health["cache_size"] = _cache->size();
        health["cache_hit_rate"] = _cache->hitRate();
        resp->set_header_pair("Content-Type", "application/json; charset=utf-8");
        resp->String(health.dump());
    });

    // 静态文件（搜索页面）
    server.GET("/", [](const HttpReq* req, HttpResp* resp) {
        resp->File("static/index.html");
    });

    // 静态资源
    server.GET("/static/*", [](const HttpReq* req, HttpResp* resp) {
        string path = req->match_path();
        resp->File(path.substr(1));
    });

    LOG_INFO("Search server starting on " + _ip + ":" + std::to_string(_port));
    LOG_INFO("Cache capacity: 1000 entries");
    LOG_INFO("Press Ctrl+C to stop the server");

    if (server.start(_port) == 0) {
        server.list_routes();
        _running = true;

        std::unique_lock<std::mutex> lock(_shutdownMutex);
        _shutdownCv.wait(lock, [this] { return !_running.load(); });

        LOG_INFO("Stopping server...");
        server.stop();
        LOG_INFO("Server stopped gracefully");
    } else {
        LOG_ERROR("Failed to start server");
    }
}

void SearchServer::stop() {
    if (_running.exchange(false)) {
        _shutdownCv.notify_all();
    }
}

string SearchServer::handleSearch(const string& query) {
    string cachedResult;
    if (_cache->get(query, cachedResult)) {
        _cache->recordQuery(true);
        return cachedResult;
    }
    _cache->recordQuery(false);

    vector<string> queryWords = _splitTool->cut(query);
    vector<pair<int, double>> results = _index->search(queryWords);
    string response = generateResponse(query, results, queryWords);
    _cache->put(query, response);

    return response;
}

string SearchServer::handleSuggest(const string& query) {
    if (!_recommender) {
        json response;
        response["query"] = query;
        response["suggestions"] = json::array();
        return response.dump();
    }

    vector<string> suggestions = _recommender->recommend(query, 5, 2);
    return generateSuggestResponse(query, suggestions);
}

string SearchServer::generateResponse(const string& query,
                                      const vector<pair<int, double>>& results,
                                      const vector<string>& queryWords) {
    json response;
    response["query"] = query;
    response["total"] = results.size();

    json items = json::array();
    int count = 0;
    for (const auto& result : results) {
        if (count++ >= 20) break;

        json item;
        item["docId"] = result.first;
        item["score"] = result.second;

        if (_useLiteMode) {
            auto it = _pageMetaLib.find(result.first);
            if (it != _pageMetaLib.end()) {
                const auto& meta = it->second;
                item["title"] = cleanUtf8(meta.title);
                item["url"] = cleanUtf8(meta.url);
                item["summary"] = cleanUtf8(_contentStore->getSummary(
                    meta.contentOffset, meta.contentLength, queryWords));
            } else {
                item["title"] = "Document " + std::to_string(result.first);
                item["url"] = "";
                item["summary"] = "";
            }
        } else {
            auto it = _pageLib.find(result.first);
            if (it != _pageLib.end()) {
                auto& page = it->second;
                item["title"] = cleanUtf8(page->getTitle());
                item["url"] = cleanUtf8(page->getUrl());
                item["summary"] = cleanUtf8(page->getSummary(queryWords));
            } else {
                item["title"] = "Document " + std::to_string(result.first);
                item["url"] = "";
                item["summary"] = "";
            }
        }

        items.push_back(item);
    }

    response["results"] = items;
    return response.dump();
}

string SearchServer::generateSuggestResponse(const string& query,
                                             const vector<string>& suggestions) {
    json response;
    response["query"] = query;

    json suggestionList = json::array();
    for (const auto& s : suggestions) {
        suggestionList.push_back(s);
    }
    response["suggestions"] = suggestionList;

    return response.dump();
}
