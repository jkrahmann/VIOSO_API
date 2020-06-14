#pragma once
#include <string>
#include <map>
#include <vector>
#include <istream>
#include <sstream>
#include <utility>
#include <algorithm>    // std::equal
class JASONWrapper
{
public:
	enum class NODE_TYPE
	{
		EMPTY,
		HASH,
		LIST,
		STRING,
		BOOL,
		UINT,
		INT,
		FLOAT,
		END
	};

	struct Node
	{
		NODE_TYPE type;
		union {
			void* nested;
			bool boolean;
			uint64_t uint;
			int64_t integer;
			double floating;
		};

		Node() : type( NODE_TYPE::EMPTY ), floating( 0.0 ) {}
		Node(std::string const& s ) : type( NODE_TYPE::STRING ), nested( new std::string( s ) ) {}
		Node(std::map <std::string, Node> const& hash ) : type( NODE_TYPE::HASH ), nested( new std::map < std::string, Node>( hash ) ) {}
		Node(std::vector <Node> const& list ) : type( NODE_TYPE::LIST ), nested( new  std::vector <Node>( list ) ) {}
		Node(bool b ) : type( NODE_TYPE::BOOL ), boolean( b ) {}
		Node(int64_t i ) : type( NODE_TYPE::INT ), integer( i ) {}
		Node(double d ) : type( NODE_TYPE::FLOAT ), floating( d ) {}
		Node( Node const& other ) : type( NODE_TYPE::EMPTY )
		{
			// we need to deep copy all the data
			switch( type )
			{
			case NODE_TYPE::HASH:
				nested = new std::map<std::string, Node>( *reinterpret_cast<std::map<std::string, Node>*>( other.nested ) );
				break;
			case NODE_TYPE::LIST:
				nested = new std::vector<Node> ( *reinterpret_cast<std::vector<Node>*>( other.nested ) );
				break;
			case NODE_TYPE::STRING:
				nested = new std::string( *reinterpret_cast<std::string*>( other.nested ) );
				break;
			default:
				floating = other.floating;
			};
			type = other.type;
		}

		//std::map < std::string, Node> hash;
		//std::vector< Node > list;
		//std::string string;

		~Node()
		{
			// clean up
			if( NODE_TYPE::HASH == type )
			{
				if( nullptr != nested )
					delete reinterpret_cast<std::map<std::string, Node>*>( nested );
			}
			else if( NODE_TYPE::LIST == type )
			{
				if( nullptr != nested )
					delete reinterpret_cast<std::vector< Node>*>( nested );
			}
			else if( NODE_TYPE::STRING == type )
			{
				if( nullptr != nested )
					delete reinterpret_cast<std::string*>( nested );
			}
		}

		Node& operator[]( std::string hashtag )
		{
			if( nullptr != nested && NODE_TYPE::HASH == type )
			{
				return reinterpret_cast<std::map<std::string, Node>*>( nested )->operator[](hashtag);
			}
			throw std::invalid_argument("node type need to be hash");
			return *this;
		}

		Node const& operator[]( std::string hashtag ) const
		{
			if( nullptr != nested && NODE_TYPE::HASH == type )
			{
				auto found = reinterpret_cast<std::map<std::string, Node>*>( nested )->find( hashtag );
				if( reinterpret_cast<std::map<std::string, Node>*>( nested )->end() != found )
				{
					return found->second;
				}
			}
			throw std::invalid_argument( "node type need to be hash" );
			return *this;
		}

		Node& operator[]( size_t index )
		{
			if( nullptr != nested && NODE_TYPE::LIST == type )
			{
				return reinterpret_cast<std::vector<Node>*>( nested )->operator[](index);
			}
			throw std::invalid_argument( "node type need to be list" );
			return *this;
		}

		Node const& operator[]( size_t index ) const
		{
			if( nullptr != nested && NODE_TYPE::LIST == type )
			{
				return reinterpret_cast<std::vector<Node>*>( nested )->operator[]( index );
			}
			throw std::invalid_argument( "node type need to be list" );
			return *this;
		}

