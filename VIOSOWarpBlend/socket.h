#ifndef VWB_SOCKET_HPP
#define VWB_SOCKET_HPP

#include "Platform.h"
//#include "../Include/VWBTypes.h"
#include "SPtr.h"
#include <queue>
#include <list>
#include <map>
#include <string>

#if defined( WIN32 ) || defined( WIN64 )

	#define NOMINMAX
	#include <WS2tcpip.h>
	#include <winsock2.h>
	#undef NOMINMAX

	#pragma comment(lib, "ws2_32.lib")

	#define ERRINTR (WSAEINTR)
	#define lastNetError WSAGetLastError()
	typedef long suseconds_t;
	typedef int socklen_t;

#define ifStartSockets() 	for( WSADATA d = {0}; 0 == d.wVersion && 0 == WSAStartup(0x0002,&d); )
#define closeSockets WSACleanup

#else /* WIN32 */

	#include "sys/socket.h"
	#include "sys/types.h"
	#include "sys/param.h"
	#include "sys/time.h"
	#include "netinet/in.h"

	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR (-1)
	#define ERRINTR (10004)     //can native bsd socket's be interrupted?
	#define lastNetError errno

#define ifStartSockets() if(1)
#define closeSockets (0)
typedef struct WSADATA { int i; } WSADATA;

#endif /* WIN32 */

class Socket;
struct SocketAddress;
#define INFINITETIMEOUT 1.7976931348623158e+308

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------
//                               SocketAddress
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct SocketAddress : sockaddr_in 
{
// construction

	// use default constructor to create empty address
	SocketAddress()
	{
		sin_family = AF_INET;
		sin_port = 0;
		sin_addr.s_addr = INADDR_ANY;
	};

	// copy constructor
	SocketAddress(sockaddr const& sa)
	{
		memcpy(this, &sa, sizeof(sockaddr));
	}; 

	// construct object from sockaddr_in struct
	SocketAddress(sockaddr_in const& sin)
	{
		memcpy(this, &sin, sizeof(sockaddr_in));
	}; 

	// construct AF_INET address using UINT-address and port in host-byte-order.
	SocketAddress(unsigned long addr, unsigned short port);

	// construct AF_INET address using url string and port number in host byte order.
	SocketAddress(char const* url, unsigned short port = 0); 

// public methods

	// returns ip in dotted decimal format i.e. "10.1.1.1".
	char const*  getDottedDecimal( char* sz, size_t nsz ) const
	{
		return inet_ntop(sin_family, (VWB_inet_ntop_cast)&sin_addr, sz, nsz);
	}; 
	template< size_t _sz >
	char const*  getDottedDecimal(char(&sz)[_sz]) const { return getDottedDecimal(sz, _sz); }

	// gets / sets port number in host byte order
	unsigned short	getPort() const;
	void			setPort(unsigned short port);

	// resets address to zero
	void zero() { memset(this,0,sizeof(SocketAddress)); }

// public operators for conversion

	operator sockaddr() const
	{
		return *((sockaddr*) this);
	}

	operator sockaddr*() const
	{
		return (sockaddr*) this;
	}

	operator sockaddr*()
	{
		return (sockaddr*) this;
	}

	operator sockaddr&() const
	{
		return *((sockaddr*) this);
	}

	operator const sockaddr&() const
	{
		return (const sockaddr&)*((sockaddr*) this);
	}

	operator sockaddr_in() 
	{
		return *((sockaddr_in*) this);
	}

	operator sockaddr_in*() 
	{
		return (sockaddr_in*) this;
	}

	operator sockaddr_in&() 
	{
		return *((sockaddr_in*) this);
	}

// public asignement and compare operators

	const SocketAddress& operator=(const sockaddr& sa);

	const SocketAddress& operator=(const sockaddr_in& sin);

	bool operator==(const SocketAddress& sa) const { return 0 == memcmp(this,&sa,sizeof(SocketAddress)); }

// public static methods

	// constructs a broadcast address to a specific port in host byte order.
	static SocketAddress broadcast(unsigned short port);
};

/**************** Socket *****************/

class Socket {

#ifdef WIN32

#	define ioctl IOCtl
#	define IFNAMSIZ 16

