#include "PageLib.h"
#include "WebPage.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>

using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::make_shared;
// 最大文档数量限制（防止内存溢出）
static const size_t MAX_DOCS = 300000;  // 30万篇上限  可修改

PageLib::PageLib(const string& dataPath, SplitTool* splitTool)
    : _dataPath(dataPath)
    , _splitTool(splitTool) {
}

void PageLib::load() {
    // 遍历数据目录，加载所有文件
    DIR* dir = opendir(_dataPath.c_str());
    if (!dir) {
        LOG_ERROR("Cannot open data directory: " + _dataPath);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        string filename = entry->d_name;
        if (filename == "." || filename == "..") {
            continue;
        }

        string filepath = _dataPath + "/" + filename;
        struct stat st;
        // 支持 .xml 和 .dat 文件格式
        if (stat(filepath.c_str(), &st) == 0 && S_ISREG(st.st_mode) &&
            (filename.find(".xml") != string::npos || filename.find(".dat") != string::npos)) {
            parseFile(filepath);
             if (_pages.size() >= MAX_DOCS) {
                  break;
              }
        }
    }
    closedir(dir);

    LOG_INFO("Loaded " + std::to_string(_pages.size()) + " pages");
}

void PageLib::parseFile(const string& filePath) {
    ifstream ifs(filePath);
    if (!ifs) {
        LOG_WARN("Cannot open file: " + filePath);
        return;
    }

    // 流式读取：分块读取文件，边读边解析，避免一次性加载整个文件
    const size_t CHUNK_SIZE = 1024 * 1024; // 1MB 分块
    char* chunk = new char[CHUNK_SIZE];
    string buffer;
    buffer.reserve(CHUNK_SIZE * 2);

    const string docStart = "<doc>";
    const string docEnd = "</doc>";
    size_t processedCount = 0;
    size_t initialSize = _pages.size();

    while (ifs.read(chunk, CHUNK_SIZE) || ifs.gcount() > 0) {
        buffer.append(chunk, ifs.gcount());

        // 处理所有完整的 <doc>...</doc> 块
        size_t searchStart = 0;
        while (true) {
            size_t startPos = buffer.find(docStart, searchStart);
            if (startPos == string::npos) {
                // 没有找到 <doc>，清理前面的内容
                buffer.clear();
                break;
            }

            size_t endPos = buffer.find(docEnd, startPos);
            if (endPos == string::npos) {
                // 有 <doc> 但没有 </doc>，保留从 startPos 开始的内容
                if (startPos > 0) {
                    buffer = buffer.substr(startPos);
                }
                break;
            }

            // 提取并处理完整的文档
            string doc = buffer.substr(startPos, endPos - startPos + docEnd.length());
            auto page = make_shared<WebPage>(doc, _splitTool);
            _pages.push_back(page);
            if (_pages.size() >= MAX_DOCS) {
                  LOG_INFO("Reached max document limit: " + std::to_string(MAX_DOCS));
                  delete[] chunk;
                  return;
              }
            processedCount++;
            if (processedCount % 10000 == 0) {
                LOG_INFO("Loaded " + std::to_string(processedCount) + " documents...");
            }

            searchStart = endPos + docEnd.length();
        }

        // 清理已处理的内容，保留未处理的部分
        if (searchStart > 0 && searchStart < buffer.length()) {
            buffer = buffer.substr(searchStart);
        } else if (searchStart >= buffer.length()) {
            buffer.clear();
        }
    }

    delete[] chunk;

    // 如果没有解析到任何文档，尝试把整个文件当作一个文档（兼容旧格式）
    if (_pages.size() == initialSize) {
        ifs.clear();
        ifs.seekg(0);
        stringstream ss;
        ss << ifs.rdbuf();
        string content = ss.str();
        if (!content.empty()) {
            auto page = make_shared<WebPage>(content, _splitTool);
            _pages.push_back(page);
        }
    }
}

void PageLib::store(const string& outputPath) {
    ofstream ofs(outputPath);
    if (!ofs) {
        LOG_ERROR("Cannot create output file: " + outputPath);
        return;
    }

    for (const auto& page : _pages) {
        ofs << "<doc>\n";
        ofs << "<docid>" << page->getDocId() << "</docid>\n";
        ofs << "<title>" << page->getTitle() << "</title>\n";
        ofs << "<url>" << page->getUrl() << "</url>\n";
        ofs << "<content>" << page->getContent() << "</content>\n";
        ofs << "</doc>\n\n";
    }

    LOG_INFO("Stored " + std::to_string(_pages.size()) + " pages to " + outputPath);
}

void PageLib::storeSeparated(const string& metaPath, const string& contentPath) {
    // 1. 写入内容文件（二进制）
    ofstream contentOfs(contentPath, std::ios::binary);
    if (!contentOfs) {
        LOG_ERROR("Cannot create content file: " + contentPath);
        return;
    }

    // 2. 写入元数据文件
    ofstream metaOfs(metaPath);
    if (!metaOfs) {
        LOG_ERROR("Cannot create meta file: " + metaPath);
        return;
    }

    metaOfs << "#FORMAT docId|title|url|offset|length\n";

    size_t currentOffset = 0;
for (const auto& page : _pages) {
          string content = page->getContent();
          size_t contentLen = content.length();

          // 写入内容
          contentOfs.write(content.c_str(), contentLen);

          // 清理标题和URL中的特殊字符（换行符和分隔符）
          string title = page->getTitle();
          string url = page->getUrl();
          std::replace(title.begin(), title.end(), '\n', ' ');
          std::replace(title.begin(), title.end(), '\r', ' ');
          std::replace(title.begin(), title.end(), '|', ' ');
          std::replace(url.begin(), url.end(), '\n', ' ');
          std::replace(url.begin(), url.end(), '\r', ' ');
          std::replace(url.begin(), url.end(), '|', ' ');

          // 写入元数据（使用 | 分隔，避免标题中的空格问题）
          metaOfs << page->getDocId() << "|"
                  << title << "|"
                  << url << "|"
                  << currentOffset << "|"
                  << contentLen << "\n";

          currentOffset += contentLen;
      }

    LOG_INFO("Stored " + std::to_string(_pages.size()) + " pages (separated format)");
    LOG_INFO("  Meta: " + metaPath);
    LOG_INFO("  Content: " + contentPath + " (" + std::to_string(currentOffset) + " bytes)");
}

unordered_map<int, WebPageMeta> PageLib::loadMeta(const string& metaPath) {
    unordered_map<int, WebPageMeta> result;

    ifstream ifs(metaPath);
    if (!ifs) {
        LOG_ERROR("Cannot open meta file: " + metaPath);
        return result;
    }

    string line;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#') continue;

        // 解析：docId|title|url|offset|length
        size_t p1 = line.find('|');
        size_t p2 = line.find('|', p1 + 1);
        size_t p3 = line.find('|', p2 + 1);
        size_t p4 = line.find('|', p3 + 1);

        if (p1 == string::npos || p2 == string::npos ||
            p3 == string::npos || p4 == string::npos) {
            continue;
        }

        WebPageMeta meta;
        meta.docId = std::stoi(line.substr(0, p1));
        meta.title = line.substr(p1 + 1, p2 - p1 - 1);
        meta.url = line.substr(p2 + 1, p3 - p2 - 1);
        meta.contentOffset = std::stoull(line.substr(p3 + 1, p4 - p3 - 1));
        meta.contentLength = std::stoull(line.substr(p4 + 1));

        result[meta.docId] = meta;
    }

    LOG_INFO("Loaded " + std::to_string(result.size()) + " page metadata entries");
    return result;
}