		operator bool() const
		{
			if( NODE_TYPE::BOOL == type )
				return boolean;
			else
			{
				if( isIntegral() )
				{
					Node e = *this;
					if( e.changeTypeTo( NODE_TYPE::BOOL ) )
					{
						return e.boolean;
					}
				}
			}
			return false;
		}

		bool isIntegral() const
		{
			return NODE_TYPE::STRING <= type && type < NODE_TYPE::END;
		}

		bool changeTypeTo( NODE_TYPE newType )
		{
			try {
				switch( type )
				{
				case NODE_TYPE::STRING:
				{
					if( nullptr == nested )
						throw std::overflow_error( "fatal: not set" );
					std::string& str = *reinterpret_cast<std::string*>( nested );
					switch( newType )
					{
					case NODE_TYPE::STRING:
						break;
					case NODE_TYPE::BOOL:
					{
						type = NODE_TYPE::BOOL;
						// const char* s( "TRUE" );
						std::string s("TRUE");
						boolean = std::equal( str.begin(), str.end(), s.begin(),
						// s + 5,
											  []( char const& c1, char const& c2 )
						{
							return c1 == c2 || toupper( c1 ) == toupper( c2 );
						} );
						break;
					}
					case NODE_TYPE::UINT:
						uint = std::stoull( str );
						type = NODE_TYPE::UINT;
						break;
					case NODE_TYPE::INT:
						integer = std::stoll( str );
						type = NODE_TYPE::INT;
						break;
					case NODE_TYPE::FLOAT:
						floating = std::stod( str );
						type = NODE_TYPE::FLOAT;
						break;
					default:
						throw std::invalid_argument( "invald type to convert to" );
					};
					delete &str;
				}
				case NODE_TYPE::BOOL:
					switch( newType )
					{
					case NODE_TYPE::STRING:
						nested = new std::string( boolean ? "true" : "false" );
						type = NODE_TYPE::STRING;
						break;
					case NODE_TYPE::BOOL:
						break;
					case NODE_TYPE::UINT:
						uint = boolean ? 1 : 0;
						type = NODE_TYPE::UINT;
						break;
					case NODE_TYPE::INT:
						integer = boolean ? 1 : 0;
						type = NODE_TYPE::INT;
						break;
					case NODE_TYPE::FLOAT:
						floating = boolean ? 1 : 0;
						type = NODE_TYPE::FLOAT;
						break;
					default:
						throw std::invalid_argument( "invald type to convert to" );
					};
				case NODE_TYPE::UINT:
					switch( newType )
					{
					case NODE_TYPE::STRING:
						nested = new std::string( std::to_string( uint ) );
						type = NODE_TYPE::STRING;
						break;
					case NODE_TYPE::BOOL:
						boolean = 0 != uint;
						type = NODE_TYPE::BOOL;
						break;
					case NODE_TYPE::UINT:
						break;
					case NODE_TYPE::INT:
						type = NODE_TYPE::INT;
						break;
					case NODE_TYPE::FLOAT:
						floating = (double)uint;
						type = NODE_TYPE::FLOAT;
						break;
					default:
						throw std::invalid_argument( "invald type to convert to" );
					};
				case NODE_TYPE::INT:
					switch( newType )
					{
					case NODE_TYPE::STRING:
						nested = new std::string( std::to_string( integer ) );
						type = NODE_TYPE::STRING;
						break;
					case NODE_TYPE::BOOL:
						boolean = 0 != integer;
						type = NODE_TYPE::BOOL;
						break;
					case NODE_TYPE::UINT:
						type = NODE_TYPE::UINT;
						break;
					case NODE_TYPE::INT:
						break;
					case NODE_TYPE::FLOAT:
						floating = double(integer);
						type = NODE_TYPE::FLOAT;
						break;
					default:
						throw std::invalid_argument( "invald type to convert to" );
					};
				case NODE_TYPE::FLOAT:
					switch( newType )
					{
					case NODE_TYPE::STRING:
						nested = new std::string( std::to_string( floating ) );
						type = NODE_TYPE::STRING;
						break;
					case NODE_TYPE::BOOL:
						boolean = 0 != floating;
						type = NODE_TYPE::BOOL;
						break;
					case NODE_TYPE::UINT:
						uint = (uint64_t)floating;
						type = NODE_TYPE::UINT;
						break;
					case NODE_TYPE::INT:
						integer = (int64_t)floating;
						type = NODE_TYPE::INT;
						break;
					case NODE_TYPE::FLOAT:
						break;
					default:
						throw std::invalid_argument( "invald type to convert to" );
					};
				default:
					throw std::invalid_argument( "invald type to convert from" );
				};
			}
			catch( std::exception& e )
			{
				return false;
			}
			return true;
		};

