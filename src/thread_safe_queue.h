#include <memory>
#include <mutex>
#include <queue>
#include <shared_mutex>

#include "eyeLike.h"

class EmptyQueueException : std::exception {
  public:
    const char *what() const throw() { return "Empty queue"; }
};

class QueueOccupiedException : std::exception {
  public:
    const char *what() const throw() { return "Access was gotten"; }
};

// Don't manipulate queue directly.
// Use ThreadSafeQueuePushViewer/ThreadSafeQueuePopViewer instead.
template <class T> class ThreadSafeQueue {
  public:
    bool have_push_viewer = false;
    bool have_pop_viewer = false;
    void pop(T &res) {
        std::lock_guard<std::mutex> lock(m);
        if (que.empty())
            throw EmptyQueueException();
        res = que.front();
        que.pop();
    }

    void push(T value) {
        std::lock_guard<std::mutex> lock(m);
        que.push(value);
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m);
        return que.empty();
    }

  private:
    std::queue<T> que;
    mutable std::mutex m;
};

template <class T> class ThreadSafeQueuePushViewer {
  public:
    void push(T value) { que.push(value); }
    explicit ThreadSafeQueuePushViewer(ThreadSafeQueue<T> &que_) : que(que_) {
        if (que.have_push_viewer)
            throw QueueOccupiedException();
    }
    ~ThreadSafeQueuePushViewer() { que.have_push_viewer = false; }

  private:
    ThreadSafeQueue<T> &que;
};

template <typename T> class ThreadSafeQueuePopViewer {
  public:
    bool empty() const { return que->empty(); }
    void pop(T &res) { que->pop(res); }
    explicit ThreadSafeQueuePopViewer(ThreadSafeQueue<T> &que_) : que(que_) {
        if (que.have_pop_viewer)
            throw QueueOccupiedException();
    }
    ~ThreadSafeQueuePopViewer() { que->have_pop_viewer = false; }

  private:
    ThreadSafeQueue<T> &que;
};