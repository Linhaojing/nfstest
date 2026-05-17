#pragma once

#include <variant>
#include <stdexcept>

namespace nfs3 {

template<typename E>
class unexpected {
public:
    explicit unexpected(E e) : error_(std::move(e)) {}
    
    const E& error() const& { return error_; }
    E& error() & { return error_; }
    E&& error() && { return std::move(error_); }
    
private:
    E error_;
};

template<typename E>
unexpected(E) -> unexpected<E>;

template<typename T, typename E>
class expected {
public:
    using value_type = T;
    using error_type = E;
    
    expected(const T& value) : data_(value) {}
    expected(T&& value) : data_(std::move(value)) {}
    expected(unexpected<E> err) : data_(std::move(err)) {}
    
    expected(const expected&) = default;
    expected(expected&&) = default;
    expected& operator=(const expected&) = default;
    expected& operator=(expected&&) = default;
    
    explicit operator bool() const { return has_value(); }
    bool has_value() const { return std::holds_alternative<T>(data_); }
    
    T& value() & {
        if (!has_value()) throw std::runtime_error("bad_expected_access");
        return std::get<T>(data_);
    }
    
    const T& value() const& {
        if (!has_value()) throw std::runtime_error("bad_expected_access");
        return std::get<T>(data_);
    }
    
    T&& value() && {
        if (!has_value()) throw std::runtime_error("bad_expected_access");
        return std::get<T>(std::move(data_));
    }
    
    E& error() & {
        if (has_value()) throw std::runtime_error("bad_expected_access");
        return std::get<unexpected<E>>(data_).error();
    }
    
    const E& error() const& {
        if (has_value()) throw std::runtime_error("bad_expected_access");
        return std::get<unexpected<E>>(data_).error();
    }
    
    E&& error() && {
        if (has_value()) throw std::runtime_error("bad_expected_access");
        return std::get<unexpected<E>>(std::move(data_)).error();
    }
    
    T& operator*() & { return value(); }
    const T& operator*() const& { return value(); }
    T&& operator*() && { return std::move(value()); }
    
    T* operator->() { return &value(); }
    const T* operator->() const { return &value(); }
    
private:
    std::variant<T, unexpected<E>> data_;
};

template<typename E>
class expected<void, E> {
public:
    using value_type = void;
    using error_type = E;
    
    expected() : has_value_(true) {}
    expected(unexpected<E> err) : has_value_(false), error_(std::move(err)) {}
    
    expected(const expected&) = default;
    expected(expected&&) = default;
    expected& operator=(const expected&) = default;
    expected& operator=(expected&&) = default;
    
    explicit operator bool() const { return has_value_; }
    bool has_value() const { return has_value_; }
    
    E& error() & {
        if (has_value_) throw std::runtime_error("bad_expected_access");
        return error_.error();
    }
    
    const E& error() const& {
        if (has_value_) throw std::runtime_error("bad_expected_access");
        return error_.error();
    }
    
    E&& error() && {
        if (has_value_) throw std::runtime_error("bad_expected_access");
        return std::move(error_).error();
    }
    
private:
    bool has_value_;
    unexpected<E> error_{E{}};
};

}
