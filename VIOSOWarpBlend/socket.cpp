#include "Platform.h"
#include "socket.h"
#include "logging.h"
#include <limits>
#include <string>
#include <sstream>

// ----------------------------------------------------------------------------------
//                               SocketAddress
// ----------------------------------------------------------------------------------

SocketAddress::SocketAddress(unsigned long addr, unsigned short port)
{
	sin_family = AF_INET;
	sin_port = Socket::hton(port);
	sin_addr.s_addr = (unsigned long)Socket::hton((u_int)addr); // problem?? works on win
}; 

SocketAddress::SocketAddress(char const* url, unsigned short port)
{
	sin_family=AF_INET;
	do
	{
		if(!port)
		{
			char* pC=strrchr( (char*)url, ':' );
			if( pC && (sscanf_s( pC, ":%hu", &port)>0) )
			{
				*pC=0x0;
				sin_port=Socket::hton(port);
				inet_pton(AF_INET, url, &sin_addr);
				*pC=':';
				break;
			}
		}

		sin_port=Socket::hton(port);
		inet_pton(AF_INET, url, &sin_addr);
	}while(0);

	if( INADDR_NONE == sin_addr.s_addr )
	{
		addrinfo* pai = NULL;
		char portStr[6]; _itoa_s( port, portStr, 10 );
		addrinfo hints = { 0 };
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		if( 0 == getaddrinfo( url, portStr, &hints, &pai ) && pai )
		{
			sin_family = AF_INET;
			sin_port = Socket::hton( port );
		#pragma warning( push )
		#pragma warning( disable : 4311 )
			sin_addr.s_addr = (ULONG)pai->ai_addr;
		#pragma warning( pop )
			freeaddrinfo( pai );
		}
		else
		{
			sin_family = AF_INET;
			sin_port = Socket::hton(port);
			sin_addr.s_addr = INADDR_ANY;
		}
	}
}

const SocketAddress& SocketAddress::operator=(const sockaddr& sa)
{ 
	memset(this,0,sizeof(SocketAddress));
	memcpy(this, &sa, sizeof(sockaddr));
	return *this; 
}

const SocketAddress& SocketAddress::operator=(const sockaddr_in& sin)
{ 
	memset(this,0,sizeof(SocketAddress));
	memcpy(this, &sin, sizeof(sockaddr_in));
	return *this; 
}

unsigned short SocketAddress::getPort() const
{
	return Socket::ntoh(sin_port);
};

void SocketAddress::setPort(unsigned short port)
{
	sin_port = Socket::hton(port);
};

SocketAddress SocketAddress::broadcast(unsigned short port)
{
	sockaddr_in sa;
	memset(&sa,0,sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port   = Socket::hton(port);
	sa.sin_addr.S_un.S_addr = INADDR_BROADCAST;
	return sa;
}

/**************************** Socket **************************************/

int Socket::create(int type, bool broadcast, int protocol, bool keepAlive,u_int qRecvBuffer,u_int qSendBuffer)
{ 
	int err;

	sock=socket( AF_INET, type, protocol);
		
	if(sock==INVALID_SOCKET)
		return WSAGetLastError();

	if(broadcast)
	{
		if( (SOCK_DGRAM==type) && ( (sock!=SOCKET_ERROR) || (sock!=INVALID_SOCKET) ) )
		{
			BOOL param=1;
			int err=::setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (const char*)&param, sizeof(BOOL));
			if(err==SOCKET_ERROR) 
				return err;
		} 
		else 
			return SOCKET_ERROR;
	}

	if(keepAlive)
	{
		u_int param=1;
			
		err=::setsockopt( sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&param, sizeof(u_int));
		if(err)
			return err;
	}

	if(qRecvBuffer)
	{
		int sz=0;
		if( !getRecvBuffSize(sz) && (sz<(int)qRecvBuffer))
		{
			sz=(int)qRecvBuffer;
			err=::setsockopt( sock, SOL_SOCKET, SO_RCVBUF, (const char FAR*)&sz, sizeof(sz));
			if(err)
				return err;
		}
	}

	if(qSendBuffer)
	{
		int sz=0;
		if( !getSendBuffSize(sz) && (sz<(int)qSendBuffer))
		{
			sz=(int)qSendBuffer;
			err=::setsockopt( sock, SOL_SOCKET, SO_SNDBUF, (const char FAR*)&sz, sizeof(sz));
			if(err)
				return err;
		}
	}

	return 0;
}

int Socket::setRecvBuff(int size)
{
	return ::setsockopt( sock, SOL_SOCKET, SO_RCVBUF, (const char FAR*)&size, sizeof(size));
}

int Socket::getRecvBuffSize(int& size)
{
	int len=sizeof(size);
	int err=::getsockopt( sock, SOL_SOCKET, SO_RCVBUF, (char FAR*)&size, &len);

	if( err || (size<0))
	{
		size=-1;
		return SOCKET_ERROR;
	}

	return 0;
}

int Socket::getSendBuffSize(int& size)
{
	int len=sizeof(size);
	int err=::getsockopt( sock, SOL_SOCKET, SO_SNDBUF, (char FAR*)&size, &len);

	if( err || (size<0))
	{
		size=-1;
		return SOCKET_ERROR;
	}

	return 0;
}

int Socket::close()
{ 
	if(sock == NULL || sock == INVALID_SOCKET) 
		return SOCKET_ERROR;
	int ret=0;

#ifdef WIN32
	ret=::closesocket(sock);
#else  /* def WIN32 */
	ret=::close(sock);
#endif /* def WIN32 */
	sock=0;

	return ret;
}

