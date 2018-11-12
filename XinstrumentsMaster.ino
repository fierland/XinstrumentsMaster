//==================================================================================================
//  Franks Flightsim Intruments project
//  by Frank van Ierland
//
// This code is in the public domain.
//
//==================================================================================================
// TODO: Get instrument power state from xplane and update instruments using beacon
// TODO: check for dead instruments and clean up entries and data requests
// TODO: test with xplane
//==================================================================================================

/*
 Name:		XinstrumentsMaster.ino
 Created:	3/30/2018 6:41:46 PM
 Author:	Frank
*/

#include <QList.h>
#include <esp32_can.h>
#include "XinstrumentsMaster.h"
#include "mydebug.h"
#include "XpUDP.h"
#include <Can2XPlane.h>
#include <ICanBus.h>

#include <WiFiUdp.h>
#include <WiFiType.h>
#include <WiFiSTA.h>
#include <WiFiServer.h>
#include <WiFiScan.h>
#include <WiFiMulti.h>
#include <WiFiGeneric.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WiFi.h>
#include <ETH.h>

const char* ssid = "OnzeStek";
const char* password = "MijnLief09";

XpUDP *myXpInterface = NULL;
ICanBus	*myCANbus = NULL;

#define DEB_TM_STEP 50000 // test only
long tmLastBeacon = 0;
long sendVal = 10;  // test only
long timerVal = DEB_TM_STEP; // testonly;

//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void XPsetStateCallback(bool isRunning)
{
	// TODO: code state callback from xplane
}

//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
int XPsetParamValue(canbusId_t canId, float value)
{
 DPRINTINFO("START");
	myXpInterface->setDataRefValue(canId, value);
 DPRINTINFO("STOP");
 return 0;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void xpCallBackFunction(uint16_t canId, float val)
{
	int service_code = 0;

	DPRINTINFO("START");

  DPRINT("CanID:");DPRINT(canId);DPRINT(" :value:");DPRINTLN(val);

	myCANbus->ParamUpdateValue(canId, val);

	DPRINTINFO("STOP");
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int callbackNewXitem(uint16_t canID)
{
	DPRINTINFO("START");
	CanasXplaneTrans* foundItem;
	// check if item exist
	foundItem = Can2XPlane::fromCan2XplaneElement(canID);

	if (foundItem->canasId == 0)
		return -1;

	myXpInterface->registerDataRef(5, foundItem);

	DPRINTINFO("STOP");
	return 0;
};
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int callbackRemoveXitem(uint16_t canID)
{
	DPRINTINFO("START");

	return myXpInterface->unRegisterDataRef(canID);
	DPRINTINFO("STOP");
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// the setup function runs once when you press reset or power the board
void setup()
{
	IPAddress ipOwn;

	Serial.begin(115200);
	// setup wifi connection
		//TODO: Store wifi details in perm memory
		//TODO: use wifi  autosetup for first connect
		  // Connect to WiFi network
	DPRINTINFO("START");
	DPRINTLN();
	DPRINT("Connecting to ");
	DPRINTLN(ssid);

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		DPRINT(".");
	};
	DPRINTLN("<");
	DPRINTLN("WiFi connected");
	// Print the IP address
	ipOwn = WiFi.localIP();
	DPRINTLN(WiFi.localIP());

	DPRINTLN("creating XPlane interface");
	myXpInterface = new XpUDP(xpCallBackFunction, XPsetStateCallback);
  DPRINTLN("starting");
	myXpInterface->start();

	// startup the CAN areospace bus
	DPRINTLN("starting Can Bus interface");
	myCANbus = new ICanBus(XI_Instrument_NodeID, XI_Hardware_Revision, XI_Software_Revision);
	assert(myCANbus != NULL);

	DPRINTLN("starting CanAs interface");
	// make sure right can pins for board aree set
	myCANbus->setCANPins(XI_CANBUS_RX, XI_CANBUS_TX);
	myCANbus->start(XI_CANBUS_SPEED);

	// as we are master we will listen to serviceRequestdata;
	myCANbus->ServiceRegister_master(callbackNewXitem, callbackRemoveXitem, XPsetParamValue);

	DPRINTINFO("STOP");
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// the loop function runs over and over again until power down or reset
void loop()
{
	//myXpInterface->dataReader();

	if (millis() > timerVal)
	{
		xpCallBackFunction(668, sendVal++);
		DPRINTLN("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$UPDATING VALUE");
		timerVal += DEB_TM_STEP;
	}

	myCANbus->Update();
};
