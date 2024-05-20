//
// Created by kirill3266 on 19.05.24.
//

#ifndef DIPLOM_CONCURRENTQUEUE_H
#define DIPLOM_CONCURRENTQUEUE_H


#include <condition_variable>
#include <mutex>
#include <queue>


template<typename T>
class ConcurrentQueue {
private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;

public:
    void push(const T &t_data);

    void pushMany(const std::vector<T> &t_data);

    bool empty();

    void waitAndPop(T &t_value);

    void waitAndPopMany(std::vector<T> &t_value);
};

template<typename T>
void ConcurrentQueue<T>::push(const T &t_data) {
        std::unique_lock lock(m_mutex);
        m_queue.push(t_data);
        lock.unlock();
        m_condition_variable.notify_one();
}

template<typename T>
void ConcurrentQueue<T>::pushMany(const std::vector<T> &t_data) {
        std::unique_lock lock(m_mutex);
        typename std::vector<T>::size_type size = t_data.size();
        for (typename std::vector<T>::size_type i = 0; i < size; ++i) {
                m_queue.push(t_data[i]);
        }
        lock.unlock();
        m_condition_variable.notify_one();
}

template<typename T>
bool ConcurrentQueue<T>::empty() {
        std::unique_lock lock(m_mutex);
        return m_queue.empty();
}

template<typename T>
void ConcurrentQueue<T>::waitAndPop(T &t_value) {
        std::unique_lock lock(m_mutex);
        m_condition_variable.wait(lock, [&] { return !m_queue.empty(); });
        t_value = m_queue.front();
        m_queue.pop();
}

template<typename T>
void ConcurrentQueue<T>::waitAndPopMany(std::vector<T> &t_value) {
        std::unique_lock lock(m_mutex);
        m_condition_variable.wait(lock, [&] { return !m_queue.empty(); });
        typename std::vector<T>::size_type size = t_value.size();
        for (typename std::vector<T>::size_type i = 0; i < size; ++i) {
                t_value[i] = m_queue.front();
                m_queue.pop();
        }
}


#endif //DIPLOM_CONCURRENTQUEUE_H
