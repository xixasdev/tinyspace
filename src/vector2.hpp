// vector2.hpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.12 | LICENSE: WTFPL/PDM/CC0... your choice


#ifndef _TINYSPACE_VECTOR2_HPP_
#define _TINYSPACE_VECTOR2_HPP_


#include <ostream>


namespace tinyspace {


template <typename T, typename U=float>
struct Vector2
{
    T x;
    T y;

    Vector2();
    Vector2( T const& x, T const& y );
    Vector2( Vector2<T,U> const& o );

    Vector2<T,U>& operator =( Vector2<T,U> const& o );

    Vector2<T,U> operator +( Vector2<T,U> const& o ) const;
    Vector2<T,U> operator -( Vector2<T,U> const& o ) const;

    Vector2<T,U> operator -() const;

    Vector2<T,U> operator *( U u ) const;
    Vector2<T,U> operator /( U u ) const;
    Vector2<T,U> operator +( U u ) const;
    Vector2<T,U> operator -( U u ) const;

    U magnitude() const;
    Vector2<T,U> normalized() const;

    Vector2<T,U> port() const;      // relative left
    Vector2<T,U> starboard() const; // relative right

    T dot( Vector2<T,U> const& o ) const;
    T cross( Vector2<T,U> const& o ) const;

    // Faster, but less accurate for very small angles
    T angleRad( Vector2<T,U> const& o ) const;
    T angleDeg( Vector2<T,U> const& o ) const;

    // Slower, but more accurate for very small angles
    T angleRad2( Vector2<T,U> const& o ) const;
    T angleDeg2( Vector2<T,U> const& o ) const;
};
template <typename T, typename U>
std::ostream& operator <<( std::ostream& os, const Vector2<T,U>& o );


} // tinyspace


#include "vector2.tpp"


#endif  // _TINYSPACE_VECTOR2_HPP_
