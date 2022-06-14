#ifndef SMEN_EVENT_HPP
#define SMEN_EVENT_HPP

#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <optional>

namespace smen {
    using EventTag = size_t;
    using EventClearFunction = std::function<void (EventTag tag)>;

    template <typename SenderType, typename... Args>
    class Event {
    public:
        using ListenerType = std::function<void (SenderType &, Args...)>;
    private:
        std::vector<std::optional<ListenerType>> _callbacks;

    public:
        Event();
        EventTag add_listener(const ListenerType & listener);
        void remove_listener(EventTag tag);

        void operator ()(SenderType & type, Args... args) const;

        Event & operator +=(const ListenerType & listener);
        Event & operator -=(const ListenerType & listener);
    };

    struct EventHolder {
    private:
        std::vector<std::pair<EventTag, EventClearFunction>> _clearers;

    public:
        template <typename SenderType, typename... Args>
        void add(Event<SenderType, Args...> & ev, typename Event<SenderType, Args...>::ListenerType listener);

        ~EventHolder();
    };


    /// TEMPLATE DEFINITIONS ///
    template <typename SenderType, typename... Args>
    Event<SenderType, Args...>::Event()
    {}

    template <typename SenderType, typename... Args>
    EventTag Event<SenderType, Args...>::add_listener(const ListenerType & listener) {
        auto it = std::find(_callbacks.begin(), _callbacks.end(), std::nullopt);
        if (it == _callbacks.end()) {
            _callbacks.emplace_back(listener);
        } else {
            *it = listener;
        }
        return _callbacks.size() - 1;
    }

    template <typename SenderType, typename... Args>
    void Event<SenderType, Args...>::remove_listener(EventTag tag) {
        _callbacks[tag] = std::nullopt;
    }

    template <typename SenderType, typename... Args>
    void Event<SenderType, Args...>::operator ()(SenderType & type, Args... args) const {
        for (auto & callback : _callbacks) {
            if (callback) {
                (*callback)(type, args...);
            }
        }
    }

    template <typename SenderType, typename... Args>
    Event<SenderType, Args...> & Event<SenderType, Args...>::operator +=(const ListenerType & listener) {
        add_listener(listener);
        return *this;
    }

    template <typename SenderType, typename... Args>
    Event<SenderType, Args...> & Event<SenderType, Args...>::operator -=(const ListenerType & listener) {
        add_listener(listener);
        return *this;
    }

    template <typename SenderType, typename... Args>
    void EventHolder::add(Event<SenderType, Args...> & ev, typename Event<SenderType, Args...>::ListenerType listener) {
        auto tag = ev.add_listener(listener);
        auto clearer = [&](auto tag) { ev.remove_listener(tag); };
        _clearers.emplace_back(tag, clearer);
    }

    inline EventHolder::~EventHolder() {
        for (auto & clearer : _clearers) {
            clearer.second(clearer.first);
        }
    }
}

#endif//SMEN_EVENT_HPP
