#ifndef SMEN_FUNCTIONS_HPP
#define SMEN_FUNCTIONS_HPP

#include <functional>
#include <unordered_map>
#include <string>
#include <optional>
#include <memory>

namespace smen {
    template <typename T>
    struct ObjectDatabaseID;

    template <typename T>
    std::ostream & operator <<(std::ostream & s, const ObjectDatabaseID<T> & db);

    class ObjectDatabaseException : public std::runtime_error {
    public:
        inline ObjectDatabaseException(const std::string & msg) : std::runtime_error(msg) {}
    };

    template <typename T>
    struct ObjectDatabaseEntry;

    template <typename T>
    class ObjectDatabase;

    template <typename T>
    struct ObjectDatabaseID {
        friend class ObjectDatabase<T>;

    private:
        std::string _id;

    public:
        ObjectDatabaseID();
        ObjectDatabaseID(const std::string & id);

        operator const std::string &() const;
        explicit operator bool() const;
        friend std::ostream & operator <<<>(std::ostream & s, const ObjectDatabaseID<T> & db);
    };

    template <typename T>
    class ObjectDatabase {
        friend struct ObjectDatabaseEntry<T>;

    public:
        using EntryType = ObjectDatabaseEntry<T>;
        using ConstIndexType = const T &;
        using IndexType = T &;

    private:
        std::unordered_map<std::string, T> _objects;

        void _dereg(const std::string & id);

    public:
        EntryType reg(const std::string & id, const T & obj);
        bool has(const std::string & id);
        ConstIndexType operator [](const ObjectDatabaseID<T> & id) const;
        IndexType operator [](const ObjectDatabaseID<T> & id);
    };

    template <typename T>
    struct ObjectDatabaseEntry {
    public:
        using DatabaseType = ObjectDatabase<T>;

    private:
        DatabaseType * _db;
        std::string _id;

    public:
        ObjectDatabaseEntry();
        ObjectDatabaseEntry(const ObjectDatabaseEntry<T> &) = delete;
        ObjectDatabaseEntry(ObjectDatabaseEntry<T> &&);
        ObjectDatabaseEntry & operator=(const ObjectDatabaseEntry<T> &) = delete;
        ObjectDatabaseEntry & operator=(ObjectDatabaseEntry<T> &&);

        ObjectDatabaseEntry(DatabaseType & db, const std::string & id);

        ObjectDatabaseID<T> id() const;
        void clear();

        ~ObjectDatabaseEntry();
    };

    /// TEMPLATE DEFINITIONS ///

    template <typename T>
    ObjectDatabaseID<T>::ObjectDatabaseID()
    : _id("") {}

    template <typename T>
    ObjectDatabaseID<T>::ObjectDatabaseID(const std::string & id)
    : _id(id) {
        if (id.size() == 0) throw std::runtime_error("cannot specify empty ID");
    }

    template <typename T>
    ObjectDatabaseID<T>::operator const std::string &() const {
        return _id;
    }

    template <typename T>
    ObjectDatabaseID<T>::operator bool() const {
        return _id.size() > 0;
    }

    template <typename T>
    std::ostream & operator <<(std::ostream & s, const ObjectDatabaseID<T> & db) {
        s << db._id;
        return s;
    }

    template <typename T>
    typename ObjectDatabase<T>::EntryType ObjectDatabase<T>::reg(const std::string & id, const T & obj) {
        _objects[id] = obj;
        return ObjectDatabaseEntry<T>(*this, id);
    }

    template <typename T>
    bool ObjectDatabase<T>::has(const std::string & id) {
        auto it = _objects.find(id);
        if (it == _objects.end()) return false;
        return true;
    }

    template <typename T>
    typename ObjectDatabase<T>::ConstIndexType ObjectDatabase<T>::operator [](const ObjectDatabaseID<T> & id) const {
        auto it = _objects.find(id);
        if (it == _objects.end()) throw ObjectDatabaseException("failed resolving object with ID '" + id._id + "'");
        return it->second;
    }

    template <typename T>
    typename ObjectDatabase<T>::IndexType ObjectDatabase<T>::operator [](const ObjectDatabaseID<T> & id) {
        auto it = _objects.find(id);
        if (it == _objects.end()) throw ObjectDatabaseException("failed resolving object with ID '" + id._id + "'");
        return it->second;
    }

    template <typename T>
    void ObjectDatabase<T>::_dereg(const std::string & id) {
        _objects.erase(id);
    }

    template <typename T>
    ObjectDatabaseEntry<T>::ObjectDatabaseEntry()
    : _db(nullptr)
    , _id()
    {}

    template <typename T>
    ObjectDatabaseEntry<T>::ObjectDatabaseEntry(ObjectDatabaseEntry<T> && other)
    : _db(other._db)
    , _id(other._id)
    {
        other._db = nullptr;
    }

    template <typename T>
    ObjectDatabaseEntry<T> & ObjectDatabaseEntry<T>::operator=(ObjectDatabaseEntry<T> && other) {
        _db = other._db;
        _id = other._id;
        other._db = nullptr;
        return *this;
    }

    template <typename T>
    ObjectDatabaseEntry<T>::ObjectDatabaseEntry(DatabaseType & db, const std::string & id)
    : _db(&db)
    , _id(id)
    {}

    template <typename T>
    ObjectDatabaseID<T> ObjectDatabaseEntry<T>::id() const {
        if (_db == nullptr) {
            throw std::runtime_error("cannot get ID of empty ObjectDatabaseEntry");
        }
        return _id;
    }

    template <typename T>
    void ObjectDatabaseEntry<T>::clear() {
        if (_db != nullptr) {
            _db->_dereg(_id);
            _db = nullptr;
        }
    }

    template <typename T>
    ObjectDatabaseEntry<T>::~ObjectDatabaseEntry() {
        clear();
    }
}

#endif//SMEN_FUNCTIONS_HPP
