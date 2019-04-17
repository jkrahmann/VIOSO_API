#pragma once
#include <string>
#include <map>
#include <vector>
#include <istream>
class JASONWrapper
{
public:
	typedef enum NODE_TYPE
	{
		NODE_TYPE_UNKNOWN,
		NODE_TYPE_HASH,
		NODE_TYPE_LIST,
		NODE_TYPE_STRING,
		NODE_TYPE_BOOL,
		NODE_TYPE_UINT,
		NODE_TYPE_INT,
		NODE_TYPE_FLOAT,
		NODE_TYPE_END
	} NODE_TYPE;
	
	struct Node
	{
		NODE_TYPE type;
		union {
			std::map < std::string, Node> hash;
			std::vector< Node > list;
			std::string string;
			bool boolean;
			uint64_t uint;
			int64_t integer;
			double floating;
		};

		Node() : type( NODE_TYPE_UNKNOWN ), floating( 0.0 ) {}
		Node( std::string s ) : type( NODE_TYPE_STRING ), string( s ) {}
		Node( bool b ) : type( NODE_TYPE_BOOL ), boolean( b ) {}
		Node( uint64_t u ) : type( NODE_TYPE_UINT ), uint( u ) {}
		Node( int64_t i ) : type( NODE_TYPE_INT ), integer( i ) {}
		Node( double d ) : type( NODE_TYPE_FLOAT ), floating( d ) {}
		Node( Node const& other ) : type( other.type )
		{
			*this = other;
		}

		~Node()
		{
			if( NODE_TYPE_HASH == type )
			{
				hash.clear();
			}
			else if( NODE_TYPE_LIST == type )
			{
				list.clear();
				list.shrink_to_fit();
			}
			floating = 0;
		}

		Node& operator=( Node const& other )
		{
			if( NODE_TYPE_HASH == type )
			{
				hash.clear();
			}
			else if( NODE_TYPE_LIST == type )
			{
				list.clear();
				list.shrink_to_fit();
			}
			type = other.type;
			floating = 0;
			// we need to deep copy all the data
			switch( type )
			{
			case NODE_TYPE_HASH:
				hash = other.hash;
				break;
			case NODE_TYPE_LIST:
				list = other.list;
				break;
			default:
				floating = other.floating;
			};
			return *this;
		}

		Node& operator[]( std::string hashtag )
		{
			if( NODE_TYPE_HASH == type )
			{
				return hash[hashtag];
			}
			#ifdef _DEBUG
			throw 0;
			#endif
			return *this;
		}

		Node const& operator[]( std::string hashtag ) const
		{
			if( NODE_TYPE_HASH == type )
			{
				auto found = hash.find( hashtag );
				if( hash.end() != found )
				{
					return found->second;
				}
			}
			#ifdef _DEBUG
			throw 0;
			#endif
			return *this;
		}

		Node& operator[]( size_t index )
		{
			if( NODE_TYPE_LIST == type )
			{
				if( list.size() < index )
				{
					return list[index];
				}
			}
			#ifdef _DEBUG
			throw 0;
			#endif
			return *this;
		}
		Node const& operator[]( size_t index ) const
		{
			if( NODE_TYPE_LIST == type )
			{
				if( list.size() < index )
				{
					return list[index];
				}
			}
			#ifdef _DEBUG
			throw 0;
			#endif
			return *this;
		}

		operator bool() const
		{
			if( NODE_TYPE_BOOL == type )
				return boolean;
			else
			{
				if( isIntegral() )
				{
					Node e = *this;
					if( e.changeTypeTo( NODE_TYPE_BOOL ) )
					{
						return e.boolean;
					}
				}
			}
			return false;
		}

		operator bool&()
		{
			if( NODE_TYPE_BOOL == type )
				return boolean;
			else
			{
				if( isIntegral() )
				{
					if( changeTypeTo( NODE_TYPE_BOOL ) )
					{
						return boolean;
					}
				}
			}
			*this = Node( false );
			return boolean;
		}

