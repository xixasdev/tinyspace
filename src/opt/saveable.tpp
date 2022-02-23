// saveable.tpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.12 | LICENSE: WTFPL/PDM/CC0... your choice


#ifndef _TINYSPACE_SAVEABLE_TPP_
#define _TINYSPACE_SAVEABLE_TPP_


#include <unordered_set>


namespace tinyspace {


template <typename T>
Saveable<T>::Saveable()
    : Updateable()
{
    _snap = _live = new T;
}


template <typename T>
Saveable<T>::Saveable( T const& t )
    : Updateable()
{
    _snap = _live = new T( t );
}


template <typename T>
Saveable<T>::Saveable( T&& t )
    : Updateable()
{
    _snap = _live = new T( t );
}


template <typename T>
Saveable<T>::Saveable( Saveable<T> const& o )
    : Updateable()
{
    _snap = _live = new T( *o._live );
}


template <typename T>
Saveable<T>::~Saveable()
{
    if (_snap != _live)
    {
        delete _snap;
    }
    delete _live;
}


template <typename T>
void Saveable<T>::set( T const& t )
{
    if (isSaving && _live == _snap)
    {
        _live = new T(t);
    }
    else
    {
        *_live = t;
    }
}


template <typename T>
void Saveable<T>::set( T&& t )
{
    if (isSaving && _live == _snap)
    {
        _live = new T(t);
    }
    else
    {
        *_live = t;
    }
}


template <typename T>
void Saveable<T>::update()
{
    if (_snap != _live)
    {
        delete _snap;
        _snap = _live;
    }
}


template <typename T>
T const& Saveable<T>::live()
{
    return *_live;
}


template <typename T>
T const& Saveable<T>::snap()
{
    return *_snap;
}


template <typename T>
Saveable<T>& Saveable<T>::operator =( T const& t )
{
    set( t );
    return *this;
}


template <typename T>
Saveable<T>& Saveable<T>::operator =( T&& t )
{
    set( t );
    return *this;
}


template <typename T>
Saveable<T>& Saveable<T>::operator =( Saveable<T> const& o )
{
    set( *o._live );
    return *this;
}


template <typename T>
Saveable<T>& Saveable<T>::operator *=( T const& t )
{
    set( *_live * t );
    return *this;
}


template <typename T>
Saveable<T>& Saveable<T>::operator /=( T const& t )
{
    set( *_live / t );
    return *this;
}


template <typename T>
Saveable<T>& Saveable<T>::operator +=( T const& t )
{
    set( *_live + t );
    return *this;
}


template <typename T>
Saveable<T>& Saveable<T>::operator -=( T const& t )
{
    set( *_live - t );
    return *this;
}


template <typename T>
Saveable<T>::operator T const&() const
{
    return isSavingThread() ? *_snap : *_live;
}


template <typename T>
T const& Saveable<T>::operator ()() const
{
    return isSavingThread() ? *_snap : *_live;
}


template <typename T>
T const* Saveable<T>::operator ->() const
{
    return isSavingThread() ? _snap : _live;
}


template <typename T>
std::ostream& operator <<( std::ostream& os, Saveable<T> const& o )
{
    os << o();
    return os;
}


//----------------------------------------------------------------------------
// RAW POINTER SPECIALIZATION
//----------------------------------------------------------------------------


template <typename T>
Saveable<T*>::Saveable()
    : Updateable()
{
    _snap = _live = nullptr;
}


template <typename T>
Saveable<T*>::Saveable( T* const& t )
    : Updateable()
{
    _snap = _live = t;
}


template <typename T>
Saveable<T*>::Saveable( Saveable<T*> const& o )
    : Updateable()
{
    _snap = _live = o._live;
}


template <typename T>
Saveable<T*>::~Saveable()
{}


template <typename T>
void Saveable<T*>::set( T* const& t )
{
    if (!isSaving)
    {
        _snap = t;
    }
    _live = t;
}


template <typename T>
void Saveable<T*>::update()
{
    _snap = _live;
}


template <typename T>
T* const& Saveable<T*>::live()
{
    return _live;
}


template <typename T>
T* const& Saveable<T*>::snap()
{
    return _snap;
}


template <typename T>
Saveable<T*>& Saveable<T*>::operator =( T* const& t )
{
    set( t );
    return *this;
}


template <typename T>
Saveable<T*>& Saveable<T*>::operator =( Saveable<T*> const& o )
{
    set( o._live );
    return *this;
}


template <typename T>
Saveable<T*>::operator T* const&() const
{
    return isSavingThread() ? _snap : _live;
}


template <typename T>
T* const& Saveable<T*>::operator ()() const
{
    return isSavingThread() ? _snap : _live;
}


template <typename T>
T* const Saveable<T*>::operator ->() const
{
    return isSavingThread() ? _snap : _live;
}


// ---------------------------------------------------------------------------
// SHARED POINTER SPECIALIZATION
// ---------------------------------------------------------------------------


template <typename T>
Saveable<std::shared_ptr<T>>::Saveable()
    : Updateable()
{
    _snap = _live = nullptr;
}


template <typename T>
Saveable<std::shared_ptr<T>>::Saveable( std::shared_ptr<T> const& t )
    : Updateable()
{
    _snap = _live = t;
}


template <typename T>
Saveable<std::shared_ptr<T>>::Saveable( Saveable<std::shared_ptr<T>> const& o )
    : Updateable()
{
    _snap = _live = o._live;
}


template <typename T>
Saveable<std::shared_ptr<T>>::~Saveable()
{}


template <typename T>
void Saveable<std::shared_ptr<T>>::set( std::shared_ptr<T> const& t )
{
    if (!isSaving)
    {
        _snap = t;
    }
    _live = t;
}


template <typename T>
void Saveable<std::shared_ptr<T>>::update()
{
    _snap = _live;
}


template <typename T>
std::shared_ptr<T> const& Saveable<std::shared_ptr<T>>::live()
{
    return _live;
}


template <typename T>
std::shared_ptr<T> const& Saveable<std::shared_ptr<T>>::snap()
{
    return _snap;
}


template <typename T>
Saveable<std::shared_ptr<T>>& Saveable<std::shared_ptr<T>>::operator =( std::shared_ptr<T> const& t )
{
    set( t );
    return *this;
}


template <typename T>
Saveable<std::shared_ptr<T>>& Saveable<std::shared_ptr<T>>::operator =( Saveable<std::shared_ptr<T>> const& o )
{
    set( o._live );
    return *this;
}


template <typename T>
Saveable<std::shared_ptr<T>>::operator bool() const
{
    return isSavingThread() ? _snap.get() : _live.get();
}


template <typename T>
Saveable<std::shared_ptr<T>>::operator std::shared_ptr<T> const&() const
{
    return isSavingThread() ? _snap : _live;
}


template <typename T>
std::shared_ptr<T> const& Saveable<std::shared_ptr<T>>::operator ()() const
{
    return isSavingThread() ? _snap : _live;
}


template <typename T>
T const* Saveable<std::shared_ptr<T>>::operator ->() const
{
    return isSavingThread() ? _snap.get() : _live.get();
}


// ---------------------------------------------------------------------------
// SPECIALIZED VECTOR2 WRAPPER
// ---------------------------------------------------------------------------


template <typename T, typename U>
SaveableVector2<T,U>::SaveableVector2()
    : Vector2<Saveable<T>, U>()
{}


template <typename T, typename U>
SaveableVector2<T,U>::SaveableVector2( T const& x, T const& y )
    : Vector2<Saveable<T>, U>( x, y )
{}


template <typename T, typename U>
SaveableVector2<T,U>::SaveableVector2( Vector2<T,U> const& o )
    : Vector2<Saveable<T>, U>( o.x, o.y )
{}


template <typename T, typename U>
SaveableVector2<T,U>::SaveableVector2( Vector2<Saveable<T>,U> const& o )
    : Vector2<Saveable<T>, U>( o.x, o.y )
{}


template <typename T, typename U>
SaveableVector2<T,U>::~SaveableVector2()
{}


template <typename T, typename U>
SaveableVector2<T,U>::operator Vector2<T,U>() const
{
    return (*this)();
}


template <typename T, typename U>
Vector2<T,U> SaveableVector2<T,U>::operator ()() const
{
    return Vector2<T,U>{ this->x, this->y };
}


template <typename T, typename U>
Vector2<T,U> operator +( Vector2<Saveable<T>,U> const& lhs, Vector2<Saveable<T>,U> const& rhs )
{
    return { lhs.x + rhs.x, lhs.y + rhs.y };
}


template <typename T, typename U>
Vector2<T,U> operator +( Vector2<Saveable<T>,U> const& lhs, Vector2<T,U> const& rhs )
{
    return { lhs.x + rhs.x, lhs.y + rhs.y };
}


template <typename T, typename U>
Vector2<T,U> operator +( Vector2<T,U> const& lhs, Vector2<Saveable<T>,U> const& rhs )
{
    return { lhs.x + rhs.x, lhs.y + rhs.y };
}


template <typename T, typename U>
Vector2<T,U> operator -( Vector2<Saveable<T>,U> const& lhs, Vector2<Saveable<T>,U> const& rhs )
{
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}


template <typename T, typename U>
Vector2<T,U> operator -( Vector2<Saveable<T>,U> const& lhs, Vector2<T,U> const& rhs )
{
    return { lhs.x - rhs.x, lhs.y - rhs.y };
}


template <typename T, typename U>
Vector2<T,U> operator -( Vector2<T,U> const& lhs, Vector2<Saveable<T>,U> const& rhs )
{
    return { lhs.x - rhs.x, lhs.y - rhs.y };
}


template <typename T, typename U, typename V>
Vector2<T,U> operator *( Vector2<Saveable<T>,U> const& lhs, Saveable<V> const& rhs )
{
    return Vector2<T,U>{ lhs.x, lhs.y } * rhs();
}


template <typename T, typename U, typename V>
Vector2<T,U> operator *( Vector2<Saveable<T>,U> const& lhs, V const& rhs )
{
    return Vector2<T,U>{ lhs.x, lhs.y } * rhs;
}


template <typename T, typename U, typename V>
Vector2<T,U> operator *( Vector2<T,U> const& lhs, Saveable<V> const& rhs )
{
    return lhs * rhs();
}


template <typename T, typename U, typename V>
Vector2<T,U> operator /( Vector2<Saveable<T>,U> const& lhs, Saveable<V> const& rhs )
{
    return Vector2<T,U>{ lhs.x, lhs.y } / rhs();
}


template <typename T, typename U, typename V>
Vector2<T,U> operator /( Vector2<Saveable<T>,U> const& lhs, V const& rhs )
{
    return Vector2<T,U>{ lhs.x, lhs.y } / rhs;
}


template <typename T, typename U, typename V>
Vector2<T,U> operator /( Vector2<T,U> const& lhs, Saveable<V> const& rhs )
{
    return lhs / rhs();
}


template <typename T, typename U, typename V>
Vector2<T,U> operator +( Vector2<Saveable<T>,U> const& lhs, Saveable<V> const& rhs )
{
    return Vector2<T,U>{ lhs.x, lhs.y } + rhs();
}


template <typename T, typename U, typename V>
Vector2<T,U> operator +( Vector2<Saveable<T>,U> const& lhs, V const& rhs )
{
    return Vector2<T,U>{ lhs.x, lhs.y } + rhs;
}


template <typename T, typename U, typename V>
Vector2<T,U> operator +( Vector2<T,U> const& lhs, Saveable<V> const& rhs )
{
    return lhs + rhs();
}


template <typename T, typename U, typename V>
Vector2<T,U> operator -( Vector2<Saveable<T>,U> const& lhs, Saveable<V> const& rhs )
{
    return Vector2<T,U>{ lhs.x, lhs.y } - rhs();
}


template <typename T, typename U, typename V>
Vector2<T,U> operator -( Vector2<Saveable<T>,U> const& lhs, V const& rhs )
{
    return Vector2<T,U>{ lhs.x, lhs.y } - rhs;
}


template <typename T, typename U, typename V>
Vector2<T,U> operator -( Vector2<T,U> const& lhs, Saveable<V> const& rhs )
{
    return lhs.x - rhs();
}


} // tinyspace


#endif // _TINYSPACE_SAVEABLE_TPP_
