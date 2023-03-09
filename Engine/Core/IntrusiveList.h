#pragma once

namespace intrusive_list {
    struct Node {
        Node* next;
        Node* prev;
    };

    static inline void init(Node* node)
    {
        node->next = node;
        node->prev = node;
    }

    static inline bool is_empty(Node* node)
    {
        return (node->next == node) && (node->prev == node);
    }

    static inline void add(Node* new_node, Node* prev, Node* next)
    {
        next->prev     = new_node;
        new_node->next = next;
        new_node->prev = prev;
        prev->next     = new_node;
    }

    /**
     * Append new_node to be right after target
     * @param new_node The new node to append
     * @param target   The target to append the new node to
     */
    static inline void append(Node* new_node, Node* target)
    {
        add(new_node, target, target->next);
    }

    static inline void unlink(Node* node)
    {
        node->next->prev = node->prev;
        node->prev->next = node->next;
        init(node);
    }

}  // namespace intrusive_list
