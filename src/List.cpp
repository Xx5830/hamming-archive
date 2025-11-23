#include "List.hpp"
#include <cstdlib>

template <typename T> inline T &contain::List<T>::Node::GetData() {
    return data;
}

template <typename T> bool contain::List<T>::Node::HasNext() { return next != nullptr; }

template <typename T> void contain::List<T>::Node::Next() {this = next;}

template <typename T> void contain::List<T>::Node::Prev() {this = prev;}

template <typename T> contain::List<T>::Node *contain::List<T>::Head() { return head; }

template <typename T> contain::List<T>::Node *contain::List<T>::Tail() { return tail; }

template <typename T> void contain::List<T>::PushBack(const T &element) {
    Node *node = new Node{nullptr, nullptr, data};

    if (!head) {
        head = tail = node;
    } else {
        node.prev = tail;
        tail = tail->next = node;
    }
}
template <typename T> void contain::List<T>::PushFront(const T &element) {Insert(0, begin);}
template <typename T> error::Error contain::List<T>::PopBack() { return Erase(0); }
template <typename T> error::Error contain::List<T>::PopFront() { return Erase(0); }

template <typename T> error::Error contain::List<T>::Erase(size_t index) {
    Node *node = head;

    for (size_t current_index = 0; current_index < index; current_index++) {
        if (node == nullptr) {
            return {"List can't delete element"};
        }
        node = node->next;
    }

    if (node == head) {
        head = node->next;
        node->next->prev = nullptr;
        delete node;
    } else if (node == tail) {
        tail->prev->next = nullptr;
        delete node;
    } else {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        delete node;
    }

    return {nullptr};
}

template <typename T> error::Error contain::List<T>::Insert(size_t index, const T &value) {
    Node *node = head;

    for (size_t current_index = 0; current_index < index; current_index++) {
        if (node == nullptr) {
            return {"List can't insert element"};
        }
        node = node->next;
    }

    Node *new_node = new Node{nullptr, nullptr, value};

    if (node != tail) {
        node->next->prev = new_node;
        new_node->next = node->next;
    }
    if (node != head) {
        new_node->prev = node;
    }
    if (node != tail) {
        tail->prev->next = nullptr;
        delete node;
    }

    return {nullptr};
}

template <typename T> contain::List<T>::~List() {
    Node *current_node = head;

    while (current_node) {
        current_node = current_node->next;
        delete head;
        head = current_node;
    }
}