		size_t size() const
		{
			if( nullptr != nested && NODE_TYPE::LIST == type )
				return reinterpret_cast<std::vector<Node> const*>( nested )->size();
			throw std::invalid_argument( "invald type to convert to" );
		}

		std::vector<std::string> tags() const
		{
			if( nullptr != nested && NODE_TYPE::HASH == type )
			{
				std::vector<std::string> vs;
				for( auto const& p : *reinterpret_cast<std::map< std::string, Node> const*>( nested ) )
					vs.push_back( p.first );
				
			}
			throw std::invalid_argument( "invald type to convert to" );
			return std::vector<std::string>();
		}

		std::string to_string( int ident = 0) const
		{
			std::string ss;
			std::ostringstream s(ss);
			try {
				switch( type )
				{
				case NODE_TYPE::HASH:
					if( nullptr == nested )
						throw std::overflow_error( "empty parameter not allowed" );
					for( int i = 0; i != ident; i++ )
						s << " ";
					s << "{\n";
					for( auto const& p : *reinterpret_cast<std::map< std::string, Node > const*>( nested ) )
					{
						if( p != *reinterpret_cast<std::map< std::string, Node > const*>( nested )->begin() )
							s << ",\n";
						for( int i = 0; i != ident; i++ )
							s << " ";
						s << "  \"" << p.first << "\" : " << p.second.to_string( ident + 2 );
					}
					for( int i = 0; i != ident; i++ )
						s << " ";
					s << "\n}";
					break;
				case NODE_TYPE::LIST:
					if( nullptr == nested )
						throw std::overflow_error( "empty parameter not allowed" );
					for( int i = 0; i != ident; i++ )
						s << " ";
					s << "[";
					for( auto const& p : *reinterpret_cast<std::vector<Node > const*>( nested ) )
					{
						if( p != *reinterpret_cast<std::vector< Node > const*>( nested )->begin() )
							s << ", ";
						s << p.to_string( ident );
					}
					s << "\n]";
					break;
				case NODE_TYPE::STRING:
					if( nullptr == nested )
						throw std::overflow_error( "empty parameter not allowed" );
					s << "\"" << *reinterpret_cast<std::string const*>( nested ) << "\"";
					break;
				case NODE_TYPE::BOOL:
					s << boolean ? "\"true\"" : "\"false\"";
					break;
				case NODE_TYPE::UINT:
					s << uint;
					break;
				case NODE_TYPE::INT:
					s << integer;
					break;
				case NODE_TYPE::FLOAT:
					s << floating;
					break;
				default:
					throw std::invalid_argument( "invald type to convert from" );
				};
			}
			catch( std::exception& e )
			{
			}
			return ss;
		};
	};

protected:
	int m_id;
	std::string method;
	Node params;

public:
	JASONWrapper();
	JASONWrapper( std::string s );
	JASONWrapper( std::istream s );
	virtual ~JASONWrapper();
	std::string to_string() const;
};

