#include <regex>
#include <string>
#include <iterator>
#include <type_traits>

namespace x {

using std::basic_regex;
using std::regex_search;
using std::match_results;

template<class ExprT, class SourceT>
struct basic_match_helper {
	static_assert(sizeof(SourceT)!=sizeof(SourceT), "Unable to process SourceT"); 
};

//specialize on To
template<class matchT, class To>
struct convert_match {
	static_assert(sizeof(matchT)!=sizeof(matchT),"No conversion from type From to type To");
};

//cstring
template<class CharT, class Traits>
struct basic_match_helper< basic_regex<CharT, Traits>, CharT> {
    typedef basic_match_helper< basic_regex<CharT, Traits>, CharT> self_type;

    basic_regex<CharT, Traits> expr;
	std::basic_string<CharT> str;
    basic_match_helper ( const basic_regex<CharT, Traits >& expr, const CharT *str ) : expr(expr), str(str) { }

	template<class T>
	operator T () { return convert_match<self_type, T>()(this); }

    typename std::basic_string<CharT>::iterator begin() { return str.begin(); }
    typename std::basic_string<CharT>::iterator end()	{ return str.end(); }
};

//string
template<class CharT, class Traits, class ST, class SA>
struct basic_match_helper< basic_regex<CharT, Traits>, std::basic_string<CharT, ST, SA> > {
    typedef  basic_match_helper< basic_regex<CharT, Traits>, std::basic_string<CharT, ST, SA> > self_type;
    typedef  std::basic_string<CharT, ST, SA> str_type;
    
    basic_regex<CharT, Traits> expr;
	const str_type& str;
    basic_match_helper ( const basic_regex<CharT, Traits >& expr, const str_type& str ) : expr(expr), str(str) { }
    
	template<class T>
	operator T () { return convert_match<self_type, T>()(this); }

    typename str_type::const_iterator begin() { return str.cbegin(); }
    typename str_type::const_iterator end()	{ return str.cend(); }
};

//range
template<class CharT, class Traits, class Range>
struct basic_match_helper< basic_regex<CharT, Traits>, Range > {
    typedef  basic_match_helper< basic_regex<CharT, Traits>, Range > self_type;
    typedef  typename Range::const_iterator iter_type;
    
    basic_regex<CharT, Traits> expr;
	const Range& rng;
    basic_match_helper ( const basic_regex<CharT, Traits >& expr, const Range& rng ) : expr(expr), rng(rng){
    	static_assert( std::is_same<CharT, typename Range::value_type>::value, "Cannot use the elements of Range with CharT. They must be the same.");
    }
    
	template<class T>
	operator T () { return convert_match<self_type, T>()(this); }

    iter_type begin()	{ return rng.cbegin(); }
    iter_type end()		{ return rng.cend(); }
};

template<class CharT,  class Traits = std::regex_traits<CharT>>
struct basic_match {
	basic_regex<CharT, Traits> expr;
	basic_match( const CharT* lit ) : expr(lit) { }
	
	basic_match_helper< basic_regex<CharT, Traits>, CharT> operator()(const char* str) {
		return basic_match_helper< basic_regex<CharT, Traits>, CharT>(expr, str);
	}
};

typedef basic_match<char> match;
typedef basic_match<wchar_t> wmatch;
//typedef basic_match<char16_t> u16match;
//typedef basic_match<char32_t> u32match;

namespace literals {
	match operator"" _match (const char* str, size_t) { return match(str); }
	wmatch operator"" _match (const wchar_t* str, size_t) { return wmatch(str); }
//	u16match operator"" _match (const char16_t* str, size_t) { return u16match(str); }
//	u32match operator"" _match (const char32_t* str, size_t) { return u32match(str); }
}

//CONVERSIONS
//bool - matches or not
template<class matchT>
struct convert_match<matchT, bool> {
	bool operator() ( matchT* ptr) {
		return regex_search( ptr->begin(), ptr->end(), ptr->expr);
	}
};

//count - may need for EACH int-type?
template<class matchT>
struct convert_match<matchT, int> {
	int operator() ( matchT* ptr) {
		int i =0;
		auto iter=ptr->begin(), end=ptr->end();
		match_results< decltype(iter) > m;
		while( regex_search( iter, end, m, ptr->expr) ) {
			++i;
			iter=m.suffix().first;
		}
		return i;
	}
};

//string - get matched stuff
template<class matchT, class CharT, class ST, class SA>
struct convert_match<matchT, std::basic_string<CharT,ST,SA> > {
	std::basic_string<CharT,ST,SA> operator() ( matchT* ptr) {
		match_results< decltype(ptr->begin()) > m;
		regex_search( ptr->begin(), ptr->end(), m, ptr->expr);
		return m.str();
	}
};
}//namespace x

#include <vector>

namespace x {
//vector<string> - get all matched stuff. Not each capture, but each set of groups
// "."_match("test") == {'t' 'e' 's' 't'}
template<class matchT, class CharT, class ST, class SA>
struct convert_match<matchT, std::vector< std::basic_string<CharT,ST,SA> > > {
	std::vector< std::basic_string<CharT,ST,SA> > operator() ( matchT* ptr) {
		match_results< decltype(ptr->begin()) > m;
		std::vector< std::basic_string<CharT,ST,SA> > v;
		auto iter=ptr->begin(), end=ptr->end();
		while( regex_search( iter, end, m, ptr->expr) ) {
			v.push_back(m.str());
			iter=m.suffix().first;
		}
		return std::move(v);	
	}
};

}//namespace x

#include <array>

namespace x {

//array<string,3> - pre, match, post
template<class matchT, class CharT, class ST, class SA>
struct convert_match<matchT, std::array< std::basic_string<CharT,ST,SA>,3 > > {
	std::array< std::basic_string<CharT,ST,SA>, 3> operator() ( matchT* ptr) {
		match_results< decltype(ptr->begin()) > m;
		std::array< std::basic_string<CharT,ST,SA>, 3 > a;
		auto iter=ptr->begin(), end=ptr->end();
		regex_search( iter, end, m, ptr->expr);
		a[0]=m.prefix().str();
		a[1]=m.str();
		a[2]=m.suffix().str();
		return std::move(a);
	}
};

}

#include <iostream>
#include <algorithm>


using namespace x;
using namespace x::literals;

int main() {
	using namespace std;
	bool b="."_match("test");
    int  i="."_match("test");
    string s="."_match("test");
    vector<string> vs="."_match("test");
    array<string,3> as="."_match("test");
	cout<<"bool "<<b
		<<"\nint "<<i
		<<"\nstring "<<s
		<<endl;
	cout<<"vector<string> ";copy(vs.begin(), vs.end(), ostream_iterator<string>(cout,", ")); cout<<endl;
	cout<<"array<string,3> ";copy(as.begin(), as.end(), ostream_iterator<string>(cout,", ")); cout<<endl;
	return 0;
}
