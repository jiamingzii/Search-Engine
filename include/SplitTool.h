#ifndef __SPLIT_TOOL_H__
#define __SPLIT_TOOL_H__

#include <string>
#include <vector>
#include <memory>

using std::string;
using std::vector;

// 分词工具基类
class SplitTool {
public:
    virtual ~SplitTool() = default;
    virtual vector<string> cut(const string& sentence) = 0;
};

// 结巴分词实现
class JiebaSplitTool : public SplitTool {
public:
    JiebaSplitTool(const string& dictPath,
                   const string& modelPath,
                   const string& userDictPath,
                   const string& idfPath,
                   const string& stopWordPath);
    ~JiebaSplitTool();

    vector<string> cut(const string& sentence) override;

private:
    class Impl;  // pImpl 模式，隐藏 cppjieba 头文件依赖
    std::unique_ptr<Impl> _pImpl;
};

#endif // __SPLIT_TOOL_H__
