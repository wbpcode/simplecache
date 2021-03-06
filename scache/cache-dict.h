#pragma once

#include "cache-base.h"
#include "cache-list.h"
#include <functional>
#include <iostream>

template<class K,class V>
struct CachePair {
    using KType = K;
    using VType = V;
    KType m_one;
    VType m_two;
};

template<class K,class V>
class CacheDict : public CacheBase {
public:
    using KType = K;
    using VType = V ;

    using PairType = CachePair<KType, VType>;

    using BucketType = CacheList<PairType>;
    using BucketNodeType = CacheListNode<PairType>;

    using SizeType = long long;

private:
    SizeType m_oldSize = -1;
    SizeType m_nowSize = -1;

    SizeType m_useSize = 0;

    BucketType** m_now = nullptr;
    BucketType** m_old = nullptr;

    bool m_isRehash = false;
    SizeType m_rehash = -1;

    double m_loadFactor = 1;
    double m_growFactor = 2;

    std::hash<KType> hash = std::hash<KType>{};

    void rehashStep() {
        BucketType* oldBucket= m_old[m_rehash];
        if (!oldBucket) {
            if ((--m_rehash) < 0) {
                delete[] m_old;
                m_old = nullptr; m_oldSize = -1;
                m_isRehash = false;
            }
            return;
        }
        while (oldBucket->getSize() > 0) { 
            auto node = oldBucket->popNode();
            auto pos = hash(node->getValue().m_one) 
                % (size_t)m_nowSize;
            BucketType* nowBucket = m_now[pos];
            if (!nowBucket)
                m_now[pos] = new BucketType();
            nowBucket = m_now[pos];
            nowBucket->addNode(node);
        }
        delete oldBucket; m_old[m_rehash] = nullptr;
        if ((--m_rehash) < 0) {
            delete[] m_old; 
            m_old = nullptr; m_oldSize = -1;
            m_isRehash = false;
        }
    }

    // 在rehash阶段进行set可能存在节点在两个数组之间的移动所以使用
    // 单个函数来操作两个数组
    void setImpl(PairType& pair) {
        SizeType nowPos = hash(pair.m_one) % size_t(m_nowSize);
        BucketType* nowBucket = m_now[nowPos];
        if (!nowBucket) m_now[nowPos] = new BucketType();
        nowBucket = m_now[nowPos];

        std::function<bool(KType&, BucketNodeType*)> func =
            [](KType& key, BucketNodeType* node) {
            return node->getValue().m_one == key;
        };

        auto nowNode = nowBucket->getNode(pair.m_one, func);
        if (nowNode) {
            // m_now中存在，直接更新
            nowNode->getValue().m_two = pair.m_two;
            return;
        }
        // m_now中不存在且非rehash阶段：新数据直接添加
        if (!m_isRehash) {
            nowBucket->add(pair);
            m_useSize++;
            return;
        }
        // m_now中不存在且在rehash阶段：进一步搜索m_old
        SizeType oldPos = hash(pair.m_one) % size_t(m_oldSize);
        BucketType* oldBucket = m_old[oldPos];

        if (!oldBucket) {
            nowBucket->add(pair);
            m_useSize++;
            return;
        }
        
        auto oldNode = oldBucket->getNode(pair.m_one, func);
        if (oldNode) {
            // m_old中存在：更新节点，调整节点位置
            oldNode->getValue().m_two = pair.m_two;
            oldBucket->popNode(oldNode);
            nowBucket->addNode(oldNode);
        }
        else {
            nowBucket->add(pair);
            m_useSize++;
        }
    }

    PairType& getImpl(BucketType** arr, KType& key, SizeType arrSize) {
        SizeType pos = hash(key) % (size_t)arrSize;
        BucketType* bucket = arr[pos];
        if (!bucket) {
            throw std::string("Try get a Key not exist.");
        }

        std::function<bool(KType&, BucketNodeType*)> func =
            [](KType& key, BucketNodeType* node) {
            return node->getValue().m_one == key;
        };

        auto node = bucket->getNode(key,func);

        if (!node) {
            throw std::string("Try get a Key not exist.");
        }
        return node->getValue();
    }

