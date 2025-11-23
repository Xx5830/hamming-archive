#include "error.hpp";
#include <cstdlib>

#ifndef INCLUDE_LIST
#define INCLUDE_LIST

template <typename T>
concept copyable = requires(T item1, T item2) { item1 = item2; };

namespace contain{
template <typename T>
    requires copyable<T>
struct List {
    class Node {
        Node *next;
        Node *prev;
        T data;

        public:
        T& GetData();
        bool HasNext();
        void Next();
        void Prev();
    };

    List& operator =(const List &other) = delete;
    List(const List &other) = delete;

    Node *Head();
    Node *Tail();

    void PushBack(const T &element);
    void PushFront(const T &element);
    error::Error PopBack();
    error::Error PopFront();
    //improve
    error::Error Erase(size_t index);
    //improve
    error::Error Insert(size_t index, const T &value);
    ~List();

  protected:
    Node *head = nullptr;
    Node *tail = nullptr;

    
};
} // contain


#endif