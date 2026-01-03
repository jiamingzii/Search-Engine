#ifndef __SEARCH_LRU_CACHE_H__
#define __SEARCH_LRU_CACHE_H__

#include <list>
#include <unordered_map>
#include <string>
#include <mutex>
#include <array>
#include <atomic>
#include <functional>

using std::list;
using std::unordered_map;
using std::string;
using std::pair;
using std::mutex;
using std::lock_guard;

// 单个 LRU 分片
template<typename K, typename V>
class LRUShard {
public:
    explicit LRUShard(size_t capacity)
        : _capacity(capacity) {
    }

    bool get(const K& key, V& value) {
        lock_guard<mutex> lock(_mutex);
        auto it = _index.find(key);
        if (it == _index.end()) {
            return false;
        }
        _cache.splice(_cache.begin(), _cache, it->second);
        value = it->second->second;
        return true;
    }

    void put(const K& key, const V& value) {
        lock_guard<mutex> lock(_mutex);
        auto it = _index.find(key);
        if (it != _index.end()) {
            it->second->second = value;
            _cache.splice(_cache.begin(), _cache, it->second);
            return;
        }
        if (_cache.size() >= _capacity) {
            auto last = _cache.back();
            _index.erase(last.first);
            _cache.pop_back();
        }
        _cache.push_front({key, value});
        _index[key] = _cache.begin();
    }

    bool contains(const K& key) {
        lock_guard<mutex> lock(_mutex);
        return _index.find(key) != _index.end();
    }

    size_t size() { 
        lock_guard<mutex> lock(_mutex);
        return _cache.size();
    }

    void clear() {
        lock_guard<mutex> lock(_mutex);
        _cache.clear();
        _index.clear();
    }

private:
    size_t _capacity;
    list<pair<K, V>> _cache;
    unordered_map<K, typename list<pair<K, V>>::iterator> _index;
    mutex _mutex;
};

// 分段锁 LRU 缓存
template<typename K, typename V, size_t ShardCount = 16>
class ShardedLRUCache {
public:
    explicit ShardedLRUCache(size_t totalCapacity)
        : _totalCapacity(totalCapacity) {
        size_t perShardCapacity = std::max(totalCapacity / ShardCount, (size_t)1);
        for (size_t i = 0; i < ShardCount; ++i) {
            _shards[i] = std::make_unique<LRUShard<K, V>>(perShardCapacity);
        }
    }

    bool get(const K& key, V& value) {
        return getShard(key).get(key, value);
    }

    void put(const K& key, const V& value) {
        getShard(key).put(key, value);
    }

    bool contains(const K& key) {
        return getShard(key).contains(key);
    }

    size_t size() {
        size_t total = 0;
        for (size_t i = 0; i < ShardCount; ++i) {
            total += _shards[i]->size();
        }
        return total;
    }

    void clear() {
        for (size_t i = 0; i < ShardCount; ++i) {
            _shards[i]->clear();
        }
    }

    double hitRate() const {
        size_t total = _totalQueries.load();
        if (total == 0) return 0;
        return (double)_hits.load() / total;
    }

    void recordQuery(bool hit) {
        _totalQueries.fetch_add(1, std::memory_order_relaxed);
        if (hit) {
            _hits.fetch_add(1, std::memory_order_relaxed);
        }
    }

private:
    LRUShard<K, V>& getShard(const K& key) {
        size_t hash = std::hash<K>{}(key);
        return *_shards[hash % ShardCount];
    }

    size_t _totalCapacity;
    std::array<std::unique_ptr<LRUShard<K, V>>, ShardCount> _shards;
    std::atomic<size_t> _totalQueries{0};
    std::atomic<size_t> _hits{0};
};

// 保持向后兼容的类型别名
template<typename K, typename V>
using SearchLRUCache = ShardedLRUCache<K, V, 16>;

using SearchCache = SearchLRUCache<string, string>;

#endif