	struct ifreq 
	{
		char ifr_name[IFNAMSIZ];
		union {
			sockaddr  ifru_addr;
			sockaddr  ifru_dstaddr;
			char      ifru_oname[IFNAMSIZ];
			sockaddr  ifru_broadaddr;
			short     ifru_flags;
			int       ifru_metric;
			char      ifru_data[1];
			char      ifru_enaddr[6];
		};
	};

#endif /* WIN32 */

protected:
		
	SOCKET sock;

public:

//construction

	// construct empty socket
	Socket() { sock = 0; }

	// copy constructor s
	Socket(SOCKET const& s) { sock = s; }
	Socket(Socket const& s) { sock=s.sock; }

	// construct and create socket of type i.e. SOCK_DGRAM or SOCK_STREAM
	Socket(int type/*=SOCK_STREAM*/, bool broadcast/*=FALSE*/, int protocol/*=0*/, bool keepAlive/*=FALSE*/,u_int qRecvBuffer/*=0*/,u_int qSendBuffer/*=0*/)
	{
		if( SOCKET_ERROR == create( type, broadcast, protocol, keepAlive, qRecvBuffer, qSendBuffer) )
			throw SOCKET_ERROR; 
	};

// public methods

	// creates socket handle
	int create(int type/*=SOCK_STREAM*/, bool broadcast/*=FALSE*/, int protocol/*=0*/, bool keepAlive/*=FALSE*/,u_int qRecvBuffer/*=0*/,u_int qSendBuffer/*=0*/);

	// set size of receive buffer
	int setRecvBuff(int size);

	// get size of receive buffer
	int getRecvBuffSize(int& size);

	// get size of receive buffer
	int getSendBuffSize(int& size);

	// close socket
	int close(); 

	// bind socket to local address
	int bind(SocketAddress const& sa)
	{ 
		return ::bind(sock,sa,sizeof(sockaddr));
	};

	// listen on socket
	int listen(int qConnMax=0)
	{ 
		return ::listen( sock, (qConnMax)? qConnMax : SOMAXCONN);
	};

	// connect socket to peer address
	int connect(SocketAddress const& sa)
	{ 
		return ::connect(sock,sa,sizeof(sockaddr));
	};

	// accepts connections; socket needs to be stream oriented
	Socket accept()
	{ 
		return (SOCKET)::accept(sock,NULL,NULL);
	}; 

	// accepts connections; socket needs to be stream oriented
	Socket accept( SocketAddress& peerAddr )
	{ 
		int l = sizeof( SocketAddress );
		return (SOCKET)::accept(sock, (sockaddr*)&peerAddr, &l );
	}; 

	// check socket to be ready to read from, operation blocks
	int tryRead(const double timeout=INFINITETIMEOUT) const;

	// check socket to be ready to write to, operation blocks
	int tryWrite(const double timeout=INFINITETIMEOUT) const;

	// check for any exceptions on socket
	int test(const double timeout=INFINITETIMEOUT) const;

	// send some data to a conected socket, operation blocks
	int send(const char* pch, const int iSize, const double timeout=INFINITETIMEOUT); 

	// send some data to a concected socket direct, without select
	int sendDirect(const char* pch, const int iSize); 

	// receive some date from connected socket, operation blocks
	int recv(char* pch, const int iSize, const double timeout=INFINITETIMEOUT); 

	// receive some date from connected socket direct, witout select
	int recvDirect(char* pch, const int iSize); 

	// sends datagram to peer
	int sendDatagram(const char* buf, const int iSize, SocketAddress& sa, bool dontRoute = false);

	// receives datagram from socket, peer address will be stored in 'sa' if specified
	int recvDatagram(char* buf, const int iSize, SocketAddress* sa=NULL);

	// returns connected peer address
	SocketAddress getpeeraddr();

	// returns bound local address
	SocketAddress getsockaddr();
	int getsockaddr(sockaddr_in& addr,int& qAddr);

// public static methods

	// close socket
	static int close(SOCKET& s);

	// these are wrapper functions for os native operations derived from berkeley sockets
	static SocketAddress  gethostbyname(const std::string& name, const unsigned short port=0); 
	static std::string gethostname(); 
	static std::vector<in_addr> getLocalIPList(); 
	static std::string gethostbyaddr(const SocketAddress& sa); 
	static std::string getErrorMessage(int err);

