#ifndef __WEB_PAGE_META_H__
#define __WEB_PAGE_META_H__

#include <string>
#include <fstream>
#include <vector>
#include <memory>

using std::string;
using std::vector;

// 轻量级网页元数据（不存储正文内容）
struct WebPageMeta {
    int docId;
    string title;
    string url;
    size_t contentOffset;  // 正文在文件中的偏移量
    size_t contentLength;  // 正文长度

    WebPageMeta() : docId(0), contentOffset(0), contentLength(0) {}
};

// 内容存储器：负责从磁盘读取正文
class ContentStore {
public:
    explicit ContentStore(const string& contentFilePath)
        : _filePath(contentFilePath) {
    }

    // 根据偏移量读取内容
    string readContent(size_t offset, size_t length) const {
        std::ifstream ifs(_filePath, std::ios::binary);
        if (!ifs) {
            return "";
        }
        ifs.seekg(offset);
        string content(length, '\0');
        ifs.read(&content[0], length);
        return content;
    }

    // 生成摘要（延迟读取版本）
    string getSummary(size_t offset, size_t length,
                      const vector<string>& queryWords,
                      size_t maxChars = 150) const {
        // 只读取需要的部分，而不是整个正文
        // 为了找到关键词上下文，我们先读取一部分
        size_t readLength = std::min(length, (size_t)5000);  // 最多读取5KB用于摘要

        std::ifstream ifs(_filePath, std::ios::binary);
        if (!ifs) {
            return "";
        }
        ifs.seekg(offset);
        string text(readLength, '\0');
        ifs.read(&text[0], readLength);

        if (text.empty()) return "";

        size_t start = 0;
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

private:
    string _filePath;
};

#endif // __WEB_PAGE_META_H__
