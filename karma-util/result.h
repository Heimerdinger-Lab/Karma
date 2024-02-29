#pragma once

namespace util {
template <typename T, typename E>
class result {
   public:
    enum class status { Ok, Err };
    union value {
        T ok;
        E err;
        value(T t) : ok(t) {}
        value(E e) : err(e) {}
        ~value() {}
    };

    result(T t) : m_status(status::Ok), m_value(t) {}
    result(E e) : m_status(status::Err), m_value(e) {}
    ~result() {}

    bool is_ok() const { return m_status == status::Ok; }
    bool is_err() const { return m_status == status::Err; }
    T ok() const { return m_value.ok; }
    E err() const { return m_value.err; }

   private:
    status m_status;
    value m_value;
};
}  // namespace util