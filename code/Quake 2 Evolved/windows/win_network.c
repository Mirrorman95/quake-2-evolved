/*
 ------------------------------------------------------------------------------
 Copyright (C) 1997-2001 Id Software.

 This file is part of the Quake 2 source code.

 The Quake 2 source code is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

 The Quake 2 source code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 more details.

 You should have received a copy of the GNU General Public License along with
 the Quake 2 source code; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ------------------------------------------------------------------------------
*/


//
// win_network.c - Network interface
//


#include "../common/common.h"
#include "win_local.h"


typedef struct {
	bool				initialized;

	SOCKET				sockets[2];

	int					packetsReceived[2];
	int					packetsSent[2];

	longlong			bytesReceived[2];
	longlong			bytesSent[2];
} network_t;

static network_t		net;

static cvar_t *			net_ip;
static cvar_t *			net_port;
static cvar_t *			net_clientPort;


/*
 ==================
 NET_ErrorString
 ==================
*/
static const char *NET_ErrorString (){

	int		error;

	error = WSAGetLastError();

	switch (error){
	case WSAEINTR:				return "WSAEINTR";
	case WSAEACCES:				return "WSAEACCES";
	case WSAEFAULT:				return "WSAEFAULT";
	case WSAEINVAL:				return "WSAEINVAL";
	case WSAEMFILE:				return "WSAEMFILE";
	case WSAEWOULDBLOCK:		return "WSAEWOULDBLOCK";
	case WSAEINPROGRESS:		return "WSAEINPROGRESS";
	case WSAEALREADY:			return "WSAEALREADY";
	case WSAENOTSOCK:			return "WSAENOTSOCK";
	case WSAEDESTADDRREQ:		return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE:			return "WSAEMSGSIZE";
	case WSAEPROTOTYPE:			return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT:		return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT:	return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT:	return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP:			return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT:		return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT:		return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE:			return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL:		return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN:			return "WSAENETDOWN";
	case WSAENETUNREACH:		return "WSAENETUNREACH";
	case WSAENETRESET:			return "WSAENETRESET";
	case WSAECONNABORTED:		return "WSAECONNABORTED";
	case WSAECONNRESET:			return "WSAECONNRESET";
	case WSAENOBUFS:			return "WSAENOBUFS";
	case WSAEISCONN:			return "WSAEISCONN";
	case WSAENOTCONN:			return "WSAENOTCONN";
	case WSAESHUTDOWN:			return "WSAESHUTDOWN";
	case WSAETIMEDOUT:			return "WSAETIMEDOUT";
	case WSAECONNREFUSED:		return "WSAECONNREFUSED";
	case WSAEHOSTDOWN:			return "WSAEHOSTDOWN";
	case WSAEHOSTUNREACH:		return "WSAEHOSTUNREACH";
	case WSAEPROCLIM:			return "WSAEPROCLIM";
	case WSAEDISCON:			return "WSAEDISCON";
	case WSASYSNOTREADY:		return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED:	return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED:		return "WSANOTINITIALISED";
	case WSATYPE_NOT_FOUND:		return "WSATYPE_NOT_FOUND";
	case WSAHOST_NOT_FOUND:		return "WSAHOST_NOT_FOUND";
	case WSANO_RECOVERY:		return "WSANO_RECOVERY";
	case WSANO_DATA:			return "WSANO_DATA";
	case WSATRY_AGAIN:			return "WSATRY_AGAIN";
	default:					return "UNKNOWN ERROR";
	}
}


/*
 ==============================================================================

 ADDRESS FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 NET_NetAdrToSockAdr
 ==================
*/
static void NET_NetAdrToSockAdr (const netAdr_t *adr, struct sockaddr *s){

	Mem_Fill(s, 0, sizeof(*s));

	if (adr->type == NA_BROADCAST){
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_addr.s_addr = INADDR_BROADCAST;
		((struct sockaddr_in *)s)->sin_port = adr->port;
	}
	else if (adr->type == NA_IP){
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_addr.s_addr = *(ulong *)&adr->ip;
		((struct sockaddr_in *)s)->sin_port = adr->port;
	}
}

/*
 ==================
 NET_SockAdrToNetAdr
 ==================
*/
static void NET_SockAdrToNetAdr (const struct sockaddr *s, netAdr_t *adr){

	Mem_Fill(adr, 0, sizeof(*adr));

	if (s->sa_family == AF_INET){
		adr->type = NA_IP;
		*(ulong *)&adr->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
		adr->port = ((struct sockaddr_in *)s)->sin_port;
	}
}

