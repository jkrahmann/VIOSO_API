#if !defined( VWB_Net_h )
#define VWB_Net_h

#include "common.h"
#include "socket.h"
#include <map>
#include <queue>

class VWBRemoteCommand
{
public:
	std::string name;
	std::vector<char> data;
};

extern std::queue< VWBRemoteCommand > g_commandQueue;

class VWBTCPListener : public TCPListener
{
public:
	typedef std::vector< VWB_Warper* > WarperList;
protected:

	WarperList m_warpers;
	u_short m_port;
public:
	VWBTCPListener( SocketAddress& s );

	VWB_ERROR add( VWB_Warper* pWarper );
	VWB_ERROR remove( VWB_Warper* pWarper );
	bool empty() { return m_warpers.empty(); }
	u_short port() { return m_port; }
	VWB_ERROR sendInfoTo( SocketAddress sa );
	VWB_Warper* getWarper( char const* szName );

	virtual int cbRead( Server* pServer );
	virtual int cbError( Server* pServer, int err );
};

class VWBTCPConnection : public TCPConnection
{
public:
	typedef std::queue<HttpRequest> RequestList;
protected:
	VWB_Warper* m_pWarper;
	int m_state;
	int m_iCalls;
	RequestList m_req;
public:
	typedef enum STATE
	{
		STATE_GOTDATA = 1,
		STATE_DISPLAY_ASSOC = 2,
		STATE_TESTIMAGE = 4,
		STATE_BYPASS = 8,
		STATE_WRITEPENDING = 16
	} STATE;

	VWBTCPConnection( Socket const& other, SocketAddress const& peerAddress, TCPListener* pL, Server* pServer );
	//explicit VWBTCPConnection( SockIn& other );
	int getState() const { return m_state | ( m_bPendingSend ? STATE_WRITEPENDING : 0 ); }
	int readRequest();
	int sendResponse();
	bool hasRequest() { return !m_req.empty(); }
	void popRequest() 
	{ 
		if( !m_req.empty() ) m_req.pop(); 
	}
	HttpRequest const* headRequestPtr()
	{
		return m_req.empty() ? NULL : &m_req.front();
	}
private:
	virtual int cbRead( Server* pServer );
	virtual int cbError( Server* pServer, int err );
};

#endif //!defined( VWB_Net_h )

