#include "PageLibPreprocessor.h"
#include "WebPage.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

using std::make_shared;

PageLibPreprocessor::PageLibPreprocessor(vector<shared_ptr<WebPage>>& pages, SplitTool* splitTool)
    : _pages(pages)
    , _splitTool(splitTool) {
}

void PageLibPreprocessor::deduplicate() {
    // 基于 SimHash 去重
    vector<uint64_t> hashes;

    for (const auto& page : _pages) {
        uint64_t hash = page->getSimhash();
        bool isDuplicate = false;

        for (const auto& existingHash : hashes) {
            if (isSimilar(hash, existingHash)) {
                isDuplicate = true;
                break;
            }
        }

        if (!isDuplicate) {
            _processedPages.push_back(page);
            hashes.push_back(hash);
        }
    }

    LOG_INFO("Deduplication: " + std::to_string(_pages.size()) + " -> " + std::to_string(_processedPages.size()) + " pages");
}

bool PageLibPreprocessor::isSimilar(uint64_t hash1, uint64_t hash2, int threshold) {
    // 计算汉明距离
    uint64_t diff = hash1 ^ hash2;
    int distance = 0;

    while (diff) {
        distance += diff & 1;
        diff >>= 1;
    }

    return distance < threshold;
}
