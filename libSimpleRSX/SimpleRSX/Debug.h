/*
   Usage:
   1. Set the correct ip to your computer below.
   2. Fire up this command in linux / cygwin or equal:
      nc -l -u 18194
   3. Start the app on ps3.

   Don't forget to rerun nc after each run.

   Btw code is 100% stolen from the gemtest sample made by bigboss.
   So credits to him!
*/

#ifndef DEBUG_H_
#define DEBUG_H_

class Debug {
public:
	Debug(int port, const char* ip_address);
	~Debug();

	void Printf(const char* fmt, ...);

private:
	int SocketFD;
};

#endif /* DEBUG_H_ */
