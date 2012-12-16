#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <winioctl.h>

#include "ppjioctl.h"
#include "ppjioctl_devname.h"

extern "C" {
#include "dictionary.h"
#include "iniparser.h"
}

#define	NUM_ANALOG	3		/* Number of analog values which we will provide */
#define	NUM_DIGITAL	0		/* Number of digital values which we will provide */

#pragma pack(push, 1)		/* All fields in structure must be byte aligned. */
struct JOYSTICK_STATE
{
	unsigned long	Signature;				/* Signature to identify packet to PPJoy IOCTL */
	char			NumAnalog;				/* Num of analog values we pass */
	long			Analog[NUM_ANALOG];		/* Analog values */
	char			NumDigital;				/* Num of digital values we pass */
	// char			Digital[NUM_DIGITAL];	/* Digital values */
};
#pragma pack(pop)

int main (int argc, char **argv)
{
	HANDLE				h;
	JOYSTICK_STATE		JoyState;

	DWORD				RetSize;
	DWORD				rc;

	long				*Analog;
//	char				*Digital;

	char				*DevName;

	DevName = PPJOY_IOCTL_DEVNAME;

	/* Open a handle to the control device for the first virtual joystick. */
	h = CreateFile(DevName,GENERIC_WRITE,FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);

	/* Make sure we could open the device! */
	if (h == INVALID_HANDLE_VALUE)
	{
		printf("CreateFile failed with error code %d trying to open %s device\n", GetLastError(), DevName);
		goto fail;
	}

	/* Initialise the IOCTL data structure */
	JoyState.Signature = JOYSTICK_STATE_V1;
	JoyState.NumAnalog = NUM_ANALOG;	/* Number of analog values */
	Analog = JoyState.Analog;			/* Keep a pointer to the analog array for easy updating */
	JoyState.NumDigital = NUM_DIGITAL;	/* Number of digital values */
//	Digital = JoyState.Digital;			/* Digital array */

	printf("Opened joystick device.\n");

	//
	// Read config file
	//

	char *hostname = "";
	int port = 0;

	char *ini_name = "config.ini";
	dictionary *ini = iniparser_load(ini_name);
	if (ini == NULL) {
		printf("Cannot parse file: %s\n", ini_name);
	}
	else
	{
		hostname = iniparser_getstring(ini, "server:host", "");
		port = iniparser_getint(ini, "server:port", 0);
	}
	
	//
	// Setup connection
	//

	WSADATA wsaData;
	int error = WSAStartup(MAKEWORD(2, 0), &wsaData);

	/* check for error */
	if (error != 0)
	{
		printf ("WinSock init failed\n");
		goto fail;
	}

	/* check for correct version */
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0)
	{
		/* incorrect WinSock version */
		WSACleanup();
		printf("WinSock init failed\n");
		goto fail;
	}

	SOCKET client = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	hostent *host = gethostbyname(hostname);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = ((struct in_addr *)(host->h_addr))->s_addr;
	sin.sin_port = htons(port);

	printf("Connecting to %s:%d...\n", hostname, port);

	if (connect(client, (sockaddr *)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		printf("Could not connect to server\n");
		goto fail;
	}

		//case	'1':	Digital[0]= 1;	break;
		//case	'2':	Digital[1]= 1;	break;
		//case	'3':	Digital[2]= 1;	break;
		//case	'4':	Digital[3]= 1;	break;
		//case	'5':	Digital[4]= 1;	break;
		//case	'6':	Digital[5]= 1;	break;
		//case	'7':	Digital[6]= 1;	break;
		//case	'8':	Digital[7]= 1;	break;
		//case	'9':	Digital[8]= 1;	break;
		//case	'0':	Digital[9]= 1;	break;
		//case	'z':	Digital[10]= 1;	break;
		//case	'x':	Digital[11]= 1;	break;
		//case	'c':	Digital[12]= 1;	break;
		//case	'v':	Digital[13]= 1;	break;
		//case	'b':	Digital[14]= 1;	break;
		//case	'n':	Digital[15]= 1;	break;

		//case	'q':	Analog[0]= PPJOY_AXIS_MAX;	break;
		//case	'w':	Analog[1]= PPJOY_AXIS_MAX;	break;
		//case	'e':	Analog[2]= PPJOY_AXIS_MAX;	break;
		//case	'r':	Analog[3]= PPJOY_AXIS_MAX;	break;
		//case	't':	Analog[4]= PPJOY_AXIS_MAX;	break;
		//case	'y':	Analog[5]= PPJOY_AXIS_MAX;	break;
		//case	'u':	Analog[6]= PPJOY_AXIS_MAX;	break;
		//case	'i':	Analog[7]= PPJOY_AXIS_MAX;	break;

	printf("Connection established! Reading data.\n");

	__int16 data[2] = {0};
	int bytes = 0;

	while (recv(client, (char *)data, sizeof(data), 0) >= 0)
	{
		Analog[0]= data[0];
		Analog[1]= data[1];
		Analog[2] = 0;

		//printf("Got data: axis[0] = %d  axis[1] = %d\n", data[0], data[1]);

		/* Send request to PPJoy for processing. */
		/* Currently there is no Return Code from PPJoy, this may be added at a */
		/* later stage. So we pass a 0 byte output buffer.                      */
		if (!DeviceIoControl(h, IOCTL_PPORTJOY_SET_STATE, &JoyState, sizeof(JoyState), NULL, 0, &RetSize,NULL))
		{
			rc = GetLastError();
			if (rc == 2)
			{
				printf ("Underlying joystick device deleted. Exiting read loop\n");
				break;
			}
			printf ("DeviceIoControl error %d\n",rc);
		}
	}

	int retval = 0;

fail:
	retval = 1;
	CloseHandle(h);
	closesocket(client);
	WSACleanup();

	return retval;
}