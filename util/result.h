template<typename T, typename E>
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
    
    result(T t) : status(status::Ok), value(t) {}
    result(E e) : status(status::Err), value(e) {}
    ~result() {}
    
    bool is_ok() const { return status == status::ok; }
    bool is_err() const { return status == status::err; }
    T ok() const { return value.ok; }
    E err() const { return value.err; }
    
private:
    status m_status;
    value m_value;
};