    void delImpl(BucketType** arr, KType& key, SizeType arrSize) {
        SizeType pos = hash(key) % (size_t)arrSize;
        BucketType* bucket = arr[pos];
        if (!bucket) return;

        std::function<bool(KType&, BucketNodeType*)> func = 
            [](KType& key, BucketNodeType* node) {
            return node->getValue().m_one == key;
        };

        auto node = bucket->getNode(key, func);
        if (!node) return;
        bucket->pop(node);
        m_useSize--;
    }

    bool hasImpl(BucketType** arr, KType& key, SizeType arrSize) {
        SizeType pos = hash(key) % (size_t)arrSize;
        BucketType* bucket = arr[pos];
        if (!bucket) return false;

        std::function<bool(KType&, BucketNodeType*)> func =
            [](KType& key, BucketNodeType* node) {
            return node->getValue().m_one == key;
        };

        auto node = bucket->getNode(key,func);
        if (!node) return false;
        return true;
    }

public:
    int64 walk(std::function<void(const PairType&)> func, 
        int64 maxSize = LLONG_MAX) {
        int64 count = 0;
        for (int64 i = 0; i < m_nowSize; i++) {
            if (count >= maxSize)break;
            BucketType* temp = m_now[i];
            if (!temp || temp->getSize() <= 0)
                continue;
            BucketNodeType* node = temp->getHead()->getNext();
            while (node) {
                if (count >= maxSize)break;
                func(node->getValue());
                node = node->getNext();
                count++;
            }
        }
        if (!m_isRehash) return count;
        for (int64 i = 0; i < m_oldSize; i++) {
            if (count >= maxSize)break;
            BucketType* temp = m_old[i];
            if (!temp || temp->getSize() <= 0)
                continue;
            BucketNodeType* node = temp->getHead()->getNext();
            while (node) {
                if (count >= maxSize)break;
                func(node->getValue());
                node = node->getNext();
                count++;
            }
        }
        return count;
    }


    CacheDict(SizeType initSize = 32) : CacheBase(DictType) {
        m_now = new BucketType*[initSize]();
        m_nowSize = initSize;
    }
    virtual ~CacheDict() {
        for (int i = 0; i < m_oldSize; i++) {
            if (m_old[i]) delete m_old[i];
        }
        for (int i = 0; i < m_nowSize; i++) {
            if (m_now[i]) delete m_now[i];
        }
        delete[] m_old;
        delete[] m_now;
    }

    void set(PairType& pair) {
        setImpl(pair);

        if (m_useSize >= m_nowSize && !m_isRehash) {
            m_old = m_now;
            m_oldSize = m_nowSize;
            m_now = new BucketType*[m_oldSize * m_growFactor]();
            m_nowSize = m_oldSize * 2;
            m_rehash = m_oldSize - 1;
            m_isRehash = true;
        }
        if (m_isRehash) rehashStep();
    }
    void del(KType key) {
        delImpl(m_now, key, m_nowSize);
        if (m_isRehash) {
            delImpl(m_old, key, m_oldSize);
            rehashStep();
        } 
    }
    PairType& get(KType key) {
        if (m_isRehash) rehashStep();
        std::string message;
        try {
            return getImpl(m_now, key, m_nowSize);
        }
        catch (std::string str) {
            message = str;
        }
        if (!m_isRehash) throw message;
        try {
            return getImpl(m_old, key, m_oldSize);
        }
        catch (std::string str) {
            throw str;
        }
    }

    bool has(KType key) {
        bool temp = hasImpl(m_now, key, m_nowSize);
        if (m_isRehash && !temp) {
            temp = hasImpl(m_old, key, m_oldSize);
        }
        if (m_isRehash) rehashStep();
        return temp;
    }

    SizeType getSize() { return m_useSize; }
};