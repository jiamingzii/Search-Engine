#include "InvertIndex.h"
#include "WebPage.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::log;
using std::sort;
using std::partial_sort;

InvertIndex::InvertIndex()
    : _totalDocs(0)
    , _avgDocLen(0) {
}

void InvertIndex::build(vector<shared_ptr<WebPage>>& pages) {
    _totalDocs = pages.size();
    if (_totalDocs == 0) {
        LOG_WARN("No pages to build index");
        return;
    }

    // 第一步：统计 DF (Document Frequency)
    unordered_map<string, int> docFreq;
    long long totalLen = 0;

    for (const auto& page : pages) {
        auto& wordsMap = page->getWordsMap();
        int docLen = 0;
        for (const auto& pair : wordsMap) {
            docLen += pair.second;
            docFreq[pair.first]++;
        }
        _docLens[page->getDocId()] = docLen;
        totalLen += docLen;
    }

    _avgDocLen = (double)totalLen / _totalDocs;
    LOG_INFO("Average document length: " + std::to_string(_avgDocLen));

    // 第二步：构建倒排索引
    for (const auto& page : pages) {
        auto& wordsMap = page->getWordsMap();
        int docId = page->getDocId();
        int docLen = _docLens[docId];

        for (const auto& pair : wordsMap) {
            const string& word = pair.first;
            int termFreq = pair.second;

            double weight = calculateBM25(termFreq, docLen, docFreq[word]);

            InvertIndexItem item;
            item.docId = docId;
            item.weight = weight;
            item.termFreq = termFreq;

            _invertIndex[word].push_back(item);
        }
    }

    // 对每个词的倒排列表排序
    for (auto& pair : _invertIndex) {
        sort(pair.second.begin(), pair.second.end(),
             [](const InvertIndexItem& a, const InvertIndexItem& b) {
                 return a.weight > b.weight;
             });
    }

    LOG_INFO("Built inverted index with " + std::to_string(_invertIndex.size()) + " terms (BM25)");
}

double InvertIndex::calculateIDF(int docFreq, int totalDocs) {
    if (docFreq == 0) return 0;
    double idf = log((totalDocs - docFreq + 0.5) / (docFreq + 0.5) + 1.0);
    return idf > 0 ? idf : 0;
}

double InvertIndex::calculateBM25(int termFreq, int docLen, int docFreq) {
    double idf = calculateIDF(docFreq, _totalDocs);
    double tfNorm = (termFreq * (K1 + 1)) /
                    (termFreq + K1 * (1 - B + B * (docLen / _avgDocLen)));
    return idf * tfNorm;
}

vector<pair<int, double>> InvertIndex::search(const vector<string>& queryWords, int topK) {
    if (queryWords.empty()) return {};

    int maxDocId = _docLens.empty() ? 0 : _docLens.rbegin()->first;
    vector<double> scores(maxDocId + 1, 0.0);
    vector<int> dirtyDocIds;

    for (const auto& word : queryWords) {
        auto it = _invertIndex.find(word);
        if (it != _invertIndex.end()) {
            for (const auto& item : it->second) {
                if (scores[item.docId] == 0.0) {
                    dirtyDocIds.push_back(item.docId);
                }
                scores[item.docId] += item.weight;
            }
        }
    }

    vector<pair<int, double>> results;
    results.reserve(dirtyDocIds.size());
    for (int docId : dirtyDocIds) {
        results.emplace_back(docId, scores[docId]);
    }

    if (results.size() > (size_t)topK) {
        partial_sort(results.begin(),
                     results.begin() + topK,
                     results.end(),
                     [](const pair<int, double>& a, const pair<int, double>& b) {
                         return a.second > b.second;
                     });
        results.resize(topK);
    } else {
        sort(results.begin(), results.end(),
             [](const pair<int, double>& a, const pair<int, double>& b) {
                 return a.second > b.second;
             });
    }

    return results;
}

void InvertIndex::store(const string& filePath) {
    ofstream ofs(filePath);
    if (!ofs) {
        LOG_ERROR("Cannot create index file: " + filePath);
        return;
    }

    ofs << "#META " << _totalDocs << " " << _avgDocLen << "\n";
    ofs << "#DOCLENS";
    for (const auto& pair : _docLens) {
        ofs << " " << pair.first << ":" << pair.second;
    }
    ofs << "\n";

    for (const auto& pair : _invertIndex) {
        ofs << pair.first;
        for (const auto& item : pair.second) {
            ofs << " " << item.docId << ":" << item.weight << ":" << item.termFreq;
        }
        ofs << "\n";
    }
    LOG_INFO("Stored index to " + filePath);
}

void InvertIndex::load(const string& filePath) {
    ifstream ifs(filePath);
    if (!ifs) {
        LOG_ERROR("Cannot open index file: " + filePath);
        return;
    }

    _invertIndex.clear();
    _docLens.clear();
    string line;

    while (std::getline(ifs, line)) {
        if (line.empty()) continue;

        if (line.size() >= 5 && line.compare(0, 5, "#META") == 0) {
            stringstream ss(line.substr(6));
            ss >> _totalDocs >> _avgDocLen;
        } else if (line.compare(0, 8, "#DOCLENS") == 0) {
            stringstream ss(line.substr(9));
            string item;
            while (ss >> item) {
                size_t pos = item.find(':');
                if (pos != string::npos) {
                    int docId = std::stoi(item.substr(0, pos));
                    int docLen = std::stoi(item.substr(pos + 1));
                    _docLens[docId] = docLen;
                }
            }
        } else {
            size_t firstSpace = line.find(' ');
            if (firstSpace == string::npos) continue;

            string word = line.substr(0, firstSpace);

            size_t curPos = firstSpace + 1;
            while (curPos < line.size()) {
                size_t nextSpace = line.find(' ', curPos);
                size_t itemLen = (nextSpace == string::npos) ? (line.size() - curPos) : (nextSpace - curPos);

                string itemStr = line.substr(curPos, itemLen);

                size_t p1 = itemStr.find(':');
                size_t p2 = itemStr.find(':', p1 + 1);

                if (p1 != string::npos && p2 != string::npos) {
                    InvertIndexItem idxItem;
                    idxItem.docId = std::stoi(itemStr.substr(0, p1));
                    idxItem.weight = std::stod(itemStr.substr(p1 + 1, p2 - p1 - 1));
                    idxItem.termFreq = std::stoi(itemStr.substr(p2 + 1));
                    _invertIndex[word].push_back(idxItem);
                }

                if (nextSpace == string::npos) break;
                curPos = nextSpace + 1;
            }
        }
    }

    LOG_INFO("Loaded index with " + std::to_string(_invertIndex.size()) + " terms (BM25)");
}