/*
 ==================
 NET_StringToSockAdr

 localhost
 idnewt
 idnewt:28000
 192.246.40.70
 192.246.40.70:28000
 ==================
*/
static bool NET_StringToSockAdr (const char *string, struct sockaddr *s){

	char			copy[MAX_ADDRESS_LENGTH];
	char			*port;
	struct hostent	*host;
	ulong			adr;
	bool			isIP = true;
	int				digits = 0, dots = 0;
	int				i;

	Mem_Fill(s, 0, sizeof(*s));

	if (!string || !string[0])
		return false;

	Str_Copy(copy, string, sizeof(copy));

	// Strip off a trailing port if present
	port = Str_FindChar(copy, ':');
	if (port)
		*port++ = 0;

	// Check if the string is an IP address
	for (i = 0; copy[i]; i++){
		if (copy[i] >= '0' && copy[i] <= '9'){
			if (digits == 3){
				isIP = false;
				break;
			}

			digits++;
		}
		else if (copy[i] == '.'){
			if (digits == 0 || dots == 3){
				isIP = false;
				break;
			}

			digits = 0;
			dots++;
		}
		else {
			isIP = false;
			break;
		}
	}

	if (digits == 0 || dots != 3)
		isIP = false;

	// Set the sockaddr
	((struct sockaddr_in *)s)->sin_family = AF_INET;

	if (isIP){
		adr = inet_addr(copy);
		if (adr == INADDR_NONE)
			return false;

		*(ulong *)&((struct sockaddr_in *)s)->sin_addr = adr;
	}
	else {
		host = gethostbyname(copy);
		if (!host || host->h_addrtype != AF_INET)
			return false;

		*(ulong *)&((struct sockaddr_in *)s)->sin_addr = *(ulong *)host->h_addr_list[0];
	}

	if (!port)
		((struct sockaddr_in *)s)->sin_port = 0;
	else
		((struct sockaddr_in *)s)->sin_port = htons((ushort)Str_ToInteger(port));

	return true;
}

/*
 ==================
 NET_AddressToString
 ==================
*/
const char *NET_AddressToString (const netAdr_t adr){

	static char	string[MAX_ADDRESS_LENGTH];

	if (adr.type == NA_LOOPBACK)
		Str_SPrintf(string, sizeof(string), "loopback");
	else if (adr.type == NA_IP){
		if (adr.port == 0)
			Str_SPrintf(string, sizeof(string), "%i.%i.%i.%i", adr.ip[0], adr.ip[1], adr.ip[2], adr.ip[3]);
		else
			Str_SPrintf(string, sizeof(string), "%i.%i.%i.%i:%hu", adr.ip[0], adr.ip[1], adr.ip[2], adr.ip[3], ntohs(adr.port));
	}
	else
		string[0] = 0;

	return string;
}

/*
 ==================
 NET_StringToAddress

 localhost
 idnewt
 idnewt:28000
 192.246.40.70
 192.246.40.70:28000
 ==================
*/
bool NET_StringToAddress (const char *string, netAdr_t *adr){

	struct sockaddr	s;

	Mem_Fill(adr, 0, sizeof(netAdr_t));

	if (!Str_ICompare(string, "localhost")){
		adr->type = NA_LOOPBACK;
		return true;
	}

	if (!NET_StringToSockAdr(string, &s))
		return false;

	NET_SockAdrToNetAdr(&s, adr);

	return true;
}


// ============================================================================


/*
 ==================
 NET_GetPacket
 ==================
*/
bool NET_GetPacket (netSrc_t sock, netAdr_t *from, msg_t *msg){

	struct sockaddr	adr;
	int				adrLen = sizeof(adr);
	int 			result, error;

	if (NET_GetLoopPacket(sock, from, msg))
		return true;

	if (net.sockets[sock] == INVALID_SOCKET)
		return false;

	result = recvfrom(net.sockets[sock], (char *)msg->data, msg->maxSize, 0, (struct sockaddr *)&adr, &adrLen);

	NET_SockAdrToNetAdr(&adr, from);

	if (result == SOCKET_ERROR){
		error = WSAGetLastError();

		// WSAEWOULDBLOCK and WSAECONNRESET are silent
		if (error == WSAEWOULDBLOCK || error == WSAECONNRESET)
			return false;

		Com_Printf(S_COLOR_RED "NET_GetPacket: %s from %s\n", NET_ErrorString(), NET_AddressToString(*from));
		return false;
	}

	if (result == msg->maxSize){
		Com_Printf(S_COLOR_RED "NET_GetPacket: oversize packet from %s\n", NET_AddressToString(*from));
		return false;
	}

	msg->curSize = result;

	net.packetsReceived[sock]++;
	net.bytesReceived[sock] += result;

	return true;
}

