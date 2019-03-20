#include "cache-base.h"
#include "cache-dict.h"
#include "cache-list.h"
#include "cache-tool.h"


CacheType CacheBase::getType(){
    return m_type;
}


CacheValue::~CacheValue() {
    switch (getType()) {
    case StringType:
        delete (std::string*)m_value;
        break;
    default:
        break;    
    }
}

std::string CacheValue::getValue() {
    if (getType() == LongType) {
        return std::to_string((int64)(m_value));
    }
    if (!m_value)return "";
    return *(std::string*)m_value;
}

void CacheValue::setValue(std::string value) {
    if (getType() == LongType) {
        int64 temp = std::stoll(value);
        m_value = (void*)temp;
        return;
    }
    if (!m_value) m_value = new std::string();
    *(std::string*)m_value = value;
}

// 作为通用的容器模板，CacheDict和CacheList在析构时
// 不会同时析构其中管理的堆上对象，其中子对象需要手动回收
void delInstance(CacheBase* base) {
    using CacheList = CacheList<CacheValue*>;
    using NodeType = CacheList::NodeType;
    using CacheDict = CacheDict<std::string,
        CacheValue*>;
    using PairType = CacheDict::PairType;

    if (base->getType() == ListType) {
        auto list = dynamic_cast<CacheList*>(base);
        std::function<void(const NodeType*)> func =
            [](const NodeType* x) {
            delInstance(x->getValue());
        };
        list->walk(func);
        delete base; return;
    }

    if (base->getType() == DictType) {
        auto dict = dynamic_cast<CacheDict*>(base);
        std::function<void(const PairType&)> func=
            [](const PairType& x) {
            delInstance(x.m_two);
        };
        dict->walk(func);
        delete base; return;
    }
    delete base;
}