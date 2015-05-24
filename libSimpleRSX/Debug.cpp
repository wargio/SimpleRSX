/*
   Small demonstration of how to use netcat to get debug printing in psl1ght.
   This is for fw 3.55 where ethdebug is not available.

   Usage:
   1. Set the correct ip to your computer below (DEBUG_IP).
   2. Fire up this command in linux / cygwin or equal:
   nc -l -u -p 18194
   3. Start the app on ps3.

   Don't forget to rerun nc after each run.

   Btw code is 100% stolen from the gemtest sample made by bigboss.
*/

#include <SimpleRSX/Debug.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <net/net.h>
#include <netinet/in.h>

static bool network = false;

void Debug::Printf(const char* fmt, ...) {
	if(!network || SocketFD < 0)
		return;
	char buffer[0x800];
	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);
	netSend(SocketFD, buffer, strlen(buffer), 0);
}

Debug::Debug(int port, const char* ip_address) {
	if(port && ip_address){
		if(!network){
			netInitialize();
			network = true;
		}
		struct sockaddr_in stSockAddr;
		SocketFD = netSocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		memset(&stSockAddr, 0, sizeof stSockAddr);

		stSockAddr.sin_family = AF_INET;
		stSockAddr.sin_port = htons(port);
		inet_pton(AF_INET, ip_address, &stSockAddr.sin_addr);

		netConnect(SocketFD, (struct sockaddr *) &stSockAddr, sizeof stSockAddr);

		Printf("network debug module initialized\n");
		Printf("ready to have a lot of fun\n");

	}else
		SocketFD = -1;
}

Debug::~Debug(){
	if(SocketFD > 0){
		netClose(SocketFD);
		if(network){
			netDeinitialize();
			network = false;
		}		
	}
}
