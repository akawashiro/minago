#pragma once

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

class StateOccupiedException : std::exception {
  public:
    const char *what() const throw() { return "Access was gotten"; }
};

template <class T> class ThreadSafeQueue {
  public:
    class ThreadSafeQueuePushViewer {
      public:
        void push(T value) { que->push(value); }
        explicit ThreadSafeQueuePushViewer(ThreadSafeQueue<T> *que_)
            : que(que_) {}
        ~ThreadSafeQueuePushViewer() { que->have_push_viewer = false; }

      private:
        ThreadSafeQueue<T> *que;
    };

    class ThreadSafeQueuePopViewer {
      public:
        bool empty() const { return que->empty(); }
        void pop(T &res) { que->pop(res); }
        std::shared_ptr<T> pop() { return std::move(que->pop()); }
        explicit ThreadSafeQueuePopViewer(ThreadSafeQueue<T> *que_)
            : que(que_) {}
        ~ThreadSafeQueuePopViewer() { que->have_pop_viewer = false; }

      private:
        ThreadSafeQueue<T> *que;
    };

    ThreadSafeQueuePopViewer getPopView() {
        std::lock_guard<std::mutex> lock(m);
        if (have_pop_viewer)
            throw QueueOccupiedException();
        have_pop_viewer = true;
        return ThreadSafeQueuePopViewer(this);
    }

    ThreadSafeQueuePushViewer getPushView() {
        std::lock_guard<std::mutex> lock(m);
        if (have_push_viewer)
            throw QueueOccupiedException();
        have_push_viewer = true;
        return ThreadSafeQueuePushViewer(this);
    }

  private:
    std::queue<T> que;
    mutable std::mutex m;
    bool have_push_viewer = false;
    bool have_pop_viewer = false;

    void pop(T &res) {
        std::lock_guard<std::mutex> lock(m);
        if (que.empty())
            throw EmptyQueueException();
        res = que.front();
        que.pop();
    }

    std::shared_ptr<T> pop() {
        std::lock_guard<std::mutex> lock(m);
        if (que.empty())
            throw EmptyQueueException();
        auto const res = std::make_shared<T>(que.front());
        que.pop();
        return res;
    }

    void push(T value) {
        std::lock_guard<std::mutex> lock(m);
        que.push(value);
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m);
        return que.empty();
    }
};

template <class T> class ThreadSafeState {
  public:
    class ThreadSafeStatePutViewer {
      public:
        void put(T v) {
            std::lock_guard<std::mutex> lock(state->m);
            state->value = v;
        }
        explicit ThreadSafeStatePutViewer(ThreadSafeState<T> *state_)
            : state(state_) {}
        ~ThreadSafeStatePutViewer() { state->have_put_viewer = false; }

      private:
        ThreadSafeState<T> *state;
    };

    class ThreadSafeStateGetViewer {
      public:
        T get() {
            std::lock_guard<std::mutex> lock(state->m);
            T v = state->value;
            return v;
        }
        explicit ThreadSafeStateGetViewer(ThreadSafeState<T> *state_)
            : state(state_) {}
        ~ThreadSafeStateGetViewer() { state->have_get_viewer = false; }

      private:
        ThreadSafeState<T> *state;
    };

    ThreadSafeStateGetViewer getGetView() {
        std::lock_guard<std::mutex> lock(m);
        if (have_get_viewer)
            throw StateOccupiedException();
        have_get_viewer = true;
        return ThreadSafeStateGetViewer(this);
    }

    ThreadSafeStatePutViewer getPutView() {
        std::lock_guard<std::mutex> lock(m);
        if (have_put_viewer)
            throw StateOccupiedException();
        have_put_viewer = true;
        return ThreadSafeStatePutViewer(this);
    }

  private:
    T value;
    mutable std::mutex m;
    bool have_put_viewer = false;
    bool have_get_viewer = false;
};
