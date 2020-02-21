#pragma once
#include <string>
#include <map>
#include <vector>
#include <istream>
class JASONWrapper
{
public:
	enum class NODE_TYPE
	{
		UNKNOWN,
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
			std::map < std::string, Node> hash;
			std::vector< Node > list;
			std::string string;
			bool boolean;
			uint64_t uint;
			int64_t integer;
			double floating;
		};

		Node() : type( NODE_TYPE::UNKNOWN ), floating( 0.0 ) {}
		Node( std::string s ) : type( NODE_TYPE::STRING ), string( s ) {}
		Node( bool b ) : type( NODE_TYPE::BOOL ), boolean( b ) {}
		Node( int64_t i ) : type( NODE_TYPE::INT ), integer( i ) {}
		Node( double d ) : type( NODE_TYPE::FLOAT ), floating( d ) {}
		Node( Node const& other ) : type( other.type )
		{
			*this = other;
		}

		~Node()
		{
			if( NODE_TYPE::HASH == type )
			{
				hash.clear();
			}
			else if( NODE_TYPE::LIST == type )
			{
				list.clear();
				list.shrink_to_fit();
			}
			floating = 0;
		}

		Node& operator=( Node const& other )
		{
			if( NODE_TYPE::HASH == type )
			{
				hash.clear();
			}
			else if( NODE_TYPE::LIST == type )
			{
				list.clear();
				list.shrink_to_fit();
			}
			type = other.type;
			floating = 0;
			// we need to deep copy all the data
			switch( type )
			{
			case NODE_TYPE::HASH:
				hash = other.hash;
				break;
			case NODE_TYPE::LIST:
				list = other.list;
				break;
			default:
				floating = other.floating;
			};
			return *this;
		}

		Node& operator[]( std::string hashtag )
		{
			if( NODE_TYPE::HASH == type )
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
			if( NODE_TYPE::HASH == type )
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
			if( NODE_TYPE::LIST == type )
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
			if( NODE_TYPE::LIST == type )
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

		operator bool&()
		{
			if( NODE_TYPE::BOOL == type )
				return boolean;
			else
			{
				if( isIntegral() )
				{
					if( changeTypeTo( NODE_TYPE::BOOL ) )
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
			return NODE_TYPE::STRING <= type && type < NODE_TYPE::END;
		}

		bool changeTypeTo( NODE_TYPE newType )
		{
			switch( type )
			{
			case NODE_TYPE::STRING:
				switch( newType )
				{
				case NODE_TYPE::STRING:
					return true;
				case NODE_TYPE::BOOL:
				{
					type = NODE_TYPE::BOOL;
					const std::string s( "TRUE" );
					boolean = std::equal( string.begin(), string.end(), s.begin(), s.end(),
										  []( char const& c1, char const& c2 )
					{
						return c1 == c2 || toupper( c1 ) == toupper( c2 );
					} );
					return true;
				}
				case NODE_TYPE::UINT:
					type = NODE_TYPE::UINT;
					return 1 == sscanf( string.c_str(), "%llu", &uint );
				case NODE_TYPE::INT:
					type = NODE_TYPE::INT;
					return 1 == sscanf( string.c_str(), "%lli", &integer );
				case NODE_TYPE::FLOAT:
					type = NODE_TYPE::FLOAT;
					return 1 == sscanf( string.c_str(), "%lf", &floating );
				default:
					return false;
				};
			case NODE_TYPE::BOOL:
				switch( newType )
				{
				case NODE_TYPE::STRING:
					type = NODE_TYPE::STRING;
					string = boolean ? "true" : "false";
					return true;
				case NODE_TYPE::BOOL:
					return true;
				case NODE_TYPE::UINT:
					type = NODE_TYPE::UINT;
					uint = boolean ? 1 : 0;
					return true;
				case NODE_TYPE::INT:
					type = NODE_TYPE::INT;
					integer = boolean ? 1 : 0;
					return true;
				case NODE_TYPE::FLOAT:
					type = NODE_TYPE::FLOAT;
					floating = boolean ? 1 : 0;
					return true;
				default:
					return false;
				};
			case NODE_TYPE::UINT:
				switch( newType )
				{
				case NODE_TYPE::STRING:
					string = std::to_string( uint );
					return true;
				case NODE_TYPE::BOOL:
					type = NODE_TYPE::BOOL;
					boolean = 0 != uint;
					return true;
				case NODE_TYPE::UINT:
					return true;
				case NODE_TYPE::INT:
					type = NODE_TYPE::INT;
					return true;
				case NODE_TYPE::FLOAT:
					type = NODE_TYPE::FLOAT;
					floating = (double)uint;
					return true;
				default:
					return false;
				};
			case NODE_TYPE::INT:
				switch( newType )
				{
				case NODE_TYPE::STRING:
					string = std::to_string( integer );
					return true;
				case NODE_TYPE::BOOL:
					type = NODE_TYPE::BOOL;
					boolean = 0 != integer;
					return true;
				case NODE_TYPE::UINT:
					type = NODE_TYPE::UINT;
					return true;
				case NODE_TYPE::INT:
					return true;
				case NODE_TYPE::FLOAT:
					type = NODE_TYPE::FLOAT;
					floating = (double)integer;
					return true;
				default:
					return false;
				};
			case NODE_TYPE::FLOAT:
				switch( newType )
				{
				case NODE_TYPE::STRING:
					string = std::to_string( floating );
					return true;
				case NODE_TYPE::BOOL:
					type = NODE_TYPE::BOOL;
					boolean = 0 != floating;
					return true;
				case NODE_TYPE::UINT:
					type = NODE_TYPE::UINT;
					uint = (uint64_t)floating;
					return true;
				case NODE_TYPE::INT:
					type = NODE_TYPE::INT;
					integer = (int64_t)floating;
					return true;
				case NODE_TYPE::FLOAT:
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

