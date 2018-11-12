//==================================================================================================
//  Franks Flightsim Intruments project
//  by Frank van Ierland
//
// This code is in the public domain.
//
//==================================================================================================

//==================================================================================================
// XPdefs.h
//
// author: Frank van Ierland
// version: 0.1
//
// various definitions and procedures to use xplane data thru UDP link
//
//------------------------------------------------------------------------------------
/// \author  Frank van Ierland (frank@van-ierland.com) DO NOT CONTACT THE AUTHOR DIRECTLY: USE THE LISTS
// Copyright (C) 2018 Frank van Ierland

// TODO: Code to send data back to XPlane

#ifndef XpUDP_h_
#define XpUDP_h_

#include "mydebug.h"

#include <stdlib.h>
#include <WiFi.h>
#include <QList.h>

#if defined(ARDUINO) && ARDUINO >= 100
#	include "Arduino.h"
#else
#	include "WProgram.h"
#endif

#if defined(ARDUINO_ARCH_ESP32)
#	include <esp_task_wdt.h>
#endif

#include <limits.h>
#include "XPUtils.h"
#include "Can2XPlane.h"

#define XP_BEACONTIMEOUT_MS 10000
#define XP_Multicast_Port 49707
#define XP_MulticastAdress_0 239
#define XP_MulticastAdress_1 255
#define XP_MulticastAdress_2 1
#define XP_MulticastAdress_3 1

// defines for msg headers used in XP UDP interface.
#define XPMSG_XXXX	"XXXX" // novalid command
#define XPMSG_BECN	"BECN"	//get beacon and inintate connection
#define XPMSG_CMND	"CMND"	// run a command
#define XPMSG_RREF	"RREF"	//SEND ME ALL THE DATAREFS I WANT: RREF
#define XPMSG_DREF	"DREF"	//SET A DATAREF TO A VALUE: DREF
#define XPMSG_DATA	"DATA"	//SET A DATA OUTPUT TO A VALUE: DATA
#define XPMSG_ALRT	"ALRT"	//MAKE AN ALERT MESSAGE IN X-PLANE: ALRT
#define XPMSG_FAIL 	"FAIL"	//FAIL A SYSTEM: FAIL
#define XPMSG_RECO 	"RECO"

constexpr auto _Xp_dref_in_msg_size = 413;
constexpr auto _Xp_rref_size = 400;
constexpr auto _Xp_dref_size = 509;

constexpr long _Xp_beacon_timeout = 1000 * 1000;
constexpr long _Xp_beacon_restart_timeout = 10 * 1000 * 1000;

//==================================================================================================
//
//==================================================================================================
class XpUDP {
public:

	typedef enum {
		SUCCESS = 0,
		BEACON_NOT_FOUND = 1,
		BEACON_WRONG_VERSION = 2,
		WRONG_HEADER = 3,
		BUFFER_OVERFLOW = 4,
		NO_DATA = 5
	} XpErrorCode;

	/*
	typedef enum {
		NO_COMMAND  = 0,
		BEACON		= 1,	//get beacon and inintate connection
		COMMAND 	= 2,	// run a command
		GET_DATAREF = 3,	//SEND ME ALL THE DATAREFS I WANT: RREF
		SET_DATAREF = 4,	//SET A DATAREF TO A VALUE: DREF
		SET_DATAVAL = 5,	//SET A DATA OUTPUT TO A VALUE: DATA
		ALERT		= 6,	//MAKE AN ALERT MESSAGE IN X-PLANE: ALRT
		FAIL		= 7,	//FAIL A SYSTEM: FAIL
		RECOVER		= 8	//RECOVER A SYSTEM: RECO
	} XpCommandType;
	*/
	//constructor
	XpUDP(void(*callbackFunc)(uint16_t canId, float par), void(*setStateFunc)(bool state));
	~XpUDP();

	// start connection
	int start();
	int dataReader();				// poll for new data need to be in loop()
	bool isRunning();

	//int registerDataRef(int iFreq, const char *sCode );
	int registerDataRef(int iFreq, const CanasXplaneTrans* newItem, bool recieveOnly = true);
	int unRegisterDataRef(const char *sCode);
	int unRegisterDataRef(uint16_t canID);
	int setDataRefValue(uint16_t canID, float value);

protected:
	/*
		IPAddress   XpMulticastAdress(239, 255,1, 1);
		uint16_t  	XpMulticastPort = 49707;
	*/

	int 	GetBeacon();
	void 	sendUDPdata(const char *header, const byte *dataArr, const int arrSize, const int sendSize);

