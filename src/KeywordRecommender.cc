#include "KeywordRecommender.h"
#include "DictProducer.h"
#include <algorithm>

using std::min;

KeywordRecommender::KeywordRecommender(const DictProducer* dictProducer)
    : _dictProducer(dictProducer) {
}

vector<string> KeywordRecommender::splitToChars(const string& s) {
    vector<string> chars;
    size_t i = 0;

    while (i < s.length()) {
        unsigned char c = s[i];
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

        if (i + charLen <= s.length()) {
            chars.push_back(s.substr(i, charLen));
        }
        i += charLen;
    }

    return chars;
}

int KeywordRecommender::editDistance(const string& s1, const string& s2) {
    vector<string> chars1 = splitToChars(s1);
    vector<string> chars2 = splitToChars(s2);

    int m = chars1.size();
    int n = chars2.size();

    vector<vector<int>> dp(m + 1, vector<int>(n + 1, 0));

    for (int i = 0; i <= m; ++i) dp[i][0] = i;
    for (int j = 0; j <= n; ++j) dp[0][j] = j;

    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (chars1[i - 1] == chars2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + min({
                    dp[i - 1][j],
                    dp[i][j - 1],
                    dp[i - 1][j - 1]
                });
            }
        }
    }

    return dp[m][n];
}

vector<string> KeywordRecommender::recommend(const string& query, int topK, int maxDistance) {
    vector<string> results;

    if (!_dictProducer) {
        return results;
    }

    vector<string> queryChars = splitToChars(query);
    priority_queue<CandidateWord> pq;

    const auto& dict = _dictProducer->getDict();

    for (const auto& pair : dict) {
        const string& word = pair.first;
        int freq = pair.second;

        vector<string> wordChars = splitToChars(word);

        int distDiff = (int)queryChars.size() - (int)wordChars.size();
        if (std::abs(distDiff) > maxDistance) {
            continue;
        }

        int dist = editDistance(query, word);

        if (dist <= maxDistance) {
            CandidateWord candidate;
            candidate.word = word;
            candidate.distance = dist;
            candidate.frequency = freq;
            pq.push(candidate);
        }
    }

    while (!pq.empty() && (int)results.size() < topK) {
        results.push_back(pq.top().word);
        pq.pop();
    }

    return results;
}