	// the ntoh and hton functions are now polymorph
	static _inline_ u_short ntoh(u_short netshort) { return ::ntohs(netshort); } 
	static _inline_ u_short hton(u_short hostshort){ return ::htons(hostshort);} 
	static _inline_ u_int   ntoh(u_int   netint)   { return ::ntohl(netint);  } 
	static _inline_ u_int   hton(u_int   hostint)  { return ::htonl(hostint); } 
	static _inline_ uint64_t ntoh(uint64_t netlong); 
	static _inline_ uint64_t hton(uint64_t  hostlong);

	// public asignmet and compare operators
	const Socket& operator=(const Socket& s) { sock=s.sock; return *this; };
	const Socket& operator=(const SOCKET s) { sock=s;  return *this; };
	//void operator=(const Socket& s) { sock=s.sock; };
	//void operator=(const SOCKET s) { sock=s; };
	operator SOCKET() { return sock; };
	bool operator==(const Socket& s) const { return sock == s.sock; };
	bool operator==(const SOCKET s) const { return sock == s; };
	friend bool operator==(const SOCKET s, const Socket& ss) { return s == ss.sock; };
};

class UDPListener;
class TCPListener;
class TCPConnection;
class Server;

class SockIn : public Socket
{
	friend Server;
protected:
	SockIn( SockIn const& other ) : Socket( other ) {}
public:
	SockIn() : Socket() {}
	SockIn( Socket const& s ) : Socket( s ) {}
	SockIn( int protocol, SocketAddress& sa );
	virtual ~SockIn();
	Socket detach() { Socket s(sock); sock = 0; return s; }

	virtual int cbRead( Server* pServer ) {return 0;}
	virtual int cbWrite( Server* pServer ) {return 0;}
	virtual int cbError( Server* pServer, int err ) {return err;}
protected:
	// return 0 for ok but break, positive if ok and listened
	virtual int processRead( Server* pServer ) {return 1;}
	virtual int processWrite( Server* pServer ) {return 1;}
	virtual int processError( Server* pServer ) {return 1;}
};

class UDPListener : public SockIn
{
public:
	typedef std::pair< std::string, SocketAddress > DataPackage;
	typedef std::queue< DataPackage > RecvQueue;
protected:
	RecvQueue m_received;
	int m_szRcvBuff;

	UDPListener( UDPListener const& other ) : SockIn( other ), m_szRcvBuff(0) {}
	UDPListener() : SockIn(), m_szRcvBuff( 0 ) {}
public:

	DataPackage& front() { return m_received.front(); }
	int size() { (int)m_received.size(); }
	void pop() { m_received.pop(); }

	UDPListener( SocketAddress& sa );

private:
	virtual int processRead( Server* pServer );
};

class TCPListener : public SockIn
{
public:
	typedef struct Peer { void* pData; SocketAddress sa; SPtr<TCPConnection> s; } Peer;
	typedef std::vector< Peer > PeerList;
protected:
	PeerList m_peers;
	
	TCPListener( TCPListener const& other ) : SockIn( other ) {}
	TCPListener() : SockIn() {}
public:
	int size() { (int)m_peers.size(); }
	Peer& getPeer( int i ) { return m_peers[i]; }
	Peer const& getPeer( int i ) const { return m_peers[i]; }
	int getPeerIndex( TCPConnection const* pC ) const;
	void closePeer( int i ) { m_peers.erase( m_peers.begin() + i ); }

	TCPListener( SocketAddress& listenAt );

private:
	virtual int processRead( Server* pServer );
};

class TCPConnection : public SockIn
{
public:
	typedef std::list< std::string > DataStream;
protected:
	DataStream m_strIn;
	DataStream m_strOut;
	int m_iRcvBuffSize;
	int m_iSndBuffSize;
	int m_iReadOffs;
	int m_iReadSize;
	bool m_bPendingSend;
	SocketAddress m_peerAddr;
	TCPListener* m_pListener;
	Server* m_pServer;
	VWB_Mutex m_mtxRWIn;
	VWB_Mutex m_mtxRWOut;
	timeval m_sto;

	TCPConnection() {}
	TCPConnection( TCPConnection const& other ) {}
public:
	TCPConnection( Socket const& other, SocketAddress const& peerAddress, TCPListener* pL, Server* pServer );
	TCPConnection( SocketAddress connectTo );
	virtual ~TCPConnection();

