#pragma once
#include "cache-object.h"

class CacheListNode {
  private:
    CacheListNode *m_next;
    CacheListNode *m_prev;
    CacheObject *m_value;

  public:
    void setNext(CacheListNode *next);
    CacheListNode *getNext();

    void setPrev(CacheListNode *prev);
    CacheListNode *getPrev();

    void setValue(CacheObject *value);
    CacheObject *getValue();

    CacheListNode() = default;
    CacheListNode(CacheObject *value) : m_value(value) { ; }

    void destoryValue();
};

class CacheList : public CacheContainer {
  private:
    int m_size;
    CacheListNode *m_head;
    CacheListNode *m_tail;

  protected:
    CacheList(std::string key, CacheType valueType = ListType);
    virtual ~CacheList();

  public:
    virtual void addKeyValue(CacheObject* o);
    virtual CacheObject *getKeyValue(std::string key);
    virtual void delKeyValue(std::string key);

    // addKeyValue会新建节点而addNode不会
    CacheListNode *addNode(CacheListNode *node, CacheListNode *pos = nullptr);
    // getKeyValue会获取CacheObject而getNode直接返回节点
    CacheListNode *getNode(std::string key);
    // delKeyvalue会销毁节点而popNode不会
    CacheListNode *popNode(CacheListNode *node = nullptr);



    CacheListNode *getHead();
    CacheListNode *getTail();
    int getSize();

    friend CacheObject *getInstance(std::string key, CacheType valueType);
    friend void destoryInstance(CacheObject *o);
};