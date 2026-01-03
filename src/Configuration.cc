#include "Configuration.h"
#include "Logger.h"
#include <fstream>

using std::ifstream;

Configuration* Configuration::_pInstance = nullptr;

Configuration* Configuration::getInstance() {
    if (_pInstance == nullptr) {
        _pInstance = new Configuration();
    }
    return _pInstance;
}

void Configuration::load(const string& configPath) {
    ifstream ifs(configPath);
    if (!ifs) {
        LOG_ERROR("Cannot open config file: " + configPath);
        return;
    }

    string line;
    while (std::getline(ifs, line)) {
        // 跳过空行和注释
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // 解析 key = value
        size_t pos = line.find('=');
        if (pos != string::npos) {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);

            // 去除首尾空格
            auto trim = [](string& s) {
                size_t start = s.find_first_not_of(" \t");
                size_t end = s.find_last_not_of(" \t");
                if (start != string::npos && end != string::npos) {
                    s = s.substr(start, end - start + 1);
                }
            };

            trim(key);
            trim(value);
            _configs[key] = value;
        }
    }
    LOG_INFO("Loaded " + std::to_string(_configs.size()) + " config items");
}

string Configuration::get(const string& key) const {
    auto it = _configs.find(key);
    if (it != _configs.end()) {
        return it->second;
    }
    return "";
}
