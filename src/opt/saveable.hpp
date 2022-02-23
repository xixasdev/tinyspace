// saveable.hpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.12 | LICENSE: WTFPL/PDM/CC0... your choice


#ifndef _TINYSPACE_SAVEABLE_HPP_
#define _TINYSPACE_SAVEABLE_HPP_


#include <atomic>
#include <ostream>
#include <memory>
#include <thread>
#include "vector2.hpp"


namespace tinyspace {


// ---------------------------------------------------------------------------
// UPDATEABLE OBJECT REGISTRATION
// ---------------------------------------------------------------------------


struct Updateable {
    Updateable();
    virtual ~Updateable();
    virtual void update() = 0;
};
void update_aftersave();


// ---------------------------------------------------------------------------
// GENERIC SNAPSHOT TEMPLATE CLASS AND TYPE ERASURE
// ---------------------------------------------------------------------------


extern std::atomic<bool> isSaving;
extern std::atomic<std::thread*> saveThread;


bool isSavingThread();


template <typename T>
class Saveable : public Updateable
{
public:
    Saveable();
    Saveable( T const& t );
    Saveable( T&& t );
    Saveable( Saveable<T> const& o );

    ~Saveable();

    void set( T const& t );
    void set( T&& t );
    void update() override;

    T const& live();
    T const& snap();

    Saveable<T>& operator =( T const& t );
    Saveable<T>& operator =( T&& t );
    Saveable<T>& operator =( Saveable<T> const& o );

    Saveable<T>& operator *=( T const& t );
    Saveable<T>& operator /=( T const& t );
    Saveable<T>& operator +=( T const& t );
    Saveable<T>& operator -=( T const& t );

    operator T const&() const;
    T const& operator ()() const;
    T const* operator ->() const;

private:
    T *_live, *_snap;
};
template <typename T> std::ostream& operator <<( std::ostream& os, Saveable<T> const& o );


// Raw pointer specialization
template <typename T>
class Saveable<T*> : public Updateable
{
public:
    Saveable();
    Saveable( T* const& t );
    Saveable( Saveable<T*> const& o );

    ~Saveable();

    void set( T* const& t );
    void update() override;

    T* const& live();
    T* const& snap();

    Saveable<T*>& operator =( T* const& t );
    Saveable<T*>& operator =( Saveable<T*> const& o );

    operator T* const&() const;
    T* const& operator ()() const;
    T* const operator ->() const;

private:
    T *_live, *_snap;
};


// Shared pointer specialization
template <typename T>
class Saveable<std::shared_ptr<T>> : public Updateable
{
public:
    Saveable();
    Saveable( std::shared_ptr<T> const& t );
    Saveable( Saveable<std::shared_ptr<T>> const& o );

    ~Saveable();

    void set( std::shared_ptr<T> const& t );
    void update() override;

    std::shared_ptr<T> const& live();
    std::shared_ptr<T> const& snap();

    Saveable<std::shared_ptr<T>>& operator =( std::shared_ptr<T> const& t );
    Saveable<std::shared_ptr<T>>& operator =( Saveable<std::shared_ptr<T>> const& o );

    operator bool() const;
    operator std::shared_ptr<T> const&() const;
    std::shared_ptr<T> const& operator ()() const;
    T const* operator ->() const;

private:
    std::shared_ptr<T> _live, _snap;
};


// Specialized Vector2 wrapper
template <typename T, typename U=float>
struct SaveableVector2 : public Vector2<Saveable<T>, U>
{
    SaveableVector2();
    SaveableVector2( T const& x, T const& y );
    SaveableVector2( Vector2<T,U> const& o );
    SaveableVector2( Vector2<Saveable<T>,U> const& o );

    ~SaveableVector2();

    operator Vector2<T,U>() const;
    Vector2<T,U> operator ()() const;
};

template <typename T, typename U>
Vector2<T,U> operator +( Vector2<Saveable<T>,U> const& lhs, Vector2<Saveable<T>,U> const& rhs );
template <typename T, typename U>
Vector2<T,U> operator +( Vector2<Saveable<T>,U> const& lhs, Vector2<T,U> const& rhs );
template <typename T, typename U>
Vector2<T,U> operator +( Vector2<T,U> const& lhs, Vector2<Saveable<T>,U> const& rhs );

template <typename T, typename U>
Vector2<T,U> operator -( Vector2<Saveable<T>,U> const& lhs, Vector2<Saveable<T>,U> const& rhs );
template <typename T, typename U>
Vector2<T,U> operator -( Vector2<Saveable<T>,U> const& lhs, Vector2<T,U> const& rhs );
template <typename T, typename U>
Vector2<T,U> operator -( Vector2<T,U> const& lhs, Vector2<Saveable<T>,U> const& rhs );

template <typename T, typename U, typename V>
Vector2<T,U> operator *( Vector2<Saveable<T>,U> const& lhs, Saveable<V> const& rhs );
template <typename T, typename U, typename V>
Vector2<T,U> operator *( Vector2<Saveable<T>,U> const& lhs, V const& rhs );
template <typename T, typename U, typename V>
Vector2<T,U> operator *( Vector2<T,U> const& lhs, Saveable<V> const& rhs );

template <typename T, typename U, typename V>
Vector2<T,U> operator /( Vector2<Saveable<T>,U> const& lhs, Saveable<V> const& rhs );
template <typename T, typename U, typename V>
Vector2<T,U> operator /( Vector2<Saveable<T>,U> const& lhs, V const& rhs );
template <typename T, typename U, typename V>
Vector2<T,U> operator /( Vector2<T,U> const& lhs, Saveable<V> const& rhs );

template <typename T, typename U, typename V>
Vector2<T,U> operator +( Vector2<Saveable<T>,U> const& lhs, Saveable<V> const& rhs );
template <typename T, typename U, typename V>
Vector2<T,U> operator +( Vector2<Saveable<T>,U> const& lhs, V const& rhs );
template <typename T, typename U, typename V>
Vector2<T,U> operator +( Vector2<T,U> const& lhs, Saveable<V> const& rhs );

template <typename T, typename U, typename V>
Vector2<T,U> operator -( Vector2<Saveable<T>,U> const& lhs, Saveable<V> const& rhs );
template <typename T, typename U, typename V>
Vector2<T,U> operator -( Vector2<Saveable<T>,U> const& lhs, V const& rhs );
template <typename T, typename U, typename V>
Vector2<T,U> operator -( Vector2<T,U> const& lhs, Saveable<V> const& rhs );


} // tinyspace


#include "saveable.tpp"


#endif // _TINYSPACE_SAVEABLE_HPP_
