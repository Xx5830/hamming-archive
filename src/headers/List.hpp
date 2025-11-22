#include "error.hpp";

#ifndef INCLUDE_LIST
#define INCLUDE_LIST
template <typename T>
//concept copyable = requires(T item1, T item2) { item1 = item2; };

namespace contain{
template <typename T>
    requires copyable<T>
struct List {
    struct Node {
        Node *next;
        Node *prev;
        T data;
    };

    operator =(const List &other) = delete;
    operator =() = delete;
    List(const List &other) = delete;

    Node *begin() { return head; };
    Node *end() { return nullptr; };

    void PushBack(const T &element) {
        Node* node = new Node{nullptr, nullptr, data};

        if (!head){
            head = tail = node;
        }
        else{
            node.prev = tail;
            tail = tail->next = node;
        }
    }

    error::Error PopBack(){
        return Erase(0);
    }

    //improve
    error::Error Erase(size_t index){
        Node *node = head;

        for (size_t current_index = 0; current_index < index; current_index++){
            if (node == nullptr){
                return {"List can't delete element"};
            }
            node = node->next;
        }

        if (node == head){
            head = node->next;
            node->next->prev = nullptr;
            delete node;
        }
        else if (node == tail){
            tail->prev->next = nullptr;
            delete node;
        }
        else{
            node->prev->next = node->next;
            node->next->prev = node->prev;
            delete node;
        }

        return {nullptr};
    }

    //improve
    error::Error Insert(size_t index, const T &value){
        Node *node = head;

        for (size_t current_index = 0; current_index < index; current_index++){
            if (node == nullptr){
                return {"List can't insert element"};
            }
            node = node->next;
        }

        Node *new_node = new Node{nullptr, nullptr, value};

        if (node != tail){
            node->next->prev = new_node;
            new_node->next = node->next;
        }
        if (node != head){
            new_node->prev = node;
        }
        if (node != tail){
            tail->prev->next = nullptr;
            delete node;
        }
        

        return {nullptr};
    }

    ~List(){
        Node *current_node = head;

        while (current_node){
            current_node = current_node->next;
            delete head;
            head = current_node;
        }
    }

  private:
    Node *head = nullptr;
    Node *tail = nullptr;
};
} // contain


#endif