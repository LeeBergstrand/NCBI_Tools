// $Id: string-util.hpp 142289 2008-10-03 20:47:49Z rotmistr $
#ifndef OLIGOFAR_STRINGUTIL__HPP
#define OLIGOFAR_STRINGUTIL__HPP

#include "defs.hpp"
#include <strstream>
#include <cstring>
#include <string>

BEGIN_OLIGOFAR_SCOPES

template<class iterator> 
inline iterator Split(const string& str, const string& delim,
                      iterator i, bool tokenize = true) 
{
	const char * c = str.c_str();
	int cnt = 0;
	while( const char * cc = strpbrk( c, delim.c_str() ) ) {
		if( cc > c || !tokenize ) *i++ = string( c, cc );
		c = cc + 1;
		if( tokenize ) while( *c && strchr( delim.c_str(), *c ) ) ++c;
		++cnt;
	}
	*i++ = c;
	return i;
}

template<class container>
inline string Join(const string& delim, const container& c) 
{
	ostringstream ret;
	typedef typename container::const_iterator iterator;
	for( iterator q = c.begin(); q != c.end(); ++q ) {
		if( q != c.begin() ) ret << delim;
		ret << *q;
	}
	return ret.str();
}

template<class iterator>
inline string Join(const string& delim, iterator b, iterator e) 
{
	ostringstream ret;
	for( iterator i = b; i != e; ++i ) {
		if( i != b ) ret << delim;
		ret << *i;
	}
	return ret.str();
}

END_OLIGOFAR_SCOPES

#endif