	bool		_isRunning = false;
	long		_last_start = -10000000000; // make sure it starts first time

private:
	/*
		string[] _XpCommands = {
			"XXXX", // novalid command
			"BECN",	//get beacon and inintate connection
			"CMND",	// run a command
			"RREF",	//SEND ME ALL THE DATAREFS I WANT: RREF
			"DREF",	//SET A DATAREF TO A VALUE: DREF
			"DATA",	//SET A DATA OUTPUT TO A VALUE: DATA
			"ALRT",	//MAKE AN ALERT MESSAGE IN X-PLANE: ALRT
			"FAIL",	//FAIL A SYSTEM: FAIL
			"RECO"	//RECOVER A SYSTEM: RECO
		} ;
	*/
	static const int 	_UDP_MAX_PACKET_SIZE = 550; // max size of message
	static const int	_MAX_INTERVAL = 500; 		// Max interval in ms between data messages

	byte 		_packetBuffer[_UDP_MAX_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
	WiFiUDP 	_Udp;
	int			_newDataRef;

	// typedefs for the xplane specific types
	typedef uint32_t xint;
	typedef char xchr;
	typedef float xflt;
	typedef double xdob;

	//
	// Internal variables
	//
	int _LastError = XpUDP::SUCCESS;
	uint16_t _XpListenPort = 49000;
	IPAddress _XPlaneIP;
	void(*_callbackFunc)(uint16_t canId, float par);
	void(*_setStateFunc)(bool state);

	//Send in a “dref_freq” of 0 to stop having X-Pane send the dataref values.
	struct _Xp_dref_struct_in {
		xint dref_freq;	//IS ACTUALLY THE NUMBER OF TIMES PER SECOND YOU WANT X-PLANE TO SEND THIS DATA!
		xint dref_en;	// reference code to pass when returning d-ref
		xchr dref_string[400];	// dataref string with optional index [xx]
	};

	_Xp_dref_struct_in _dref_struct_in;

	struct _dref_struct_out {
		xint dref_en;	//Where dref_en is the integer code you sent in for this dataref in the struct above.
		xflt dref_flt;	//Where dref_flt is the dataref value, in machine-native floating-point value, even for ints!
	};

	//Use this to set ANY data-ref by UDP! With this power, you can send in any floating-point value to any data-ref in the entire sim!
	//Just look up the datarefs at http://www.xsquawkbox.net/.
	//Easy!
	//	DREF0+(4byte byte value)+dref_path+0+spaces to complete the whole message to 509 bytes

	struct _dref_struct {
		xflt var;
		xchr dref_path[500];
	};

	//SEND A -999 FOR ANY VARIABLE IN THE SELECTION LIST THAT YOU JUST WANT TO LEAVE ALONE, OR RETURN TO DEFAULT CONTROL IN THE SIMULATOR RATHER THAN UDP OVER-RIDE.

	struct _data_struct {
		uint32_t index;		 // data index, the index into the list of variables you can output from the Data Output screen in X-Plane.
		float data[8]; 	// the up to 8 numbers you see in the data output screen associated with that selection.. many outputs do not use all 8, though.
	};

	struct _Xp_becn_struct {
		char 		header[5];
		uint8_t 	beacon_major_version;		// 1 at the time of X-Plane 10.40
		uint8_t 	beacon_minor_version;		// 1 at the time of X-Plane 10.40
		xint 		application_host_id;		// 1 for X-Plane, 2 for PlaneMaker
		xint 		version_number;				// 104103 for X-Plane 10.41r3
		uint32_t	role;						// 1 for master, 2 for extern visual, 3 for IOS
		uint16_t 	port;						// port number X-Plane is listening on, 49000 by default
		xchr		computer_name[500];			// the hostname of the computer, e.g. “Joe’s Macbook”
	};

	_Xp_becn_struct _becn_struct;

	struct _DataRefs {
		uint8_t	refID;	//Internal number of dataref
		uint8_t frequency = 5;
		bool subscribed = false;
		bool active = true;
		//char command[_Xp_rref_size];	//short internal name
		float value;	// last received value
		//void(*setdata)(float) = NULL;	// function to set updated data in instrument
		unsigned long timestamp = 0;	// last read time
		uint8_t canId = 0;
		const CanasXplaneTrans* paramInfo = NULL;
		bool recieveOnly = true;
		_dref_struct sendStruct;
	};

	//
	// a place to hold all referenced items
	//
	QList<_DataRefs *> _listRefs;

	int		sendRREF(int frequency, int refID, char * xplaneId);
	int 	sendRREF(_DataRefs* newRef);
	int		sendDREF(_DataRefs* newRef);
};
#endif
