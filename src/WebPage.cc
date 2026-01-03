#include "WebPage.h"
#include "SplitTool.h"
#include <regex>
#include <algorithm>
#include <functional>

using std::regex;
using std::regex_search;
using std::smatch;

int WebPage::_idGen = 0;

WebPage::WebPage(const string& doc, SplitTool* splitTool)
    : _docId(++_idGen)
    , _splitTool(splitTool) {
    processDoc(doc);
}

void WebPage::processDoc(const string& doc) {
    // 解析 XML 格式的文档
    // <doc>
    //   <docid>1</docid>
    //   <title>标题</title>
    //   <url>http://...</url>
    //   <content>内容</content>
    // </doc>

    smatch match;

    // 提取标题 - 兼容 <title> 和 <contenttitle> 标签
    static const regex titleRegex("<(?:content)?title>([\\s\\S]*?)</(?:content)?title>");
    //              源文件 结果存放   提取的标识
    if (regex_search(doc, match, titleRegex)) {
        _title = match[1].str();
    }

    // 提取 URL
    static const regex urlRegex("<url>([\\s\\S]*?)</url>");
    if (regex_search(doc, match, urlRegex)) {
        _url = match[1].str();
    }

    // 提取内容
    static const regex contentRegex("<content>([\\s\\S]*?)</content>");
    if (regex_search(doc, match, contentRegex)) {
        _content = match[1].str();
    }

    // 如果没有 XML 标签，直接使用原文
    if (_title.empty() && _content.empty()) {
        _content = doc;
        _title = doc.substr(0, std::min((size_t)50, doc.length()));
    }

    // 分词并统计词频
    string text = _title + " " + _content;
    vector<string> words = _splitTool->cut(text);

    for (const auto& word : words) {
        _wordsMap[word]++;
    }
}

// Jenkins hash 函数（用于 SimHash）
static uint64_t jenkinsHash(const string& key) {
    uint64_t hash = 0;
    for (size_t i = 0; i < key.length(); ++i) {
        hash += static_cast<uint8_t>(key[i]);
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

uint64_t WebPage::getSimhash() const {
    // 真正的 SimHash 实现
    // 1. 对每个词计算64位hash
    // 2. 根据词频作为权重，对每一位进行加权统计
    // 3. 每一位权重和大于0则该位为1，否则为0

    // 64位的权重数组
    double weights[64] = {0};

    for (const auto& pair : _wordsMap) {
        const string& word = pair.first;
        int freq = pair.second;  // 词频作为权重

        // 计算词的64位hash
        uint64_t wordHash = jenkinsHash(word);

        // 对每一位进行加权
        for (int i = 0; i < 64; ++i) {
            if ((wordHash >> i) & 1) {
                weights[i] += freq;  // 该位为1，加权重
            } else {
                weights[i] -= freq;  // 该位为0，减权重
            }
        }
    }

    // 根据权重生成最终的 SimHash
    uint64_t simhash = 0;
    for (int i = 0; i < 64; ++i) {
        if (weights[i] > 0) {
            simhash |= (1ULL << i);
        }
    }

    return simhash;
}

// 计算汉明距离
int WebPage::hammingDistance(uint64_t h1, uint64_t h2) {
    uint64_t x = h1 ^ h2;
    int count = 0;
    while (x) {
        count += x & 1;
        x >>= 1;
    }
    return count;
}




 string WebPage::getSummary(const vector<string>& queryWords) const {
      if (_content.empty()) return "";

      size_t maxChars = 150;
      string text = _content;
      size_t start = 0;
//在文档里面找匹配第一个查询词的结果，前30共150个字符
      for (const auto& word : queryWords) {
          size_t pos = text.find(word);
          if (pos != string::npos && pos > 30) {
              start = pos - 30;
              break;
          }
      }

      size_t charCount = 0;
      size_t endPos = start;

      while (endPos < text.length() && charCount < maxChars) {
          unsigned char c = text[endPos];
          size_t charLen = 1;
          if ((c & 0x80) == 0) charLen = 1;
          else if ((c & 0xE0) == 0xC0) charLen = 2;
          else if ((c & 0xF0) == 0xE0) charLen = 3;
          else if ((c & 0xF8) == 0xF0) charLen = 4;

          if (endPos + charLen > text.length()) break;
          endPos += charLen;
          charCount++;
      }

      string summary = text.substr(start, endPos - start);
      if (start > 0) summary = "..." + summary;
      if (endPos < text.length()) summary += "...";

      return summary;
 }
