//==================================================================================================
//  Franks Flightsim Intruments project
//  by Frank van Ierland
//
// This code is in the public domain.
//
//==================================================================================================

//==================================================================================================
// XpUDP.h
//
// author: Frank van Ierland
// version: 0.1
//
// UDP communication to Xplane server
//
//------------------------------------------------------------------------------------
/// \author  Frank van Ierland (frank@van-ierland.com) DO NOT CONTACT THE AUTHOR DIRECTLY: USE THE LISTS
// Copyright (C) 2018 Frank van Ierland// setting up the UPD stuff
//==================================================================================================
//==================================================================================================
#include "XpUDP.h"

//==================================================================================================
//	constructor
//==================================================================================================
XpUDP::XpUDP(void(*callbackFunc)(uint16_t canId, float par), void(*setStateFunc)(bool state))
{
	DPRINTINFO("START");
	_newDataRef = 1;
	_callbackFunc = callbackFunc;
	_setStateFunc = setStateFunc;
	//_listRefs = new QList<XpUDP::_DataRefs>() ;
	DPRINTINFO("STOP");
}

XpUDP::~XpUDP()
{}

//==================================================================================================
//	get beacon from X Plane server and store adres and port
//==================================================================================================
int XpUDP::GetBeacon()
{
	DPRINTINFO("START");
	int message_size;
	int noBytes;

	noBytes = 0;
	message_size = 0;

	_LastError = XpUDP::SUCCESS;

	noBytes = _Udp.parsePacket(); //// returns the size of the packet
	if (noBytes > 0)
	{
		DPRINT(millis() / 1000);
		DPRINT(":Packet of ");
		DPRINT(noBytes);
		DPRINT(" received from ");
		DPRINT(_Udp.remoteIP());
		DPRINT(":");
		DPRINTLN(_Udp.remotePort());
		_XPlaneIP = _Udp.remoteIP();

		if (noBytes < _UDP_MAX_PACKET_SIZE)
		{
			message_size = _Udp.read(_packetBuffer, noBytes); // read the packet into the buffer

			if (message_size < sizeof(_becn_struct))
			{
				// copy to breacon struct and compensate byte alignment at 8th byte

				memcpy(&_becn_struct, _packetBuffer, 8);
				memcpy(&((char*)&_becn_struct)[8], (_packetBuffer)+7, sizeof(_becn_struct) - 8);
				;

				if (strcmp(_becn_struct.header, XPMSG_BECN) == 0)
				{
					DPRINTLN("UDP beacon received");
					DPRINT("beacon_major_version:");
					DPRINTLN(_becn_struct.beacon_major_version);
					DPRINT("beacon_minor_version:");
					DPRINTLN(_becn_struct.beacon_minor_version);
					DPRINT("application_host_id:");
					DPRINTLN(_becn_struct.application_host_id);
					DPRINT("version_number:");
					DPRINTLN(_becn_struct.version_number);
					DPRINT("application_host_id:");
					DPRINTLN(_becn_struct.application_host_id);
					DPRINT("version_number:");
					DPRINTLN(_becn_struct.version_number);
					DPRINT("role:");
					DPRINTLN(_becn_struct.role);
					DPRINT("port:");
					DPRINTLN(_becn_struct.port);
					DPRINT("computer_name:");
					DPRINTLN(_becn_struct.computer_name);

					if (_becn_struct.beacon_major_version != 1 || _becn_struct.beacon_minor_version != 1)
					{
						DPRINTLN("Beacon: Version wrong");
						DPRINT("Version found: ");
						DPRINT(_becn_struct.beacon_major_version);
						DPRINT(".");
						DPRINTLN(_becn_struct.beacon_minor_version);

						_LastError = XpUDP::WRONG_HEADER;
					}
					// store listen port
					_XpListenPort = _becn_struct.port;
				}
				else
				{
					DPRINTLN("Beacon: Header wrong");
					DPRINTLN(_becn_struct.header);

					_LastError = XpUDP::WRONG_HEADER;
				}
			}
			else
			{
				DPRINTLN("Beacon: Message size to big for struct");

				_LastError = XpUDP::BUFFER_OVERFLOW;
			}
		}
		else
		{
			DPRINTLN("Beacon: Message size to big for buffer");

			_LastError = XpUDP::BUFFER_OVERFLOW;
		}
	}
	else
	{
		DPRINTLN("Beacon: Nothing to read");

		_LastError = XpUDP::BEACON_NOT_FOUND;
	}
	// everything ok :-)

	DPRINTINFO("STOP");
	// debug
	//_XpListenPort = 49000;
	//_XPlaneIP = IPAddress(192, 168, 9, 121);
	//return 0;

	return _LastError;
}