/*
 ==================
 NET_SendPacket
 ==================
*/
void NET_SendPacket (netSrc_t sock, const netAdr_t to, const void *data, int length){

	struct sockaddr	adr;
	int				result, error;

	if (NET_SendLoopPacket(sock, to, data, length))
		return;

	if (to.type != NA_BROADCAST && to.type != NA_IP)
		Com_Error(ERR_FATAL, "NET_SendPacket: bad address type");

	if (net.sockets[sock] == INVALID_SOCKET)
		return;

	NET_NetAdrToSockAdr(&to, &adr);

	result = sendto(net.sockets[sock], (const char *)data, length, 0, &adr, sizeof(adr));

	if (result == SOCKET_ERROR){
		error = WSAGetLastError();

		// WSAEWOULDBLOCK is silent
		if (error == WSAEWOULDBLOCK)
			return;

		// Some PPP links don't allow broadcasts
		if (error == WSAEADDRNOTAVAIL && to.type == NA_BROADCAST)
			return;

		Com_Printf(S_COLOR_RED "NET_SendPacket: %s to %s\n", NET_ErrorString(), NET_AddressToString(to));
	}

	net.packetsSent[sock]++;
	net.bytesSent[sock] += result;
}


/*
 ==============================================================================

 UDP SOCKET

 ==============================================================================
*/