		bool isIntegral() const
		{ 
			return NODE_TYPE_STRING <= type && type < NODE_TYPE_END;
		}

		bool changeTypeTo( NODE_TYPE newType )
		{
			switch( type )
			{
			case NODE_TYPE_STRING:
				switch( newType )
				{
				case NODE_TYPE_STRING:
					return true;
				case NODE_TYPE_BOOL:
				{
					type = NODE_TYPE_BOOL;
					const std::string s( "TRUE" );
					boolean = std::equal( string.begin(), string.end(), s.begin(), s.end(),
										  []( char const& c1, char const& c2 )
					{
						return c1 == c2 || toupper( c1 ) == toupper( c2 );
					} );
					return true;
				}
				case NODE_TYPE_UINT:
					type = NODE_TYPE_UINT;
					return 1 == sscanf( string.c_str(), "%llu", &uint );
				case NODE_TYPE_INT:
					type = NODE_TYPE_INT;
					return 1 == sscanf( string.c_str(), "%lli", &integer );
				case NODE_TYPE_FLOAT:
					type = NODE_TYPE_FLOAT;
					return 1 == sscanf( string.c_str(), "%lf", &floating );
				default:
					return false;
				};
			case NODE_TYPE_BOOL:
				switch( newType )
				{
				case NODE_TYPE_STRING:
					type = NODE_TYPE_STRING;
					string = boolean ? "true" : "false";
					return true;
				case NODE_TYPE_BOOL:
					return true;
				case NODE_TYPE_UINT:
					type = NODE_TYPE_UINT;
					uint = boolean ? 1 : 0;
					return true;
				case NODE_TYPE_INT:
					type = NODE_TYPE_INT;
					integer = boolean ? 1 : 0;
					return true;
				case NODE_TYPE_FLOAT:
					type = NODE_TYPE_FLOAT;
					floating = boolean ? 1 : 0;
					return true;
				default:
					return false;
				};
			case NODE_TYPE_UINT:
				switch( newType )
				{
				case NODE_TYPE_STRING:
					string = std::to_string( uint );
					return true;
				case NODE_TYPE_BOOL:
					type = NODE_TYPE_BOOL;
					boolean = 0 != uint;
					return true;
				case NODE_TYPE_UINT:
					return true;
				case NODE_TYPE_INT:
					type = NODE_TYPE_INT;
					return true;
				case NODE_TYPE_FLOAT:
					type = NODE_TYPE_FLOAT;
					floating = (double)uint;
					return true;
				default:
					return false;
				};
			case NODE_TYPE_INT:
				switch( newType )
				{
				case NODE_TYPE_STRING:
					string = std::to_string( integer );
					return true;
				case NODE_TYPE_BOOL:
					type = NODE_TYPE_BOOL;
					boolean = 0 != integer;
					return true;
				case NODE_TYPE_UINT:
					type = NODE_TYPE_UINT;
					return true;
				case NODE_TYPE_INT:
					return true;
				case NODE_TYPE_FLOAT:
					type = NODE_TYPE_FLOAT;
					floating = (double)integer;
					return true;
				default:
					return false;
				};
			case NODE_TYPE_FLOAT:
				switch( newType )
				{
				case NODE_TYPE_STRING:
					string = std::to_string( floating );
					return true;
				case NODE_TYPE_BOOL:
					type = NODE_TYPE_BOOL;
					boolean = 0 != floating;
					return true;
				case NODE_TYPE_UINT:
					type = NODE_TYPE_UINT;
					uint = (uint64_t)floating;
					return true;
				case NODE_TYPE_INT:
					type = NODE_TYPE_INT;
					integer = (int64_t)floating;
					return true;
				case NODE_TYPE_FLOAT:
					return true;
				default:
					return false;
				};
			default:
				return false;
			};
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
	std::string to_string();
};

