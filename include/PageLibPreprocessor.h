#ifndef __PAGE_LIB_PREPROCESSOR_H__
#define __PAGE_LIB_PREPROCESSOR_H__

#include <string>
#include <vector>
#include <set>
#include <memory>

using std::string;
using std::vector;
using std::set;
using std::shared_ptr;

class WebPage;
class SplitTool;

// 网页预处理器：去重、构建索引
class PageLibPreprocessor {
public:
    PageLibPreprocessor(vector<shared_ptr<WebPage>>& pages, SplitTool* splitTool);

    // 网页去重（基于 SimHash）
    void deduplicate();

    // 获取去重后的网页
    vector<shared_ptr<WebPage>>& getProcessedPages() { return _processedPages; }

private:
    // 判断两个 SimHash 是否相似（汉明距离 < 阈值）
    bool isSimilar(uint64_t hash1, uint64_t hash2, int threshold = 3);

private:
    vector<shared_ptr<WebPage>>& _pages;
    vector<shared_ptr<WebPage>> _processedPages;
    SplitTool* _splitTool;
};

#endif // __PAGE_LIB_PREPROCESSOR_H__
