#include "DictProducer.h"
#include "SplitTool.h"
#include "WebPage.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

using std::ifstream;
using std::ofstream;
using std::getline;

DictProducer::DictProducer(SplitTool* splitTool)
    : _splitTool(splitTool) {
}

void DictProducer::build(const vector<shared_ptr<WebPage>>& pages) {
    // 从网页库构建词典
    for (const auto& page : pages) {
        auto& wordsMap = page->getWordsMap();
        for (const auto& pair : wordsMap) {
            _dict[pair.first] += pair.second;
        }
    }

    LOG_INFO("Built dictionary with " + std::to_string(_dict.size()) + " words");

    // 构建字符索引
    buildIndex();
}

void DictProducer::buildFromFile(const string& filePath) {
    ifstream ifs(filePath);
    if (!ifs) {
        LOG_ERROR("Cannot open file: " + filePath);
        return;
    }

    string line;
    while (getline(ifs, line)) {
        if (line.empty()) continue;

        // 分词
        vector<string> words = _splitTool->cut(line);
        for (const auto& word : words) {
            _dict[word]++;
        }
    }

    LOG_INFO("Built dictionary with " + std::to_string(_dict.size()) + " words");

    // 构建字符索引
    buildIndex();
}

void DictProducer::buildIndex() {
    // 为每个词建立字符索引
    for (const auto& pair : _dict) {
        const string& word = pair.first;
        vector<string> chars = extractChars(word);

        for (const auto& ch : chars) {
            _charIndex[ch].insert(word);
        }
    }

    LOG_INFO("Built character index with " + std::to_string(_charIndex.size()) + " characters");
}

bool DictProducer::isChinese(const string& ch) const {
    if (ch.length() != 3) return false;

    unsigned char c0 = ch[0];
    unsigned char c1 = ch[1];
    unsigned char c2 = ch[2];

    return (c0 >= 0xE4 && c0 <= 0xE9) &&
           (c1 >= 0x80 && c1 <= 0xBF) &&
           (c2 >= 0x80 && c2 <= 0xBF);
}

vector<string> DictProducer::extractChars(const string& word) const {
    vector<string> chars;
    size_t i = 0;

    while (i < word.length()) {
        unsigned char c = word[i];
        size_t charLen = 1;

        if ((c & 0x80) == 0) {
            charLen = 1;
        } else if ((c & 0xE0) == 0xC0) {
            charLen = 2;
        } else if ((c & 0xF0) == 0xE0) {
            charLen = 3;
        } else if ((c & 0xF8) == 0xF0) {
            charLen = 4;
        }

        if (i + charLen <= word.length()) {
            chars.push_back(word.substr(i, charLen));
        }
        i += charLen;
    }

    return chars;
}

vector<string> DictProducer::getCandidates(const string& prefix) const {
    vector<string> candidates;

    vector<string> chars = extractChars(prefix);
    if (chars.empty()) return candidates;

    auto it = _charIndex.find(chars[0]);
    if (it == _charIndex.end()) return candidates;

    for (const auto& word : it->second) {
        bool match = true;
        for (size_t i = 1; i < chars.size() && match; ++i) {
            if (word.find(chars[i]) == string::npos) {
                match = false;
            }
        }
        if (match) {
            candidates.push_back(word);
        }
    }

    std::sort(candidates.begin(), candidates.end(),
              [this](const string& a, const string& b) {
                  auto itA = _dict.find(a);
                  auto itB = _dict.find(b);
                  int freqA = (itA != _dict.end()) ? itA->second : 0;
                  int freqB = (itB != _dict.end()) ? itB->second : 0;
                  return freqA > freqB;
              });

    return candidates;
}

void DictProducer::storeDict(const string& filePath) {
    ofstream ofs(filePath);
    if (!ofs) {
        LOG_ERROR("Cannot create dict file: " + filePath);
        return;
    }

    vector<std::pair<string, int>> sortedDict(_dict.begin(), _dict.end());
    std::sort(sortedDict.begin(), sortedDict.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });

    for (const auto& pair : sortedDict) {
        ofs << pair.first << " " << pair.second << "\n";
    }

    LOG_INFO("Stored dictionary to " + filePath);
}

void DictProducer::storeIndex(const string& filePath) {
    ofstream ofs(filePath);
    if (!ofs) {
        LOG_ERROR("Cannot create index file: " + filePath);
        return;
    }

    for (const auto& pair : _charIndex) {
        ofs << pair.first;
        for (const auto& word : pair.second) {
            ofs << " " << word;
        }
        ofs << "\n";
    }

    LOG_INFO("Stored index to " + filePath);
}

void DictProducer::loadDict(const string& filePath) {
    ifstream ifs(filePath);
    if (!ifs) {
        LOG_ERROR("Cannot open dict file: " + filePath);
        return;
    }

    _dict.clear();
    string line;
    while (getline(ifs, line)) {
        std::istringstream iss(line);
        string word;
        int freq;
        if (iss >> word >> freq) {
            _dict[word] = freq;
        }
    }

    LOG_INFO("Loaded dictionary with " + std::to_string(_dict.size()) + " words");
}

void DictProducer::loadIndex(const string& filePath) {
    ifstream ifs(filePath);
    if (!ifs) {
        LOG_ERROR("Cannot open index file: " + filePath);
        return;
    }

    _charIndex.clear();
    string line;
    while (getline(ifs, line)) {
        std::istringstream iss(line);
        string ch;
        iss >> ch;

        string word;
        while (iss >> word) {
            _charIndex[ch].insert(word);
        }
    }

    LOG_INFO("Loaded index with " + std::to_string(_charIndex.size()) + " characters");
}
