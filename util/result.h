template<typename T, typename E>
class Result {
public:
    enum class Status { Ok, Err };
    union Value {
        T ok;
        E err;
        Value(T t) : ok(t) {}
        Value(E e) : err(e) {}
        ~Value() {}
    };
    
    Result(T t) : status(Status::Ok), value(t) {}
    Result(E e) : status(Status::Err), value(e) {}
    ~Result() {}
    
    bool is_ok() const { return status == Status::Ok; }
    bool is_err() const { return status == Status::Err; }
    T ok() const { return value.ok; }
    E err() const { return value.err; }
    
private:
    Status status;
    Value value;
};