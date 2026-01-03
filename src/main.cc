#include "Configuration.h"
#include "SplitTool.h"
#include "PageLib.h"
#include "PageLibPreprocessor.h"
#include "InvertIndex.h"
#include "SearchServer.h"
#include "WebPage.h"
#include "DictProducer.h"
#include "KeywordRecommender.h"
#include "Logger.h"
#include <memory>
#include <csignal>
#include <atomic>

using std::make_shared;

// 全局变量用于信号处理
static std::atomic<bool> g_running{true};
static SearchServer* g_server = nullptr;

// 信号处理函数
void signalHandler(int signum) {
    LOG_INFO("Received signal " + std::to_string(signum) + ", shutting down gracefully...");
    g_running = false;
    if (g_server) {
        g_server->stop();
    }
}

void printUsage(const char* progName) {
    LOG_INFO("Usage:");
    LOG_INFO("  " + string(progName) + " build       - Build index from data");
    LOG_INFO("  " + string(progName) + " server      - Start search server (traditional mode)");
    LOG_INFO("  " + string(progName) + " server-lite - Start search server (memory-optimized mode)");
}

int main(int argc, char* argv[]) {
    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 初始化日志系统
    Logger::getInstance()->init("conf/log4cpp.properties");

    try {
        if (argc < 2) {
            printUsage(argv[0]);
            return 1;
        }

        string mode = argv[1];

        // 加载配置
        Configuration* config = Configuration::getInstance();
        config->load("conf/search.conf");

        // 初始化分词工具
        auto splitTool = make_shared<JiebaSplitTool>(
            config->get("dict_path"),
            config->get("model_path"),
            config->get("user_dict_path"),
            config->get("idf_path"),
            config->get("stop_word_path")
        );

        if (mode == "build") {
            // 构建索引模式
            LOG_INFO("=== Building Index ===");

            // 1. 加载网页库
            PageLib pageLib(config->get("data_path"), splitTool.get());
            pageLib.load();

            // 2. 预处理（去重）
            PageLibPreprocessor preprocessor(pageLib.getPages(), splitTool.get());
            preprocessor.deduplicate();

            auto& processedPages = preprocessor.getProcessedPages();
            LOG_INFO("After deduplication: " + std::to_string(processedPages.size()) + " pages");

            // 3. 构建倒排索引
            auto index = make_shared<InvertIndex>();
            index->build(processedPages);

            // 4. 存储索引
            index->store(config->get("index_path"));

            // 5. 构建词典（用于关键词推荐）
            LOG_INFO("=== Building Dictionary ===");
            auto dictProducer = make_shared<DictProducer>(splitTool.get());
            dictProducer->build(processedPages);
            dictProducer->storeDict(config->get("dict_path_output"));
            dictProducer->storeIndex(config->get("dict_index_path"));

            // 6. 存储网页库（用于搜索时获取标题、摘要）
            LOG_INFO("=== Storing Page Library ===");
            pageLib.store(config->get("pagelib_path"));

            // 7. 存储分离格式（用于轻量级模式）
            LOG_INFO("=== Storing Separated Format (for lite mode) ===");
            string metaPath = config->get("pagelib_path") + ".meta";
            string contentPath = config->get("pagelib_path") + ".content";
            pageLib.storeSeparated(metaPath, contentPath);

            LOG_INFO("=== Index Build Complete ===");

        } else if (mode == "server" || mode == "server-lite") {
            bool useLiteMode = (mode == "server-lite");
            LOG_INFO("=== Starting Search Server ===");
            if (useLiteMode) {
                LOG_INFO("Mode: Memory-optimized (lite)");
            } else {
                LOG_INFO("Mode: Traditional");
            }

            // 1. 加载索引
            auto index = make_shared<InvertIndex>();
            index->load(config->get("index_path"));

            // 2. 加载网页库
            LOG_INFO("Loading page library...");

            map<int, shared_ptr<WebPage>> pageMap;
            unordered_map<int, WebPageMeta> pageMeta;
            string contentFilePath;

            if (useLiteMode) {
                string metaPath = config->get("pagelib_path") + ".meta";
                contentFilePath = config->get("pagelib_path") + ".content";
                pageMeta = PageLib::loadMeta(metaPath);
                LOG_INFO("Lite mode: " + std::to_string(pageMeta.size()) + " page metadata loaded");
            } else {
                PageLib pageLib(config->get("data_path"), splitTool.get());
                pageLib.load();

                for (auto& page : pageLib.getPages()) {
                    pageMap[page->getDocId()] = page;
                }
                LOG_INFO("Loaded " + std::to_string(pageMap.size()) + " pages (full content in memory)");
            }

            // 3. 加载词典
            shared_ptr<DictProducer> dictProducer;
            shared_ptr<KeywordRecommender> recommender;

            string dictPath = config->get("dict_path_output");
            if (!dictPath.empty()) {
                dictProducer = make_shared<DictProducer>(splitTool.get());
                dictProducer->loadDict(dictPath);

                string indexPath = config->get("dict_index_path");
                if (!indexPath.empty()) {
                    dictProducer->loadIndex(indexPath);
                }

                recommender = make_shared<KeywordRecommender>(dictProducer.get());
                LOG_INFO("Keyword recommender enabled");
            }

            // 4. 启动服务
            string ip = config->get("server_ip");
            int port = std::stoi(config->get("server_port"));

            SearchServer server(ip, port, index, splitTool.get());
            g_server = &server;

            if (useLiteMode) {
                server.setPageLibLite(pageMeta, contentFilePath);
            } else {
                server.setPageLib(pageMap);
            }

            if (dictProducer) {
                server.setDictProducer(dictProducer);
            }
            if (recommender) {
                server.setRecommender(recommender);
            }

            string cacheSizeStr = config->get("cache_size");
            if (!cacheSizeStr.empty()) {
                server.setCacheCapacity(std::stoul(cacheSizeStr));
            }

            server.start();
            g_server = nullptr;

        } else {
            printUsage(argv[0]);
            return 1;
        }

    } catch (const std::exception& e) {
        LOG_FATAL(string("Fatal error: ") + e.what());
        return 1;
    } catch (...) {
        LOG_FATAL("Unknown fatal error occurred");
        return 1;
    }

    return 0;
}
