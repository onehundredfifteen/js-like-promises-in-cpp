#pragma once

#ifndef CONCURRENCY_QUEUE
#define CONCURRENCY_QUEUE

#include <memory>
#include <atomic>

namespace pro {
    namespace concurrency {
        /*
        Note this is not a full lock-free queue, only push is thread-safe here
        */
        template<typename T>
        class queue
        {
        private:
            template<typename T>
            struct node
            {
                std::shared_ptr<T> data;
                node* next;

                node(const T& data) 
                    : data(std::make_shared<T>(data)), next(nullptr)
                {}
                node(T&& data)
                    : data(std::make_shared<T>(std::move(data))), next(nullptr)
                {}
            };

            std::atomic<node<T>*> head;
            std::atomic<node<T>*> tail;
            std::atomic<unsigned int> count;

        public:
            queue() : tail(nullptr), head(nullptr), count(0)
            {}

            void push(T&& data)
            {
                node<T>* new_node = new node<T>(std::move(data));
                new_node->next = tail.load(std::memory_order_relaxed);

                node<T>* old_tail = tail.load(std::memory_order_relaxed);
                do {
                    new_node->next = old_tail;
                }
                while(!tail.compare_exchange_weak(old_tail, new_node,
                                                     std::memory_order_release,
                                                     std::memory_order_relaxed));

                count.fetch_add(1);

                if (head.load() == nullptr)
                {
                    head = tail.load(std::memory_order_relaxed);
                    node<T>* old_tail = tail.load(std::memory_order_relaxed);

                    while (!head.compare_exchange_weak(old_tail, tail.load(),
                        std::memory_order_release,
                        std::memory_order_relaxed));
                }            
            }

            T pop()
            {
                node<T>* const old_head = head.load();
                if (old_head == nullptr) {
                    throw std::exception("queue is empty");
                }

                node<T>* next_head = tail.load();
                if (old_head != next_head) {
                    while (old_head != next_head->next) {
                        next_head = next_head->next;
                    }

                    head.store(next_head);
                }
                else head.store(nullptr);
                    
                std::shared_ptr<T> const res(old_head->data);
                delete old_head;

                count.fetch_sub(1);
                return std::move(*(res.get()));
            }

            unsigned int size() {
                return count.load();
            }

            bool empty() {
                node<T>* const _head = head.load();
                return nullptr == _head;
            }
        };
    }
}  //namespace
#endif