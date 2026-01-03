#include "SplitTool.h"
#include "Logger.h"
#include "cppjieba/Jieba.hpp"
#include <fstream>
#include <unordered_set>

using std::ifstream;
using std::unordered_set;
using std::move;

// pImpl 实现类
class JiebaSplitTool::Impl {
public:
    Impl(const string& dictPath,
         const string& modelPath,
         const string& userDictPath,
         const string& idfPath,
         const string& stopWordPath)
        : _jieba(dictPath.c_str(),
                 modelPath.c_str(),
                 userDictPath.c_str(),
                 idfPath.c_str(),
                 stopWordPath.c_str()) {
        loadStopWords(stopWordPath);
    }

    vector<string> cut(const string& sentence) {
        vector<string> words;
        _jieba.Cut(sentence, words, true);

        vector<string> result;
        result.reserve(words.size());

        for (auto& word : words) {
            if (_stopWords.find(word) == _stopWords.end() &&
                word.length() > 0 &&
                word != " " &&
                word != "\n") {
                result.push_back(std::move(word));
            }
        }
        return result;
    }

private:
    void loadStopWords(const string& stopWordPath) {
        ifstream ifs(stopWordPath);
        if (!ifs) {
            LOG_WARN("Cannot open stop words file: " + stopWordPath);
            return;
        }
        string word;
        while (std::getline(ifs, word)) {
            if (!word.empty()) {
                _stopWords.insert(word);
            }
        }
    }

private:
    cppjieba::Jieba _jieba;
    unordered_set<string> _stopWords;
};

JiebaSplitTool::JiebaSplitTool(const string& dictPath,
                               const string& modelPath,
                               const string& userDictPath,
                               const string& idfPath,
                               const string& stopWordPath)
    : _pImpl(new Impl(dictPath, modelPath, userDictPath, idfPath, stopWordPath)) {
}

JiebaSplitTool::~JiebaSplitTool() = default;

vector<string> JiebaSplitTool::cut(const string& sentence) {
    return _pImpl->cut(sentence);
}
