#include <iostream>
#include <stdexcept>

template <typename T>
struct node {
    T data;
    node* next = nullptr;
    node* last = nullptr;

    node() = default;
    node(const T& d) : data(d) {}
};

// Insert nd before hd
template <typename T>
void hInsert(node<T>* hd, node<T>* nd) {
    if (!hd || !nd) return;
    nd->next = hd;
    nd->last = hd->last;
    if (hd->last) hd->last->next = nd;
    hd->last = nd;
}

// Insert nd after hd
template <typename T>
void tInsert(node<T>* hd, node<T>* nd) {
    if (!hd || !nd) return;
    nd->last = hd;
    nd->next = hd->next;
    if (hd->next) hd->next->last = nd;
    hd->next = nd;
}

// Delete node on target index
template <typename T>
void del(node<T>*& hd, int index) {
    if (!hd || index < 0) return;
    node<T>* cur = hd;

    // Find target node
    for (int i = 0; i < index && cur; i++) {
        cur = cur->next;
    }
    if (!cur) return; // Out of range

    // Change next and last pointers
    if (cur->last) cur->last->next = cur->next;
    if (cur->next) cur->next->last = cur->last;

    // If deleted head pointer, update
    if (cur == hd) hd = cur->next;

    delete cur;
}

// Insert node at target index
template <typename T>
void ins(node<T>* hd, node<T>* insert, int index) {
    if (!hd || !insert || index < 0) return;
    node<T>* cur = hd;

    for (int i = 0; i < index && cur; i++) {
        cur = cur->next;
    }
    if (!cur) return;

    insert->next = cur;
    insert->last = cur->last;
    if (cur->last) cur->last->next = insert;
    cur->last = insert;
}

// Get data in node on target index
template <typename T>
T get(node<T>* hd, int index) {
    if (!hd || index < 0) throw std::out_of_range("Index out of range");
    node<T>* cur = hd;
    for (int i = 0; i < index && cur; i++) {
        cur = cur->next;
    }
    if (!cur) throw std::out_of_range("Index out of range");
    return cur->data;
}

// Get left node's data
template <typename T>
node<T>* getLeft(node<T>* nd) {
    return nd ? nd->last->data : T();
}

// Get right node's data
template <typename T>
node<T>* getRight(node<T>* nd) {
    return nd ? nd->next : T();
}