int Socket::tryRead(const double timeout) const 
{
	fd_set fd={1,sock};
	timeval t,*pt=NULL;
	if(timeout < INFINITETIMEOUT) 
	{
		t.tv_sec=(long)timeout;
		t.tv_usec=suseconds_t((timeout-t.tv_sec) * 1000000.0);
		pt=&t;
	}
	return ::select(0,&fd,NULL,NULL,pt);
}

int Socket::tryWrite(const double timeout) const 
{
	fd_set fd={1,sock};
	timeval t,*pt=NULL;
	if(timeout < INFINITETIMEOUT) 
	{
		t.tv_sec=(long)timeout;
		t.tv_usec=suseconds_t((timeout-t.tv_sec) * 1000000.0);
		pt=&t;
	}
	return ::select(0,NULL,&fd,NULL,pt);
}

int Socket::test(const double timeout) const
{
	fd_set fd={1,sock};
	timeval t,*pt=NULL;
	if(timeout < INFINITETIMEOUT) 
	{
		t.tv_sec=(long)timeout;
		t.tv_usec=suseconds_t((timeout-t.tv_sec) * 1000000.0);
		pt=&t;
	}
	return ::select(0,NULL,NULL,&fd,pt);
}

int	Socket::send(const char* pch, const int iSize, const double timeout)
{
	int i;
	int iBytesSend=0;
	int qBuf=10485760;
	int iBytesThisTime=0;
	int s=tryWrite(timeout);

	if(s==1) do 
	{
		i= iSize-iBytesSend > qBuf ? qBuf : iSize-iBytesSend;
		iBytesThisTime=::send( sock, pch, i, 0);
		if(iBytesThisTime<=0)
		{
			if( (iBytesThisTime==0) || (::WSAGetLastError()!=WSAENOBUFS) )
				break;
			for( qBuf=i>>1; qBuf>0 ; qBuf>>=1)
			{
				iBytesThisTime=::send( sock, pch, qBuf, 0);
				if(iBytesThisTime>=0)
					break;
			}
			if(iBytesThisTime<=0)
				break;
		}
		iBytesSend+=iBytesThisTime;
		if(iBytesSend>=iSize)
			break;
		pch+=iBytesThisTime;
	}while(1);

	return iBytesSend;
}

int	Socket::sendDirect(const char* pch, const int iSize)
{
	int iBytesSend=0;
	int iBytesThisTime=0;

	do 
	{
		if((iBytesThisTime=::send(sock,pch,iSize-iBytesSend,0))==0) break;
		if(iBytesThisTime == SOCKET_ERROR) break;
		if((iBytesSend+=iBytesThisTime) >= iSize) break;
		pch+=iBytesThisTime;
	}
	while(iBytesSend < iSize);

	return iBytesSend;
}

int	Socket::recv(char* pch, const int iSize, const double timeout)
{ 
	if(tryRead(timeout)==1) 
	{
		return ::recv(sock,pch,iSize,0);
	} 

	return 0;
}

int	Socket::recvDirect(char* pch, const int iSize)
{ 
	return ::recv(sock,pch,iSize,0);
}


SocketAddress	Socket::getpeeraddr()
{ 
	sockaddr a;
	int addLen=sizeof(sockaddr);
	if(::getpeername(sock,&a,&addLen) == SOCKET_ERROR)
		a.sa_family=AF_UNSPEC;
	return a;
}

SocketAddress   Socket::getsockaddr() 
{
	sockaddr a;
	int addLen=sizeof(sockaddr);

	ZeroMemory( &a, addLen);
	if(::getsockname( sock, &a, &addLen)==SOCKET_ERROR)
		a.sa_family=AF_UNSPEC;

	return a;
}

int Socket::getsockaddr(sockaddr_in& addr,int& qAddr)
{
	qAddr=sizeof(sockaddr_in);
	ZeroMemory( &addr, qAddr);
	return ::getsockname( sock, (sockaddr*)&addr, &qAddr);
}

int Socket::sendDatagram(const char* pch,const int iSize,SocketAddress& sa, bool dontRoute)
{
	//fd_set fd={1,sock};

	//if(::select( 0, NULL, &fd, NULL, NULL)==1)
		return ::sendto( sock, pch, iSize,(dontRoute)? MSG_DONTROUTE : 0, sa, sizeof(SOCKADDR));

	//return 0;
}

int Socket::recvDatagram(char* pch,const int iSize, SocketAddress* sa)
{
	fd_set fd={ 1, sock};
	socklen_t size=sizeof(SocketAddress);
	int res=::select( 0, &fd, NULL, NULL, 0);

	if( (res!=SOCKET_ERROR) && (res>0) )
	{
		res= ::recvfrom( sock, pch, iSize, 0, (struct sockaddr*)sa, &size);
	}

	return res;
}

//static
int Socket::close(SOCKET& s)
{ 
	if( !s || (s==INVALID_SOCKET) )
		return 0;

	int ret;

#ifdef WIN32
	ret=::closesocket(s);
#else  /* def WIN32 */
	ret=::close(s);
#endif /* def WIN32 */

	s=NULL;

	return ret;
}

//static
SocketAddress	Socket::gethostbyname(const std::string& name, const unsigned short port)
{ 
	addrinfo* pai = NULL;
	char portStr[6]; _itoa_s( port, portStr, 10 );
	addrinfo hints = { 0 };
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	if( 0 == getaddrinfo( name.c_str(), portStr, &hints, &pai ) && pai )
	{
		sockaddr_in sTmp;
		sTmp.sin_family = AF_INET;
		sTmp.sin_port = ::htons( port );
		#pragma warning( push )
		#pragma warning( disable : 4311 )
		sTmp.sin_addr.s_addr = (ULONG)pai->ai_addr;
		#pragma warning( pop )
		freeaddrinfo( pai );
		return sTmp;
	}
	else
	{
		sockaddr a;
		a.sa_family = AF_UNSPEC;
		return a;
	}
}

