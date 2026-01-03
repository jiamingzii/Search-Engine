#ifndef __PAGE_LIB_H__
#define __PAGE_LIB_H__

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "WebPageMeta.h"

using std::string;
using std::vector;
using std::shared_ptr;
using std::unordered_map;

class WebPage;
class SplitTool;

// 网页库：管理所有网页
class PageLib {
public:
    PageLib(const string& dataPath, SplitTool* splitTool);

    // 从文件加载网页
    void load();

    // 获取所有网页
    vector<shared_ptr<WebPage>>& getPages() { return _pages; }

    // 存储网页库到文件
    void store(const string& outputPath);

    // === 新增：轻量级存储方案 ===
    // 分离存储：元数据文件 + 内容文件
    void storeSeparated(const string& metaPath, const string& contentPath);

    // 加载轻量级元数据（不加载正文内容）
    static unordered_map<int, WebPageMeta> loadMeta(const string& metaPath);

private:
    // 解析单个文件
    void parseFile(const string& filePath);

private:
    string _dataPath;
    SplitTool* _splitTool;
    vector<shared_ptr<WebPage>> _pages;
};

#endif // __PAGE_LIB_H__