/*
 ==================
 NET_UDPSocket
 ==================
*/
static bool NET_UDPSocket (netSrc_t sock, const char *ip, int port){

	struct sockaddr_in	adr;
	ulong				value = 1;

	Com_DPrintf("NET_UDPSocket( %s, %i )\n", ip, port);

	// Open an UDP socket
	if ((net.sockets[sock] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET){
		Com_DPrintf(S_COLOR_YELLOW "NET_OpenSocket: socket() = %s\n", NET_ErrorString());
		return false;
	}

	// Make it non-blocking
	if (ioctlsocket(net.sockets[sock], FIONBIO, &value) == SOCKET_ERROR){
		Com_DPrintf(S_COLOR_YELLOW "NET_OpenSocket: ioctlsocket() = %s\n", NET_ErrorString());

		closesocket(net.sockets[sock]);
		net.sockets[sock] = INVALID_SOCKET;

		return false;
	}

	// Make it broadcast capable
	if (setsockopt(net.sockets[sock], SOL_SOCKET, SO_BROADCAST, (const char *)&value, sizeof(value)) == SOCKET_ERROR){
		Com_DPrintf(S_COLOR_YELLOW "NET_OpenSocket: setsockopt() = %s\n", NET_ErrorString());

		closesocket(net.sockets[sock]);
		net.sockets[sock] = INVALID_SOCKET;

		return false;
	}

	// Bind it
	adr.sin_family = AF_INET;

	if (!Str_ICompare(ip, "localhost"))
		adr.sin_addr.s_addr = INADDR_ANY;
	else {
		if (!NET_StringToSockAdr(ip, (struct sockaddr *)&adr))
			adr.sin_addr.s_addr = INADDR_ANY;
	}

	if (port == 0)
		adr.sin_port = 0;
	else
		adr.sin_port = htons((ushort)port);

	if (bind(net.sockets[sock], (void *)&adr, sizeof(adr)) == SOCKET_ERROR){
		Com_DPrintf(S_COLOR_YELLOW "NET_OpenSocket: bind() = %s\n", NET_ErrorString());

		closesocket(net.sockets[sock]);
		net.sockets[sock] = INVALID_SOCKET;

		return false;
	}

	return true;
}

/*
 ==================
 NET_OpenUDP
 ==================
*/
static void NET_OpenUDP (){

	if (NET_UDPSocket(NS_SERVER, net_ip->value, net_port->integerValue))
		Com_Printf("Opened server UDP socket: %s:%i\n", net_ip->value, net_port->integerValue);
	else
		Com_Printf(S_COLOR_RED "Could not open server UDP socket\n");

	// Dedicated servers don't need client ports
	if (com_dedicated->integerValue)
		return;

	if (NET_UDPSocket(NS_CLIENT, net_ip->value, net_clientPort->integerValue))
		Com_Printf("Opened client UDP socket: %s:%i\n", net_ip->value, net_clientPort->integerValue);
	else
		Com_Printf(S_COLOR_RED "Could not open client UDP socket\n");
}

/*
 ==================
 NET_CloseSocket
 ==================
*/
static void NET_CloseSocket (netSrc_t sock){

	if (net.sockets[sock] == INVALID_SOCKET)
		return;

	if (closesocket(net.sockets[sock]) == SOCKET_ERROR)
		Com_DPrintf(S_COLOR_YELLOW "NET_CloseSocket: closesocket() = %s\n", NET_ErrorString());

	net.sockets[sock] = INVALID_SOCKET;
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 NET_ShowIP_f
 ==================
*/
static void NET_ShowIP_f (){

	char			name[MAX_ADDRESS_LENGTH];
	struct hostent	*host;
	struct in_addr	adr;
	int				i;

	if (gethostname(name, sizeof(name)) == SOCKET_ERROR){
		Com_Printf("Couldn't get host name\n");
		return;
	}

	host = gethostbyname(name);
	if (!host || host->h_addrtype != AF_INET){
		Com_Printf("Couldn't get host\n");
		return;
	}

	Com_Printf("HostName: %s\n", host->h_name);

	for (i = 0; host->h_aliases[i]; i++)
		Com_Printf("Alias: %s\n", host->h_aliases[i]);

	for (i = 0; host->h_addr_list[i]; i++){
		adr.s_addr = *(ulong *)host->h_addr_list[i];

		Com_Printf("IP: %s\n", inet_ntoa(adr));
	}
}

/*
 ==================
 NET_ShowTraffic_f
 ==================
*/
static void NET_ShowTraffic_f (){

	Com_Printf("\n");
	Com_Printf("%6s: %9I64i bytes (%6.2f MB) %8s in %6i packets\n", "Client", net.bytesReceived[NS_CLIENT], net.bytesReceived[NS_CLIENT] * (1.0f / 1048576.0f), "received", net.packetsReceived[NS_CLIENT]);
	Com_Printf("%6s: %9I64i bytes (%6.2f MB) %8s in %6i packets\n", "Client", net.bytesSent[NS_CLIENT], net.bytesSent[NS_CLIENT] * (1.0f / 1048576.0f), "sent", net.packetsSent[NS_CLIENT]);
	Com_Printf("%6s: %9I64i bytes (%6.2f MB) %8s in %6i packets\n", "Server", net.bytesReceived[NS_SERVER], net.bytesReceived[NS_SERVER] * (1.0f / 1048576.0f), "received", net.packetsReceived[NS_SERVER]);
	Com_Printf("%6s: %9I64i bytes (%6.2f MB) %8s in %6i packets\n", "Server", net.bytesSent[NS_SERVER], net.bytesSent[NS_SERVER] * (1.0f / 1048576.0f), "sent", net.packetsSent[NS_SERVER]);
	Com_Printf("\n");
}

/*
 ==================
 NET_Restart_f
 ==================
*/
static void NET_Restart_f (){

	NET_Shutdown();
	NET_Init();
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 NET_Init
 ==================
*/
void NET_Init (){

	WSADATA	wsaData;
	int		error;

	Com_Printf("-------- Network Initialization --------\n");

	// Register variables
	net_ip = CVar_Register("net_ip", "localhost", CVAR_STRING, CVAR_LATCH, "Local IP address", 0, 0);
	net_port = CVar_Register("net_port", Str_FromInteger(PORT_SERVER), CVAR_INTEGER, CVAR_LATCH, "Local server IP port", 10000, 65535);
	net_clientPort = CVar_Register("net_clientPort", Str_FromInteger(PORT_CLIENT), CVAR_INTEGER, CVAR_LATCH, "Local client IP port", 10000, 65535);

	// Add commands
	Cmd_AddCommand("showIP", NET_ShowIP_f, "Shows the local host name and IP address", NULL);
	Cmd_AddCommand("showTraffic", NET_ShowTraffic_f, "Shows network traffic statistics", NULL);
	Cmd_AddCommand("restartNetwork", NET_Restart_f, "Restarts the network subsystem", NULL);

	// Make sure the net_ip variable is valid
	if (!net_ip->value[0])
		CVar_ForceSet("net_ip", "localhost");

	// Initialize WinSock
	error = WSAStartup(MAKEWORD(1, 1), &wsaData);
	if (error)
		Com_Error(ERR_FATAL, "WinSock initialization failed, returned %i", error);

	Com_Printf("WinSock Initialized\n");

	// Open the UDP sockets
	NET_OpenUDP();

	// Print local host name and IP address
	NET_ShowIP_f();

	// Network fully initialized
	net.initialized = true;

	Com_Printf("----------------------------------------\n");
}

/*
 ==================
 NET_Shutdown
 ==================
*/
void NET_Shutdown (){

	if (!net.initialized)
		return;

	// Remove commands
	Cmd_RemoveCommand("showIP");
	Cmd_RemoveCommand("showTraffic");
	Cmd_RemoveCommand("restartNetwork");

	// Close the UDP sockets
	NET_CloseSocket(NS_SERVER);
	NET_CloseSocket(NS_CLIENT);

	// Shutdown WinSock
	WSACleanup();

	Mem_Fill(&net, 0, sizeof(network_t));
}