//static
std::string Socket::gethostname()
{
	std::string s;
	int len = ::gethostname( NULL, 0 );
	if( 0 < len )
	{
		s.resize( len + 1, 0 );
		::gethostname( &s[0], len + 1 );
	}
	return s;
}

//static
std::vector<in_addr> Socket::getLocalIPList()
{
	std::vector<in_addr> l;
	static const in_addr lh = {127,0,0,1};

	std::string s( 256, 0 );
	if( 0 == ::gethostname( &s[0], 256 ) )
	{
		addrinfo* pai = NULL;
		char portStr[6]; _itoa_s( 80, portStr, 10 );
		addrinfo hints = { 0 };
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		if( 0 == getaddrinfo( s.c_str(), portStr, &hints, &pai ) && pai )
		{
			for( addrinfo* p = pai; p; p = p->ai_next )
			{
				if( AF_INET == p->ai_family && 4 == p->ai_addrlen )
				{
					in_addr a;
					#pragma warning( push )
					#pragma warning( disable : 4311 )
					a.s_addr = (ULONG)p->ai_addr;
					#pragma warning( pop )
					l.push_back( a );
				}
			}
			freeaddrinfo( pai );
		}
	}
	l.push_back( lh );
	return l;
}

//static
std::string	Socket::gethostbyaddr(const SocketAddress& sa)
{ 
	char name[1096];
	if( SOCKET_ERROR == getnameinfo((SOCKADDR*)&sa.sin_addr,sizeof(sa.sin_addr), name, 1096, NULL, 0, NI_NUMERICSERV ) )
		return std::string(); // returning an empty string
 	return name;
}

//static
u_int64	Socket::ntoh(u_int64   netlong)
{
	if(htons(1)==1)
	{
		return netlong;
	} 
	else 
	{
		char* c=reinterpret_cast<char*>(&netlong);
		char c2[8];
		c2[0]=c[7];
		c2[1]=c[6];
		c2[2]=c[5];
		c2[3]=c[4];
		c2[4]=c[3];
		c2[5]=c[2];
		c2[6]=c[1];
		c2[7]=c[0];
		return reinterpret_cast<u_int64&>(*c2);
	}
} 

//static
u_int64	Socket::hton(u_int64   hostlong)
{
	if(htons(1)==1) 
	{
		return hostlong;
	} 
	else 
	{
		char* c=reinterpret_cast<char*>(&hostlong);
		char c2[8];
		c2[0]=c[7];
		c2[1]=c[6];
		c2[2]=c[5];
		c2[3]=c[4];
		c2[4]=c[3];
		c2[5]=c[2];
		c2[6]=c[1];
		c2[7]=c[0];
		return reinterpret_cast<u_int64&>(*c2);
	}

}

///////////////////////////////////////////////////////////////////////////////////////////

SockIn::SockIn( int protocol, SocketAddress& sa )
{
	if( IPPROTO_TCP == protocol )
	{
		create( SOCK_STREAM, false, IPPROTO_TCP, true, 0, 0 );
	}
	else if( IPPROTO_UDP == protocol )
	{
		create( SOCK_DGRAM, true, IPPROTO_UDP, false, 0, 0 );
	}
	else
		throw -1;
	bind( sa );
}

SockIn::~SockIn()
{
	if( 0 != sock ) close();	
}

///////////////////////////////////////////////////////////////////////////////////////////

UDPListener::UDPListener( SocketAddress& sa )
	: SockIn( IPPROTO_UDP, sa )
	, m_szRcvBuff( 0 ) 
{
	getRecvBuffSize( m_szRcvBuff );
	char buff[20];
	logStr( 1, "INFO: New UDPListener(%s:%hu).\n", sa.getDottedDecimal(buff), sa.getPort() );
}

int UDPListener::processRead( Server* pServer )
{
	DataPackage a( std::string( m_szRcvBuff, 0 ), SocketAddress() );
	int size=(int)sizeof(SocketAddress);
	int r = ::recvfrom( sock, &a.first[0], (int)a.first.size(), 0, (struct sockaddr*)&a.second, &size );
	if( 0 < r )
	{
		a.first.resize(r);
		m_received.push( a );
		return cbRead( pServer );
	}
	else
		return cbError( pServer, r );
}

///////////////////////////////////////////////////////////////////////////////////////////

TCPListener::TCPListener(SocketAddress& sa)
	: SockIn(IPPROTO_TCP, sa)
{
	listen();
	char buff[20];
	logStr( 1, "INFO: New TCPListener(%s:%hu).\n", sa.getDottedDecimal(buff), sa.getPort() );
}

int TCPListener::processRead( Server* pServer )
{
	Peer peer;
	peer.pData = NULL;
	peer.s = new TCPConnection( Socket::accept(peer.sa), peer.sa, this, pServer );
	if( 0 < *peer.s )
	{
		m_peers.push_back( peer );
		cbRead( pServer );
		pServer->addReceiver( m_peers.back().s );
		return 0;
	}
	else
		cbError( pServer, (int)(SOCKET)*peer.s );
	return SOCKET_ERROR;
}

