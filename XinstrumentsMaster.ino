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

#include <stdlib.h>
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
// send changed state XPLane interface to instruments
//-------------------------------------------------------------------------------------------------
void XpSendState2Canbus(bool isRunning)
{
	myCANbus->setExternalBusState(isRunning);
}

//-------------------------------------------------------------------------------------------------
//  send changed state
//-------------------------------------------------------------------------------------------------
int CANASsendChangedControlValue2XP(canbusId_t canId, float value)
{
	DLPRINTINFO(2, "START");
	myXpInterface->setDataRefValue(canId, value);
	DLPRINTINFO(2, "STOP");
	return 0;
}
//-------------------------------------------------------------------------------------------------
// set changed state of a parameter to CanBus
//-------------------------------------------------------------------------------------------------
void XpSendChangedParam2Canbus(uint16_t canId, float val)
{
	int service_code = 0;

	DLPRINTINFO(2, "START");

	DLVARPRINT(1, "CanID:", canId); DLVARPRINTLN(1, " :value:", val);

	myCANbus->ParamUpdateValue(canId, val);

	DLPRINTINFO(2, "STOP");
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int CANASrequestNewXitem(uint16_t canID)
{
	DLPRINTINFO(2, "START/STOP");

	//CanasXplaneTrans* foundItem;
	// check if item exist
	//foundItem = Can2XPlane::fromCan2XplaneElement(canID);

	//if (foundItem->canasId == 0)
	//	return -1;

	return myXpInterface->registerDataRef((uint16_t)canID);
};
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int CANASremoveXitem(uint16_t canID)
{
	DLPRINTINFO(2, "START");

	return myXpInterface->unRegisterDataRef(canID);

	DLPRINTINFO(2, "STOP");
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
	DLPRINTINFO(2, "START");

	DLVARPRINTLN(1, "Connecting to ", ssid);

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		DLPRINT(1, ".");
	};
	DLPRINTLN(1, "<");

	// Print the IP address
	ipOwn = WiFi.localIP();
	DLVARPRINTLN(1, "WiFi connected:", WiFi.localIP());

	DLPRINTLN(1, "creating XPlane interface");
	myXpInterface = new XpUDP(XpSendChangedParam2Canbus, XpSendState2Canbus);

	DLPRINTLN(1, "Creating Can Bus interface");
	myCANbus = new ICanBus(XI_Instrument_NodeID, XI_Hardware_Revision, XI_Software_Revision);
	assert(myCANbus != NULL);

	DLPRINTLN(1, "starting xp interface");
	myXpInterface->start();

	// startup the CAN areospace bus

	DLPRINTLN(1, "starting CanAs interface");
	// make sure right can pins for board aree set
	myCANbus->setCANPins(XI_CANBUS_RX, XI_CANBUS_TX);
	// as we are master we will listen to serviceRequestdata;
	myCANbus->ServiceRegister_master(CANASrequestNewXitem, CANASremoveXitem, CANASsendChangedControlValue2XP);
	myCANbus->start(XI_CANBUS_SPEED);

	DLPRINTINFO(2, "STOP");
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// the loop function runs over and over again until power down or reset
void loop()
{
	int res;

	myXpInterface->dataReader();

	res = myCANbus->UpdateMaster();
	if (res != 0)
	{
		DLVARPRINTLN(0, "Error update Canbus:", res);
	}
};