//==================================================================================================
//	start the session with the X-Plane server
//==================================================================================================

int XpUDP::start()
{
	IPAddress   XpMulticastAdress(XP_MulticastAdress_0, XP_MulticastAdress_1, XP_MulticastAdress_2, XP_MulticastAdress_3);
	uint16_t  	XpMulticastPort = XP_Multicast_Port;
	int 	result;
	long startTs = millis();

	DPRINTINFO("Start");
	if (_isRunning)
	{
		DPRINTINFO("STOPPING: Is running");
		return -3;
	}

	if (startTs < _last_start + _Xp_beacon_restart_timeout)
	{
		DPRINTINFO("STOPPING: to soon");
		DPRINT(startTs); DPRINT(":"); DPRINT(_last_start); DPRINT(":"); DPRINTLN(_Xp_beacon_restart_timeout);
		return -2;
	}

	_last_start = startTs;

	DPRINTINFO("START");
	DPRINT("Starting connection, listening on:"); DPRINT(XpMulticastAdress); DPRINT(" Port:"); DPRINTLN(XpMulticastPort);

#ifdef ARDUINO_ARCH_ESP32
	if (_Udp.beginMulticast(XpMulticastAdress, XpMulticastPort))
	{
#else
	IPAddress ipOwn;
	ipOwn = WiFi.localIP();
	if (_Udp.beginMulticast(ipOwn, XpMulticastAdress, XpMulticastPort))
	{
#endif
		DPRINTLN("Udp start ok");
		do
		{
			result = GetBeacon();
#ifdef ARDUINO_ARCH_ESP8266
			ESP.wdtFeed();
#endif
#ifdef ARDUINO_ARCH_ESP32
			esp_task_wdt_reset();
#endif
			delay(10);
			DPRINT("Result:"); DPRINTLN(result);
		} while (result != 0 && ((millis() - startTs) < XP_BEACONTIMEOUT_MS));
	}
	else
	{
		DPRINTLN("Udp start failed");
		return  -1;
	}

	if (result != 0)
	{
		DPRINTLN("!! No beackon");
	}
	else
		_isRunning = true;

	_setStateFunc(_isRunning);

	DPRINTINFO("STOP");
	return result;
}

//==================================================================================================
//	data reader for in loop() pols the udp port for new messages and dispaces them to controls
// TODO: process arrays
//==================================================================================================
int XpUDP::dataReader()
{
	int message_size = 0;
	int noBytes = 0;
	int curRecord = -1;
	_DataRefs* tmpRef;
	_DataRefs* curRef = NULL;
	long startTs = millis();

	if (!_isRunning)
	{
		if (startTs < _last_start + _Xp_beacon_restart_timeout)
			start();
		return -1;
	}
	// noBytes=0;
	// message_size=0;

	_LastError = XpUDP::SUCCESS;

	noBytes = _Udp.parsePacket(); //// returns the size of the packet

	if (noBytes > 0)
	{
		DPRINT(millis() / 1000);
		DPRINT(":Packet of ");
		DPRINT(noBytes);
		DPRINT(" received from ");
		DPRINT(_Udp.remoteIP());
		DPRINT(":");
		DPRINTLN(_Udp.remotePort());

		message_size = _Udp.read(_packetBuffer, noBytes); // read the packet into the buffer
		DPRINT("Header:");
		DPRINTLN((char *)_packetBuffer);
		DPRINTBUFFER(_packetBuffer, noBytes);

		// check if header is RREF
		if (noBytes >= 4 && _packetBuffer[0] == 'R' && _packetBuffer[1] == 'R' && _packetBuffer[2] == 'E' && _packetBuffer[3] == 'F')
		{
			DPRINTLN("@@**** Received RREF");
			// read the values into array
			for (int idx = 5; idx < noBytes; idx += 8)
			{
#if defined(ARDUINO_ARCH_ESP8266)
				ESP.wdtFeed();
#endif
#ifdef ARDUINO_ARCH_ESP32
				esp_task_wdt_reset();
#endif
				// get value
				float value = xflt2float(&(_packetBuffer[idx + 4]));
				// get en
				uint32_t en = xint2uint32(&(_packetBuffer[idx]));
				DPRINT("Index value:");
				DPRINTLN(en);
				DPRINT("param value:");
				DPRINTLN(value);

				// check if we know the dataref number
				for (int i = 0; i < _listRefs.size(); i++)
				{
					tmpRef = _listRefs[i];
					if (tmpRef->refID == en)
					{
						curRef = _listRefs[i];
						break;
					}
				}

				// process data
				if (curRef != NULL)
				{
					DPRINT("Code found in subscription list canID="); DPRINTLN(curRef->paramInfo->canasId);
					curRef->timestamp = millis();
					if (curRef->value != value)
					{
						DPRINT("Updating value from:");
						DPRINT(curRef->value);
						DPRINT(":to:");
						DPRINTLN(value);
						// TODO: Call to update function
						if (_callbackFunc != NULL)
							_callbackFunc(curRef->paramInfo->canasId, value);
						//if (curRef->setdata) {
						//	curRef->setdata(value);
					}
					curRef->value = value;
				}
				else
				{
					// not found in subscription list so sign out of a RREF
					DPRINTLN("Code not found in subscription list");
					sendRREF(0, en, "");
				}
		}
	}
		// test for other headers here

		else
		{
			_LastError = XpUDP::NO_DATA;
		}
}

	// now loop to check if we are receiving all data
	//
	unsigned long timeNow = millis();

	for (int i = 0; i < _listRefs.size(); i++)
	{
		tmpRef = _listRefs[i];
		if ((timeNow - tmpRef->timestamp) > _MAX_INTERVAL)
		{
			DPRINT("Item not receiving data:");
			DPRINTLN(i);
			sendRREF(_listRefs[i]);
			break;
		}
	}

	return _LastError;
}
bool XpUDP::isRunning()
{
	return _isRunning;
}
//==================================================================================================
// create new data referece link with X-Plane
//
// int XpUDP::registerDataRef(int iFreq, const char *sCode ){
///==================================================================================================
int XpUDP::registerDataRef(int iFreq, const CanasXplaneTrans* newItem, bool recieveOnly)
{
	bool notFound = true;
	int curRecord = -1;
	_DataRefs* newRef;
	_DataRefs* tmpRef;

	DPRINT("RegisterDataRef:START:");

	// first test if we already have the item.
	for (int i = 0; i < _listRefs.size(); i++)
	{
		tmpRef = _listRefs[i];
		if (strcmp(tmpRef->paramInfo->xplaneId, newItem->xplaneId) == 0)
		{
			notFound = false;
			curRecord = i;
			break;
		}
	}

	if (curRecord == -1)
	{
		//not found so create new item
		DPRINT("New record");
		newRef = new _DataRefs;
		_listRefs.push_back(newRef);
		newRef->refID = _listRefs.size();
		newRef->paramInfo = newItem;
		//strcpy(newRef->command, newItem->xplaneId);
		//newRef->setdata = callBack;
	}
	else
	{
		// found a record so update item
		DPRINT("existing reccord");
		newRef = _listRefs[curRecord];
	}

	newRef->recieveOnly = recieveOnly;

	// check if active and for last timestamp
	if ((newRef->recieveOnly) && (!newRef->subscribed || (millis() - newRef->timestamp) > _MAX_INTERVAL || newRef->frequency != iFreq))
	{
		// create new request
		DPRINT("create x-plane request");
		newRef->subscribed = true;
		newRef->frequency = iFreq;
		sendRREF(newRef);
		// read first returned value
		dataReader();
	}

	if (!newRef->recieveOnly)
	{
		strcpy(newRef->sendStruct.dref_path, newItem->xplaneId);
	}

	return (int)XpUDP::SUCCESS;
}

//==================================================================================================
// remove a data referece link with X-Plane
//
// int XpUDP::unRegisterDataRef(const char *sCode )
///==================================================================================================
int XpUDP::unRegisterDataRef(const char * sCode)
{
	// TODO: unRegisterDataRef Code
	return 0;
}

int XpUDP::unRegisterDataRef(uint16_t canID)
{
	bool notFound = true;
	int curRecord = -1;
	_DataRefs* newRef;
	_DataRefs* tmpRef;

	DPRINT("RegisterDataRef:START:");

	// first test if we have the item.
	for (int i = 0; i < _listRefs.size(); i++)
	{
		tmpRef = _listRefs[i];
		if (tmpRef->canId == canID)
		{
			notFound = false;
			curRecord = i;
			tmpRef->active = false;
			newRef->frequency = 0;
			sendRREF(newRef);
			break;
		}
	}

	return 0;
}
///==================================================================================================
// send a updat to a xref to x-plane
///==================================================================================================
int XpUDP::setDataRefValue(uint16_t canID, float value)
{
	// TODO: Code send data to Xplane
	_DataRefs* tmpRef;
	int curRecord = -1;

	// first test if we have the item.
	for (int i = 0; i < _listRefs.size(); i++)
	{
		tmpRef = _listRefs[i];
		if (tmpRef->canId == canID)
		{
			curRecord = i;
			break;
		}
	}

	if (curRecord != -1)
	{
		tmpRef->value = value;
		sendDREF(tmpRef);
	}
	else
		return -1;

	return 0;
}

///==================================================================================================
// send a data request to x-plane
///==================================================================================================
int XpUDP::sendRREF(int frequency, int refID, char* xplaneId)
{
	// fill Xplane struct array
	_dref_struct_in.dref_freq = frequency;
	_dref_struct_in.dref_en = refID;
	strcpy(_dref_struct_in.dref_string, xplaneId);

	sendUDPdata(XPMSG_RREF, (byte *)&_dref_struct_in, sizeof(_dref_struct_in), _Xp_dref_in_msg_size);
}
//==================================================================================================
//==================================================================================================
int XpUDP::sendRREF(_DataRefs* newRef)
{
	DPRINT("Sendinf New Ref:");
	DPRINT(newRef->refID);
	DPRINT(":");
	DPRINTLN(newRef->paramInfo->xplaneId);

	sendRREF(newRef->frequency, newRef->refID, newRef->paramInfo->xplaneId);
}
//==================================================================================================
//==================================================================================================
int XpUDP::sendDREF(_DataRefs * newRef)
{
	newRef->sendStruct.var = newRef->value;

	sendUDPdata(XPMSG_DREF, (byte *)&(newRef->sendStruct), sizeof(_dref_struct), _Xp_dref_size);
	return 0;
}
//==================================================================================================
//	Send a packet to X-Plane
//==================================================================================================
void XpUDP::sendUDPdata(const char *header, const byte *dataArr, const int arrSize, const int sendSize)
{
	DPRINTLN("sending XPlane packet...");
#if defined(ARDUINO_ARCH_ESP8266)
	ESP.wdtFeed();
#endif
#ifdef ARDUINO_ARCH_ESP32
	esp_task_wdt_reset();
#endif

	// set all bytes in the buffer to 0
	memset(_packetBuffer, 0, _UDP_MAX_PACKET_SIZE);
	// Initialize values needed to form request to x-plane

	memcpy(_packetBuffer, header, 4);
	memcpy((_packetBuffer)+5, dataArr, arrSize);

	DPRINT("Header:");
	DPRINTLN((char *)_packetBuffer);
	DPRINTBUFFER(_packetBuffer, sendSize);
	//printBuffer(_packetBuffer);
	DPRINT("Sending to:");
	DPRINT(_XPlaneIP);
	DPRINT(": Port:");
	DPRINTLN(_XpListenPort);
	DPRINT("Sending size:");
	DPRINTLN(sendSize);

	_Udp.beginPacket(_XPlaneIP, _XpListenPort);
	_Udp.write(_packetBuffer, sendSize);
	_Udp.endPacket();
}
//==================================================================================================
//
//==================================================================================================