int TCPListener::getPeerIndex( TCPConnection const* pC ) const
{
	for( PeerList::const_iterator it = m_peers.begin(); it != m_peers.end(); it++ )
	{
		if( pC == it->s.ptr )
			return (int)(it - m_peers.begin());
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////

TCPConnection::TCPConnection( Socket const& other, SocketAddress const& peerAddress, TCPListener* pL, Server* pServer ) 
: SockIn( other )
, m_iRcvBuffSize(0)
, m_iSndBuffSize( 0 )
, m_iReadOffs(0)
, m_peerAddr( peerAddress )
, m_pListener( pL )
, m_pServer( pServer )
, m_bPendingSend( false )
, m_iReadSize(0)
, m_sto()
{
	getRecvBuffSize( m_iRcvBuffSize );
	getSendBuffSize( m_iSndBuffSize );
	m_sto.tv_sec = 0;
	m_sto.tv_usec = 16000;
	char buff[20];
	logStr( 2, "Info: TCPConnection accepted at server from %s:%hu.\n\tBuffers in:%i out:%i.\n", peerAddress.getDottedDecimal(buff), peerAddress.getPort(), m_iRcvBuffSize, m_iSndBuffSize );
}

TCPConnection::TCPConnection( SocketAddress connectTo ) // client connection
: SockIn( IPPROTO_TCP, connectTo )
, m_iRcvBuffSize( 0 )
, m_iSndBuffSize( 0 )
, m_iReadOffs(0)
, m_peerAddr( connectTo )
, m_pListener( NULL )
, m_pServer( NULL )
, m_bPendingSend( false )
, m_iReadSize(0)
, m_sto()
{
	char buff[20];
	if( 0 != connect( connectTo ) )
	{
		logStr( 0, "Error: TCPConnection client failed to connect to server at %s:%hu.. Reason %i.\n", connectTo.getDottedDecimal(buff), connectTo.getPort(), lastNetError );
		return;
	}
	getRecvBuffSize( m_iRcvBuffSize );
	getSendBuffSize( m_iSndBuffSize );
	m_sto.tv_sec = 0;
	m_sto.tv_usec = 16000;
	logStr( 2, "Info: TCPConnection client connected to server at %s:%hu as %08x.\n\tBuffers in:%i out:%i.\n", connectTo.getDottedDecimal(buff), connectTo.getPort(), (SOCKET)*this, m_iRcvBuffSize, m_iSndBuffSize );
}

TCPConnection::~TCPConnection()
{
	if( *this )
		logStr( 2, "Info: TCPConnection %08x closed.\n", (SOCKET)*this );
}

int TCPConnection::write( char const* buff, int size )
{
	int r = 0;
	VWB_LockedStatement( m_mtxRWOut )
	//if( VWB_MutexLock __l = VWB_MutexLock( m_mtxRWOut ) )
	{
		while( size > m_iSndBuffSize )
		{
			m_strOut.push_back( std::string( buff, m_iSndBuffSize ) );
			size-= m_iSndBuffSize;
			m_iReadSize+= m_iSndBuffSize;
			buff+= m_iSndBuffSize;
		}
		if( 0 != size )
		{
			m_strOut.push_back( std::string( buff, size ) );
		}
		if( !m_strOut.empty() )
		{
			r = sendDirect( &m_strOut.front()[0], (int)m_strOut.front().size() );
			m_strOut.pop_front();
			if( m_pServer )
				m_pServer->addResponder( *this );
			m_bPendingSend = true;
		}
	}
	return r;
}

int TCPConnection::read( char* buff, int szBuff, int size )
{
	int r = 0;
	if( szBuff < size )
		return SOCKET_ERROR;
	VWB_LockedStatement( m_mtxRWIn )
	{
		while( !m_strIn.empty() && 0 < m_strIn.front().size() - m_iReadOffs )
		{
			if( 0 == size || 0 == szBuff )
				break;
			int ts = size > szBuff ? szBuff : size; // this is what fits into the string
			int tr = ((int)m_strIn.front().size()) - m_iReadOffs; // this is what will be read in this run
			if( tr <= ts ) 
			{ // all fits in
				memcpy( buff, &m_strIn.front()[m_iReadOffs], tr );
				m_iReadOffs = 0;
				m_strIn.pop_front();
				size-= tr;
				szBuff-= tr;
				r+= tr;
				buff+= tr;
			}
			else
			{ // only part fits in
				memcpy( buff, &m_strIn.front()[m_iReadOffs], ts );
				m_iReadOffs+= ts;
				r+= ts;
				buff+= ts;
				break;
			}
		}
		m_iReadSize-= r;
	}
	return r;
}

int TCPConnection::readUntil( char* buff, int iBuffSize, const char* szDelimiter )
{
	int r = 0;
	if( NULL == szDelimiter || 0 == szDelimiter[0] )
		szDelimiter = "\015\012";
	const int iDelSize = (int)strlen( szDelimiter );

	VWB_LockedStatement( m_mtxRWIn )
	{
		auto it = m_strIn.begin();
		char* pL = NULL;
		int nn = 0;
		if( it != m_strIn.end() )
		{
			char const* b = &(*it)[m_iReadOffs];
			char const* bE = b + (int)(it->size() - m_iReadOffs ); 
			for(; b < bE; b++ )
				if( *b == szDelimiter[nn] )
				{
					if( iDelSize == ++nn )
						break;
				}
				else
					nn = 0;

			if( iDelSize == nn )
			{
				r+= (int)(b - &(*it)[m_iReadOffs] ) + 1 - iDelSize;
			}
			else
				r+= (int)it->size()-m_iReadOffs;
		}
		int c = 0;
		if( iDelSize != nn )
		{
			for( it++; it != m_strIn.end(); it++ )
			{
				char const* b = &(*it)[0];
				char const* bE = b + (int)it->size(); 
				for(; b < bE; b++ )
					if( *b == szDelimiter[nn] )
					{
						if( iDelSize == ++nn )
							break;
					}
					else
						nn = 0;
				if( iDelSize == nn )
				{
					c = (int)( b - &(*it)[0] ) + 1 - iDelSize;
					r+= c;
					break;
				}
				r+= (int)it->size();
			}
		}

		if( m_strIn.end() == it )
			return 0; // not found

		if( NULL == buff )
			return r + 1;

		if( r > iBuffSize )
			return SOCKET_ERROR;

		int t = (int)(m_strIn.front().size() - m_iReadOffs);
		int o = r;
		while( t < o )
		{
			memcpy( buff, &m_strIn.front()[m_iReadOffs], t );
			o-= t;
			buff+= t;
			m_strIn.pop_front();
			m_iReadOffs = 0;
			m_iReadSize-= t;
			t = (int)m_strIn.front().size();
		}
		if( 0 != o )
		{
			memcpy( buff, &m_strIn.front()[m_iReadOffs], o );
		}
		buff[o] = 0;
		m_iReadOffs+= o + iDelSize; // this is ok as we found the delimiter here
		m_iReadSize-= o + iDelSize;
		if( m_iReadOffs > (int)m_strIn.front().size() ) // if it exceeded current buffer, we move onward
		{
			m_iReadOffs-= (int)m_strIn.size();
			m_strIn.pop_front();
		}
	}
	return r;
}


int TCPConnection::cmpFront( char const* str ) const
{
	VWB_LockedStatement( m_mtxRWIn )
	{
		if( strlen( str ) <= (m_strIn.front().size() - m_iReadOffs) )
		{
			return strcmp( str, &m_strIn.front()[m_iReadOffs] );
		}
	}
	return SOCKET_ERROR;
}

int TCPConnection::processAsClient()
{
	int n = 0;
	timeval to = m_sto;
	FD_SET r;FD_ZERO(&r);
	FD_SET( *this, &r );
	FD_SET w;FD_ZERO(&w);
	if( m_bPendingSend )
		FD_SET( *this, &w );
	FD_SET e;FD_ZERO(&e);
	n = ::select( 1, &r, &w, &e, &to );
	if( 0 < n )
	{
		if( FD_ISSET( *this, &r ) )
			n = processRead( NULL );
		if( FD_ISSET( *this, &w ) )
			n = processWrite( NULL );
		if( FD_ISSET( *this, &w ) )
			n = processError( NULL );
	}
	if( SOCKET_ERROR == n )
	{
		int err = lastNetError;
		logStr( 0, "Client: Socket Error %i.\n", err );
		close();
	}
	return n;
}

int TCPConnection::processRead( Server* pServer )
{
	if( std::numeric_limits<int>::max() - m_iRcvBuffSize < m_iReadSize )
	{
		logStr( 0, "Error: Receiver buffer of socket %08x full.\nClosing connection.\n", (SOCKET)*this );
		return -1;
	}
	std::string s( m_iRcvBuffSize, 0 );
	int r = recvDirect( &s[0], m_iRcvBuffSize );
	if( 0 < r )
	{
		VWB_LockedStatement( m_mtxRWIn )
		{
			s.resize( r );
			m_strIn.push_back( s );
			m_iReadSize+= r;
			r = cbRead( pServer );
			return r;
		}
		r = SOCKET_ERROR;
		logStr( 0, "Error: Connection socket %08x read mutex inaccessible.\n", (SOCKET)*this );
	}

	logStr( 1, "Closing socket %08x reason %i.\n", (SOCKET)*this, r );
	r = cbError( pServer, r );
	if( m_pListener )
	{
		int i = m_pListener->getPeerIndex( this );
		if( 0 <= i )
		{
			if( pServer )
				pServer->removeReceiver( m_pListener->getPeer(i).s );
			m_pListener->closePeer(i);
		}
	}
	return r;
}

int TCPConnection::processWrite( Server* pServer )
{
	int r = 0;
	VWB_LockedStatement( m_mtxRWOut )
	{
		if( m_strOut.empty() )
		{
			m_bPendingSend = false;
			if( pServer )
				pServer->removeResponder( *this );
		}
		else 
		{
			r = sendDirect( &m_strOut.front()[0], (int)m_strOut.front().size() );
			m_strOut.pop_front();
		}
		return r;
	}

	r = SOCKET_ERROR;
	logStr( 0, "Error: Connection socket %08x read write inaccessible.\n", (SOCKET)*this );
	logStr( 1, "Closing socket %08x reason %i.\n", (SOCKET)*this, r );
	return r;
}

int TCPConnection::processError( Server* pServer )
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////

HttpRequest::HttpRequest( TCPConnection& conn )
: state( STATE_UNDEF ), type( TYPE_UNDEF ), enctype( ENCTYPE_UNDEF ) 
{
	parseRequest( conn );
};

bool HttpRequest::readFrom( TCPConnection& conn )
{
	*this = HttpRequest(); // empty
	return STATE_ERROR != parseRequest( conn );
};

bool HttpRequest::writeTo( TCPConnection& conn )
{
	std::stringstream s( std::ios_base::binary );
	if( postData.empty() ) // GET header
		s << "GET ";
	else // POST header
	{
		s << "POST ";
		bound = "-----------------------------VIOSOSPCALIBRATOR";
	}
	if( request.empty() )
		s << "/";
	else
		s << request;
	if( !getData.empty() )
	{
		s << "?";
		ParamMap::const_iterator param = getData.begin();
		s << param->first;
		if( !param->second.empty() )
			s << ":" << param->second;
		for( ; param != getData.end(); param++ )
		{
			s << "&" << param->first;
			if( !param->second.empty() )
				s << ":" << param->second;
		}
	}
	s << " HTTP/1.1\015\012";
	for( ParamMap::const_iterator header = headers.begin(); header != headers.end(); header++ )
	{
		s << header->first;
		if( !header->second.empty() )
			s << ": " << header->second;
		s << "\015\012";
	}
	// calculate content size, if POST
	
	if( !postData.empty() )
	{
		s << "Content-Type: multipart/form-data; boundary=" << bound << "\015\012";
		std::stringstream ss;
		for( PostParamMap::const_iterator it = postData.begin(); it != postData.end(); it++ )
		{
			ss << "--" << bound << "\015\012";
			ss << "Content-Disposition: form-data; name=\"" << it->first << "\"";
			if( !it->second.file.empty() )
			{
				ss << "; filename = \"" << it->second.value << "\"";
			}
			ss << "\015\012";
			if( it->second.file.empty() )
			{
				ss << "Content-Type: text/plain" << "\015\012";
				ss << "\015\012";
				ss << it->second.value << "\015\012";
			}
			else
			{
				switch( it->second.encType )
				{
				case ENCTYPE_UNDEF:
					ss << "Content-Type: application/octet-stream" << "\015\012";
					break;
				case ENCTYPE_URL:
					ss << "Content-Type: application/x-url-encoded" << "\015\012";
					break;
				case ENCTYPE_MULTI: // actually an error
				case ENCTYPE_OTHER:
					ss << "Content-Type: application/octet-stream" << "\015\012";
					break;
				}
				ss << "\015\012";
				ss << it->second.file;
				ss << "\015\012";
			}
		}
		ss << "--" << bound << "--" << "\015\012";
		s << "Content-Length: " << ss.str().size() << "\015\012";
		s << "\015\012";
		s << ss.str();
	}

	conn.write( s.str().c_str(), (int)s.str().size() );
	return false;
}

HttpRequest::STATE HttpRequest::parseRequest( TCPConnection& conn )
{
	char* pB = NULL;
	int nB = 0;
	switch( state )
	{
	case STATE_UNDEF:
		{
			if( conn.cmpFront( "GET " ) || conn.cmpFront( "POST" ) )
			{
				state = STATE_INIT;
			}
			else
			{
				state = STATE_ERROR;
				break;
			}
		}
	case STATE_INIT:
		{
			nB = conn.getNumRead();
			pB = new char[ nB ];
			int n = conn.readUntil( pB, nB, "\015\012\015\012" );
			if( 0 < n )
			{
				if( 0 < parseHttpHeader( pB, type, request, headers ) )
					state = STATE_HEADER;
				else
				{
					state = STATE_ERROR;
					break;
				}
			}
			else if( 0 > n )
			{
				state = STATE_ERROR;
				break;
			}
			else
				break;
		}
	case STATE_HEADER:
		{
			parseURL( request, request, getData );
			// read content length, this is a must on post requests
			if( TYPE_POST == type )
			{
				auto it = headers.find("Content-Type");
				if( it != headers.end() )
				{
					if( 0 == it->second.compare( "application/x-www-form-urlencoded" ) )
						enctype = ENCTYPE_URL;
					else if( 0 == it->second.compare( 0, 19, "multipart/form-data" ) )
					{
						enctype = ENCTYPE_MULTI;
						bound = std::string("--");
						bound.append( it->second.substr( 30 ) );
					}
					else // other
					{
						enctype = ENCTYPE_OTHER;
						bound = it->second.substr( 0, it->second.find( ';' ) );
					}
					headers.erase( it );
				}
				else
					enctype = ENCTYPE_URL;
				it = headers.find( "Content-Length" );
				if( it != headers.end() )
				{
					contentLength = atoi( it->second.c_str() );
					if( 0 < contentLength )
					{
						state = STATE_CONTENTLENGTH;
					}
					else
					{
						state = STATE_ERROR;
						break;
					}
					headers.erase( it ); // need to remove from headers after parsed
				}
				else
				{
					state = STATE_ERROR;
					break;
				}
			}
			else
			{
				state = STATE_PARSED;
				break;
			}
		}
	case STATE_CONTENTLENGTH:
		{
			if( contentLength > conn.getNumRead() )
				break;

			body.resize( size_t(contentLength) + 1 );

			if( contentLength == conn.read( &body[0], contentLength + 1, contentLength ) )
				state = STATE_BODY;
			else
				state = STATE_ERROR;
		}
	case STATE_BODY:
		{
			int i;
			if( ENCTYPE_URL == enctype )
				i = parseURLencBody( body, postData );
			else
				i = parseMultipartBody( body, bound, postData );
			if( 0 < i )
				state = STATE_PARSED;
			else if( 0 == i )
				break;
			else
			{
				state = STATE_ERROR;
				break;
			}
		}
	case STATE_PARSED:
		break;
	}
	if( pB )
		delete[] pB;
	return state;
}


int HttpRequest::parseURL( std::string const& request, std::string& site, ParamMap& getData )
{
	if( request.empty() )
		return SOCKET_ERROR;

	std::string::size_type pos = request.find( '?' );
	if( std::string::npos != pos )
	{
		std::string::size_type pos1 = pos+1;
		while( 1 )
		{
			std::string::size_type pos2 = request.find( '&', pos1 );
			std::string::size_type pos3 = request.find( '=', pos1 );
			if( std::string::npos == pos3 )
				break;
			getData[std::string( request, pos1, pos3 - pos1 )] = std::string( request, pos3, std::string::npos == pos2 ? std::string::npos : pos2 - pos3 );
			if( std::string::npos == pos2 )
				break;
			pos1 = pos2 + 1;
		}
		site = request.substr( 0, pos - 1 );
		return 1 + (int)getData.size();
 	}
	else
		site = request;
	return 1;
}

int HttpRequest::parseHttpHeader( char const* szIn, TYPE& type, std::string& request, ParamMap& headers )
{
	if( NULL == szIn )
		return -1;
	char const* p = szIn;
	char const* pE = strstr( p, "\015\012" );
	if( NULL == pE )
		return SOCKET_ERROR;
	request = std::string( p, pE - p );

	std::string::size_type po1 = request.find( ' ', 5 ); // this is the space before protocol, this is mandatory but does not change change processing
	if( std::string::npos == po1 )
		return SOCKET_ERROR;

	if( 0 == request.compare( 0, 4, "GET " ) )
	{
		type = TYPE_GET;
		request = request.substr( 4, po1 - 4 );
	}
	else if( 0 == request.compare( 0, 5, "POST " ) )
	{
		type = TYPE_POST;
		request = request.substr( 5, po1 - 5 );
	}
	else
		return SOCKET_ERROR;

	// read header lines
	p = pE + 2;
	parseHeader( p, headers );
	return 1 + (int)headers.size();
}

int HttpRequest::parseHeader( char const* p, ParamMap& headers )
{
	char const* pS = p;
	char const* pE = NULL;
	while( *p && NULL != ( pE = strstr( p,  "\015\012" ) ) )
	{
		std::string s( p, pE - p );
		if( s.empty() )
			return (int)(p - pS) + 2;
		std::string::size_type pos = s.find( ':' );
		if( std::string::npos != pos )
		{
			headers[ std::string( p, pos ) ] = std::string( s, pos+2, std::string::npos );
		}
		else
			headers[ std::string( p ) ] = "";
		p = pE + 2;
	}
	return SOCKET_ERROR;
}

int HttpRequest::parseHeaderLine( char const* p, ParamMap& params )
{
	char const* pE = NULL;
	while( p )
	{
		pE = strstr( p,  "; " );

		std::string s;
		if( pE )
		{
			s = std::string( p, pE - p );
			pE+= 2;
		}
		else
			s = p;

		std::string::size_type pos = s.find( '=' );
		if( std::string::npos != pos )
		{
			if( '"' == s[pos+1] )
			{
				params[ std::string( p, pos ) ] = std::string( s, pos+2, -1 );
				params[ std::string( p, pos ) ].pop_back();
			}
			else
				params[ std::string( p, pos ) ] = std::string( s, pos+1, -1 );
		}
		else
		{
			params[ s ] = "";
		}
		p = pE;
	}

	return (int)params.size();
}

int HttpRequest::parseURLencBody( std::string const& body, PostParamMap& postData )
{
	if( body.empty() )
		return SOCKET_ERROR;

	std::string::size_type pos1 = 0;
	while( 1 )
	{
		std::string::size_type pos2 = body.find( '&', pos1 );
		std::string::size_type pos3 = body.find( '=', pos1 );
		if( std::string::npos == pos3 )
			break;
		PostValue v = { ENCTYPE_URL, std::string( body, pos3 + 1, std::string::npos == pos2 ? std::string::npos : pos2 - pos3 - 1 ), "" };
		postData[std::string( body, pos1, pos3 - pos1 )] = v;
		if( std::string::npos == pos2 )
			break;
		pos1 = pos2 + 1;
	}
	return (int)postData.size();
}

int HttpRequest::parseMultipartBody( std::string const& body, std::string bound, PostParamMap& postData )
{
	if( bound.length() * 2 + 10 > body.size() )
		return SOCKET_ERROR;
	std::string::size_type lBound = bound.length();
	std::string::size_type found = std::string::npos;

	// must start with bound
	if( 0 == body.compare( 0, lBound, bound ) )
	{
		std::string::size_type offs = lBound + 2; // step over first boundary line feed
		std::string b2( "\015\012" ); b2.append( bound ); lBound+= 2;
		while( std::string::npos != ( found = body.find( b2, offs ) ) )
		{
			// next boundary found; inbetween there is content
			std::string blob = body.substr( offs, found - offs );
			offs = found + lBound + 2;

			// parse blob
			ParamMap headers;
			int datastart = parseHeader( blob.c_str(), headers );
			if( SOCKET_ERROR != datastart )
			{
				//std::string cntt = "ContentDissition: form-data; ";
				ParamMap::iterator itCD = headers.find( "Content-Disposition" );
				ParamMap::iterator itCT = headers.find( "Content-Type" );
				if( headers.end() != itCD && headers.end() != itCT )
				{
					ParamMap params;
					parseHeaderLine( itCD->second.c_str(), params );
					ParamMap::iterator itName = params.find( "name" );
					ParamMap::iterator itFile = params.find( "filename" );
					if( params.end() != itName  )
					{
						PostValue& v = postData[itName->second];
						v.encType = ENCTYPE_OTHER;
						v.value = blob.substr( datastart, std::string::npos );
						if( params.end() != itFile )
						{
							v.file.swap( itFile->second );
						}
					}
				}
			}

			if( '-' == body[offs] )
				return int(offs + 4); // finished NOTE: conversion to INT is sane, as the content size cannot exceed 2GB
			else 
				return SOCKET_ERROR;
		}
	}
	return SOCKET_ERROR; // did not find final boundary, this is an error

/*
	while( std::string::npos != ( found = body.find( "\015\012", offs ) ) )
	{
		std::string s( body, offs, found - offs );
		if(  ) // first
		{
			offs = found + 2;
			int ct = 0;
			int enc = ENCTYPE_UNDEF;
			while( std::string::npos != ( found = body.find( "\015\012", offs ) ) )
			{
				std::string l( body, offs, found - offs );
				offs = found + 2;
			}
			if( 0 == ct && ENCTYPE_UNDEF == enc )
				break;
		}
		else if( 0 == s.compare( 0, bound.length(), bound ) ) // subsequent
		{
			if( s.length() == bound.length() + 2 && '-' == s[bound.length()] && '-' == s[bound.length()+1] ) // last
			{
				// terminate
			}
			else
			{ // inbetween
			}
		}
		else
		{
			std::string::size_type p1 = s.find( '=' );
			if( std::string::npos != p1 )
			{
				std::string& data = postData[ std::string( s, p1 ) ] = std::string( s, p1+2, -1 );
				std::string::size_type f2 = data.find( "file" );
				if( std::string::npos != f2 )
				{
					// get file...
					std::string fname;
					files[ fname ] = std::string( );
				}
			}
		}
		offs = found;
	}
*/
	return (int)postData.size();
}

///////////////////////////////////////////////////////////////////////////////////////////

Server::Server() 
: m_modalState(0)
{
	ifStartSockets() 
	{
		m_sto.tv_sec = 0;
		m_sto.tv_usec = 16000;
		FD_ZERO( &m_readers );
		FD_ZERO( &m_writers );
	}
};

Server::Server( SPtr<SockIn> const& s, bool bRunModal )
: m_modalState(0)
{
	ifStartSockets()
	{
		m_sto.tv_sec = 0;
		m_sto.tv_usec = 16000;
		FD_ZERO( &m_readers );
		FD_ZERO( &m_writers );
		addReceiver( s );
		if( bRunModal )
			doModal();
	}
}
Server::~Server()
{
	VWB_LockedStatement( m_mtxGlobal )
	{
		m_listeners.clear();
	}
	if( m_thread.joinable() )
	{
		m_modalState = 0;
		m_thread.join();
	}
	closeSockets();
}

int Server::addReceiver( SPtr<SockIn> s )
{
	if( s )
	{
		VWB_LockedStatement( m_mtxGlobal )
		{
			m_listeners.push_back(s);
			FD_SET( *s, &m_readers );
			logStr( 2, "INFO: Server added socket %08X. Server now listens to %u connections.\n", (SOCKET)*s, (u_int)m_listeners.size() );
			return 0;
		}
	}
	logStr( 0, "Server::add receiver error.\n" );
	return SOCKET_ERROR;
}

int Server::removeReceiver( SPtr<SockIn> s )
{
	if( s )
	{
		VWB_LockedStatement( m_mtxGlobal )
		{
			for( Listeners::iterator it = m_listeners.begin(); it != m_listeners.end(); it++ )
				if( *it == s )
				{
					FD_CLR( **it, &m_readers );
					m_listeners.erase( it );
					logStr( 2, "Server removed socket %08X. Server now listens to %u connections.\n", (SOCKET)*s, (u_int)m_listeners.size() );
					return 0;
				}
		}
	}
	logStr( 0, "Server::remove receiver error.\n" );
	return SOCKET_ERROR;
}

int Server::addResponder( Socket& s )
{
	if( s )
	{
		VWB_LockedStatement( m_mtxGlobal )
		{
			FD_SET( s, &m_writers );
			return 0;
		}
	}
	logStr( 0, "Server::add responder error.\n" );
	return SOCKET_ERROR;
}

int Server::removeResponder( Socket& s )
{
	if( s )
	{
		VWB_LockedStatement( m_mtxGlobal )
		{
			FD_CLR( s, &m_writers );
			return 0;
		}
	}
	logStr( 0, "Server::remove responder error.\n" );
	return SOCKET_ERROR;
}

int Server::doModalStep()
{
	int n = 0;
	timeval to = m_sto;
	FD_SET r;
	FD_SET w;
	VWB_LockedStatement( m_mtxGlobal )
	{
		if( 0 != m_readers.fd_count ) // there can't be any writers without readers in TCP
		{
			r = m_readers;
			w = m_writers;
			Listeners tmp = m_listeners;
			n = ::select( (int)m_listeners.size(), &r, &w, NULL, &to );
			if( 0 < n )
			{
				// handle readers
				for( Listeners::iterator it = tmp.begin(); it != tmp.end(); it++ )
				{
					if( FD_ISSET( **it, &r ) )
					{
						n = (*it)->processRead( this );
					}
				}
				// handle writers
				for( Listeners::iterator it = tmp.begin(); it != tmp.end(); it++ )
				{
					if( FD_ISSET( **it, &w ) )
					{
						(*it)->processWrite( this ); // handle all ready writing sockets
					}
				}
			}
			else if( SOCKET_ERROR == n )
			{
				int err = lastNetError;
				logStr( 0, "Server: Socket Error %i. Server is stopping.\n", err );
			}
		}
		else if( MODALSTATE_RUN == m_modalState )
		{ // idle
			sleep(m_sto.tv_usec/1000);
		}
	}
	else
	{
		n = -2;
		logStr( 0, "Server: Could not aquire mutex. Server is stopping.\n" );
	}
	return n;
}

int Server::endModal( int code )
{
	if( MODALSTATE_RUN == m_modalState )
	{
		m_modalState = code;
		if( m_thread.joinable() )
		{
			m_thread.join();
		}
	}
	return m_modalState;
}

int Server::doModal()
{
	if( m_thread.joinable() )
	{
		if( MODALSTATE_RUN == m_modalState )
		{
			return -2;
		}
		else
		{
			m_thread.join();
		}
	}
	m_thread.swap( std::thread( Server::_theadFn, this ) );
	if( m_thread.joinable() )
		return 0;
	logStr( 0, "Server: FATAL ERROR cannot sart listener loop.\n" );
	return SOCKET_ERROR;
}

u_int Server::modalLoop()
{
	try
	{
		m_modalState = MODALSTATE_RUN;
		logStr( 1, "Server: Server starts listening.\n" );
		while( MODALSTATE_RUN == m_modalState )
		{
			if( 0 > doModalStep() )
				m_modalState = MODALSTATE_ERR;
		}
		logStr( 1, "Server: Server stops listening.\n" );
		return 0;
	}
	catch(...)
	{
		m_modalState = MODALSTATE_EXC;
		return SOCKET_ERROR;
	}
}

bool Server::isRunningModal()
{
	return MODALSTATE_RUN == m_modalState; 
}

void Server::_theadFn( void* param )
{
	if( NULL == param )
		return;
	//return
	((Server*)param)->modalLoop();
}
