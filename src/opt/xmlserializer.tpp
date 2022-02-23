// xmlserializer.tpp
// tiny space v3 - experiment: snapshot serialization (C++11)
// AUTHOR: xixas | DATE: 2022.02.12 | LICENSE: WTFPL/PDM/CC0... your choice


#ifndef _TINYSPACE_XMLSERIALIZER_TPP_
#define _TINYSPACE_XMLSERIALIZER_TPP_


#include <sstream>


namespace tinyspace {


template<typename T>
string XmlSerializer::number( T const& v )
{
    std::ostringstream os;
    os << v;
    return os.str();
}


template <typename T, typename U>
string XmlSerializer::vector2( Vector2<T,U> const& v )
{
    std::ostringstream os;
    os << '{' << v.x << ',' << v.y << '}';
    return os.str();
}


template <typename T>
string XmlSerializer::pair( std::pair<T,T> const& v )
{
    std::ostringstream os;
    os << '{' << v.first << ',' << v.second << '}';
    return os.str();
}


} // tinyspace


#endif // _TINYSPACE_XMLSERIALIZER_TPP_
