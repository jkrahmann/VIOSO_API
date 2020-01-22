#include "Net.h"
#include <sstream>

std::queue< VWBRemoteCommand > g_commandQueue;

VWBTCPListener::VWBTCPListener( SocketAddress& s )
: TCPListener( s )
, m_port( s.getPort() ) 
{}

VWB_ERROR VWBTCPListener::add( VWB_Warper* pWarper )
{
	if( NULL == pWarper )
		return VWB_ERROR_PARAMETER;

	if( 0 == pWarper->port )
		return VWB_ERROR_PARAMETER;

	if( m_port != pWarper->port )
		return VWB_ERROR_FALSE;

	for( WarperList::iterator it = m_warpers.begin(); it != m_warpers.end(); it++ )
	{
		if( *it == pWarper )
		{
			logStr( 0, "ERROR: VWBUDPListen: duplicate warper!\n" );
			return VWB_ERROR_GENERIC;
		}
		if( 0 == strcmp( (*it)->channel, pWarper->channel ) )
		{
			logStr( 0, "ERROR: VWBUDPListen: duplicate warper name!\n" );
			return VWB_ERROR_GENERIC;
		}
	}
	m_warpers.push_back( pWarper );
	logStr( 2, "INFO: VWBUDPListen(%hu): warper \"%s\" added.\n", m_port, pWarper->channel );
	return VWB_ERROR_NONE;
}

VWB_ERROR VWBTCPListener::remove( VWB_Warper* pWarper )
{
	if( NULL == pWarper )
		return VWB_ERROR_PARAMETER;

	//if( 0 == pWarper->port ) /// don't be over-picky ;)
	//	return VWB_ERROR_PARAMETER;

	//if( m_port != Socket::hton(pWarper->port) )
	//	return VWB_ERROR_FALSE;

	for( WarperList::iterator it = m_warpers.begin(); it != m_warpers.end(); it++ )
	{
		if( *it == pWarper )
		{
			logStr( 2, "INFO: VWBUDPListen(%hu): warper %s removed!\n", m_port, pWarper->channel );
			m_warpers.erase( it );
			return VWB_ERROR_NONE;
		}
	}
	return VWB_ERROR_FALSE;
}

VWB_ERROR VWBTCPListener::sendInfoTo( SocketAddress sa, SocketAddress* local )
{
	if( 0 == sa.sin_addr.S_un.S_addr )
		return VWB_ERROR_PARAMETER;
	if( 0xFFFFFFFF == sa.sin_addr.S_un.S_addr )
		return VWB_ERROR_PARAMETER;
	SocketAddress my;
	if( NULL == local )
	{
		std::vector<in_addr> list = Socket::getLocalIPList();
		my = SocketAddress( list[0].s_addr, sa.getPort() );
		local = &my;
	}
	char buf[SO_RCVBUF] = {0};
	char buff[20];
	try {
		int pos = sprintf_s( buf, "VIOSOWarpBlend API %d.%d.%d.%d %Iu display(s) on %s:%hu.\015\012", VWB_Version_MAJ,VWB_Version_MIN,VWB_Version_MAI,VWB_Version_REV, m_warpers.size(), local->getDottedDecimal(buff), local->getPort() );
		for( WarperList::iterator it = m_warpers.begin(); it != m_warpers.end(); it++ )
		{
			VWB_Warper_base* p = (VWB_Warper_base*)*it;
			pos+= sprintf_s( &buf[pos], SO_RCVBUF-pos, "\"%s\" %dx%d.\015\012", p->channel, p->getMappingSize().cx, p->getMappingSize().cy );
		}
		if( pos != Socket::sendDatagram( buf, pos, sa, true ) ) // sendto will always send all if smaller than SO_RECVBUF
			return VWB_ERROR_NETWORK;
	}catch( ... )
	{
		return VWB_ERROR_GENERIC;
	}
	return VWB_ERROR_NONE;
}

VWB_Warper* VWBTCPListener::getWarper( char const* szName )
{
	for( auto it = m_warpers.begin(); it != m_warpers.end(); it ++ )
		if( (*it) && 0 == strcmp( (*it)->channel, szName ) )
			return *it;
	return NULL;
}

int VWBTCPListener::cbRead( Server* pServer )
{
	// a new connection has been established
	// we need to promote the new connection to own type to process data to make it call our virtuals
	SPtr<VWBTCPConnection> s = new VWBTCPConnection( m_peers.back().s->detach(), m_peers.back().sa, this, pServer );
	m_peers.back().s = s;
	return 0;
}

int VWBTCPListener::cbError( Server* pServer, int err )
{
	return 0;
}

VWBTCPConnection::VWBTCPConnection( Socket const& other, SocketAddress const& peerAddress, TCPListener* pL, Server* pServer )
: TCPConnection( other, peerAddress, pL, pServer )
, m_pWarper( NULL )
, m_state(0)
, m_iCalls(0)
{
}

int VWBTCPConnection::readRequest()
{
	return 0;
}

int VWBTCPConnection::sendResponse()
{
	return 0;
}

int VWBTCPConnection::cbRead( Server* pServer )
{ // this is always called from within lock
	m_iCalls++;
	if( m_req.empty() ) // initial 
		m_req.push( HttpRequest(*this) );
	else
		m_req.front().parseRequest( *this );
	HttpRequest::STATE r = m_req.front();

	if( HttpRequest::STATE_ERROR == r )
	{
		// report error
		logStr( 2, "INFO: No http request, parse as command string...\n" );
		if( 2 < getNumRead() )
		{
			std::string s( getNumRead() + 1, 0 );
			read( &s[0], getNumRead()+1, getNumRead() ); 

			m_req.front().type = HttpRequest::TYPE_GET;
			if( 0 < HttpRequest::parseURL( s, m_req.front().request, m_req.front().getData ) )
				m_req.front().state = HttpRequest::STATE_PARSED;
			else
			{
				logStr( 1, "WARNING: Malformed request(2)\n" );
			}
		}
		logStr( 1, "WARNING: Malformed request(1)\n" );
		return SOCKET_ERROR;
	}
	if( HttpRequest::STATE_PARSED == r )
	{
		logStr( 2, "INFO: Net receive request %s\n", m_req.front().request.c_str() );
		m_req.push( HttpRequest() );
		return 1;
	}
	return 0;
}

int VWBTCPConnection::cbError( Server* pServer, int err )
{
	return err;
}
