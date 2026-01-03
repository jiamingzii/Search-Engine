#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include <string>
#include <map>

using std::string;
using std::map;

// 配置文件单例类
class Configuration {
public:
    static Configuration* getInstance();

    void load(const string& configPath);
    string get(const string& key) const;

private:
    Configuration() = default;
    ~Configuration() = default;
    Configuration(const Configuration&) = delete;
    Configuration& operator=(const Configuration&) = delete;

private:
    static Configuration* _pInstance;
    map<string, string> _configs;
};

#endif // __CONFIGURATION_H__