	int write( char* buff, int size );

	// returns number of read chars, obtain available number of chars in stream from getNumRead()
	// returns SOCKET_ERROR if buffer too small
	int read( char* buff, int iBuffSize, int size );

	// @param buff			the buffer to read into
	// @param iBuffSize		the size of the bufferto read until
	// @param szDelimiter	optional zero terminated string, set to network new-line "\015\012" 
	// if buff is NULL, it returns the needed size including terminating zero char
	// returns number of read chars
	// returns 0 if no line is found or an empty line is read
	// returns SOCKET_ERROR if buffer too small
	int readUntil( char* buff, int iBuffSize, char const* szDelimiter = NULL ); // Adds zero termination to string

	int cmpFront( char const* str ) const;
	int processAsClient();

	bool isWritePending() const { return m_bPendingSend; }
	int getNumRead() const { return m_iReadSize; }
	int getCurrRcvBuffSize() const { return m_iRcvBuffSize; }
	int getCurrSndBuffSize() const { return m_iSndBuffSize; }
private:
	virtual int processRead( Server* pServer );
	virtual int processWrite( Server* pServer );
	virtual int processError( Server* pServer );
};

class HttpRequest
{
public:
	typedef enum STATE {
		STATE_UNDEF,
		STATE_INIT,
		STATE_HEADER,
		STATE_CONTENTLENGTH,
		STATE_BODY,
		STATE_PARSED,
		STATE_ERROR = 0xFFFFFFFF
	} STATE;

	typedef enum TYPE {
		TYPE_UNDEF,
		TYPE_GET,
		TYPE_POST,
		TYPE_DELETE,
		TYPE_PUT
	} TYPE;

	typedef enum ENCTYPE {
		ENCTYPE_UNDEF,
		ENCTYPE_URL,
		ENCTYPE_MULTI,
		ENCTYPE_OTHER
	} ENCTYPE;

	struct strcmpFn { bool operator()( std::string const& x, std::string const& y ) const { return 0 < x.compare(y); };};
	typedef std::map< std::string, std::string, strcmpFn > ParamMap;

	std::string		request;
	ParamMap		getData;
	ParamMap		headers;
	std::string		body;
	ParamMap		postData;
	ParamMap		files;
	STATE			state;
	TYPE			type;
	ENCTYPE			enctype;
	std::string		bound;
	int				contentLength;

	HttpRequest() : state(STATE_UNDEF), type(TYPE_UNDEF), enctype(ENCTYPE_UNDEF) {};
	HttpRequest( TCPConnection& conn );
	operator STATE() { return state; }

	STATE parseRequest( TCPConnection& conn );
	static int parseURL( std::string url, std::string& site, ParamMap& getParams );
	static int parseHttpHeader( char const* szIn, TYPE& type, std::string& request, ParamMap& heads );
	static int parseHeader( char const* szIn, ParamMap& heads );
	static int parseHeaderLine( char const* p, ParamMap& params );
	static int parseURLencBody( std::string const& body, ParamMap& postData );
	static int parseMultipartBody( std::string const& body, std::string bound, ParamMap& postData, ParamMap& files );
};

class Server
{
public:
	typedef std::vector< SPtr<SockIn> > Listeners;
	typedef enum MODALSTATE { 
		MODALSTATE_RUN = -1025,
 		MODALSTATE_ERR = -2049,
		MODALSTATE_EXC = -1535
	} MODALSTATE;
protected:
	Listeners	m_listeners;
	FD_SET		m_readers;
	FD_SET		m_writers;
	int			m_modalState;
	VWB_Thread	m_thread;
	VWB_Mutex	m_mtxGlobal;
public:
	timeval m_sto;
	Server();
	Server( SPtr<SockIn> const& s, bool bRunModal );
	virtual ~Server();

	int addReceiver( SPtr<SockIn> s );
	int removeReceiver( SPtr<SockIn> s );

	// s needs to be in listeners already!!
	int addResponder( Socket& s );
	int removeResponder( Socket& s );
	bool empty() { return m_listeners.empty(); }
	int doModalStep(); // call in external loop if not modal
	int endModal( int code );
	int doModal();
	bool isRunningModal();
private: 
	u_int modalLoop();
	static u_int __stdcall _theadFn( void* param );
};

#endif //ndef VWB_SOCKET_HPP
