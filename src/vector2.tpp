// Vector2.tpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.12 | LICENSE: WTFPL/PDM/CC0... your choice


#ifndef _TINYSPACE_VECTOR2_TPP_
#define _TINYSPACE_VECTOR2_TPP_


#include <cmath>


namespace tinyspace {


template <typename T>
static inline T sign( T t )
{
    return t > T( 0 ) ? 1 : t < T( 0 ) ? -1 : t;
}


template <typename T, typename U>
inline Vector2<T,U>::Vector2()
    : x()
    , y()
{}


template <typename T, typename U>
inline Vector2<T,U>::Vector2( T const& x, T const& y )
    : x( x )
    , y( y )
{}


template <typename T, typename U>
inline Vector2<T,U>::Vector2( Vector2<T,U> const& o )
    : x( o.x )
    , y( o.y )
{}


template <typename T, typename U>
inline Vector2<T,U>& Vector2<T,U>::operator =( Vector2<T,U> const& o )
{
    x = o.x;
    y = o.y;
    return *this;
}


template <typename T, typename U>
inline Vector2<T,U> Vector2<T,U>::operator +( Vector2<T,U> const& o ) const
{
    return { x+o.x, y+o.y };
}


template <typename T, typename U>
inline Vector2<T,U> Vector2<T,U>::operator -( Vector2<T,U> const& o ) const
{
    return { x-o.x, y-o.y };
}


template <typename T, typename U>
inline Vector2<T,U> Vector2<T,U>::operator -() const
{
    return { -x, -y };
}


template <typename T, typename U>
inline Vector2<T,U> Vector2<T,U>::operator *( U u ) const
{
    return { x*u, y*u };
}


template <typename T, typename U>
inline Vector2<T,U> Vector2<T,U>::operator /( U u ) const
{
    return { x/u, y/u };
}


template <typename T, typename U>
inline Vector2<T,U> Vector2<T,U>::operator +( U u ) const
{
    return *this + ( this->normalized() * u );
}


template <typename T, typename U>
inline Vector2<T,U> Vector2<T,U>::operator -( U u ) const
{
    return *this - ( this->normalized() * u );
}


template <typename T, typename U>
inline U Vector2<T,U>::magnitude() const
{
    return sqrt( x*x + y*y );
}


template <typename T, typename U>
inline Vector2<T,U> Vector2<T,U>::normalized() const
{
    U m = magnitude();
    if ( ! m )
    {
        return *this;
    }
    return { static_cast<T>( x/m ), static_cast<T>( y/m ) };
}


template <typename T, typename U>
inline Vector2<T,U> Vector2<T,U>::port() const
{
    return { -y, x }; // relative left
}


template <typename T, typename U>
inline Vector2<T,U> Vector2<T,U>::starboard() const
{
    return { y, -x }; // relative right
}


template <typename T, typename U>
inline T Vector2<T,U>::dot( Vector2<T,U> const& o ) const
{
    return x*o.x + y*o.y;
}


template <typename T, typename U>
inline T Vector2<T,U>::cross( Vector2<T,U> const& o ) const
{
    return x*o.y - y*o.x;
}


template <typename T, typename U>
inline T Vector2<T,U>::angleRad( Vector2<T,U> const& o ) const
{
    return sign( starboard().dot( o )) * acos( dot( o ) / ( magnitude() * o.magnitude() ));
}


template <typename T, typename U>
inline T Vector2<T,U>::angleDeg( Vector2<T,U> const& o ) const
{
    return angleRad( o ) * 180.f / M_PI;
}


template <typename T, typename U>
inline T Vector2<T,U>::angleRad2( Vector2<T,U> const& o ) const
{
    return -atan2( cross( o ), dot( o ));
}


template <typename T, typename U>
inline T Vector2<T,U>::angleDeg2( Vector2<T,U> const& o ) const
{
    return angleRad2( o ) * 180.f / M_PI;
}


template <typename T, typename U>
inline std::ostream& operator <<( std::ostream& os, const Vector2<T,U>& o )
{
    os << '{'
       << o.x
       << ','
       << o.y
       << '}';
    return os;
}


} // tinyspace


#endif  // _TINYSPACE_VECTOR2_TPP_
