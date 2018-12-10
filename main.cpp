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
#undef CONFIG_AUTOSTART_ARDUINO
#define LOG_LOCAL_LEVEL 5

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include <stdlib.h>
#include <QList.h>
#include <esp32_can.h>
//#include "GenericDefs.h"
#include "XinstrumentsMaster.h"
#include "mydebug.h"
#include "XpUDP.h"
#include <Can2XPlane.h>
#include <ICanBus.h>

//#include <WiFi.h>
//#include <WiFiType.h>
//#include <WiFiMulti.h>
//#include <ETH.h>

//const char* ssid = "OnzeStek";
//const char* password = "MijnLief09";
#define EXAMPLE_ESP_WIFI_SSID      "OnzeStek"
#define EXAMPLE_ESP_WIFI_PASS      "MijnLief09"
#define EXAMPLE_ESP_MAXIMUM_RETRY  100

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
/* The event group allows multiple bits for each event, but we only care about one event
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

bool _WifiIsConnected = false;

static const char *TAG = "InstrumentMaster";

static int s_retry_num = 0;
// end wifi

XpUDP *myXpInterface = NULL;
ICanBus	*myCANbus = NULL;

#define DEB_TM_STEP 50000 // test only
long tmLastBeacon = 0;
long sendVal = 10;  // test only
long timerVal = DEB_TM_STEP; // testonly;

QueueHandle_t xQueueRREF = NULL;
QueueHandle_t xQueueDREF = NULL;
QueueHandle_t xQueueDataSet = NULL;
QueueHandle_t xQueueStatus = NULL;

#if DEBUG_LEVEL > 0

short debugGetState()
{
	short state = XpUDP::getState();
	//DLVARPRINTLN(1, "RState:", state);
	return state;
}

#endif

//-------------------------------------------------------------------------------------------------
// main task wrappers
//-------------------------------------------------------------------------------------------------
extern "C" void taskXplane(void* parameter)
{
	for (;;)
	{
		myXpInterface->dataReader();
	}
}
//-------------------------------------------------------------------------------------------------
extern "C" void taskCanbus(void* parameter)
{
	for (;;)
	{
		myCANbus->UpdateMaster();
	}
}

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
void mySetup()
{
	IPAddress ipOwn;

	// define all needed inter task queues;
	xQueueRREF = xQueueCreate(20, sizeof(queueDataItem));
	assert(xQueueRREF != NULL);
	xQueueDREF = xQueueCreate(20, sizeof(queueDataItem));
	assert(xQueueDREF != NULL);
	xQueueDataSet = xQueueCreate(20, sizeof(queueDataSetItem));
	assert(xQueueDataSet != NULL);
	xQueueStatus = xQueueCreate(5, sizeof(uint32_t));
	assert(xQueueStatus != NULL);

	// setup wifi connection
		//TODO: Store wifi details in perm memory
		//TODO: use wifi  autosetup for first connect
		  // Connect to WiFi network
	DLPRINTINFO(1, "START");

	//DLVARPRINTLN(1, "Connecting to:", ssid);

	//WiFi.mode(WIFI_STA);

	//WiFi.printDiag(Serial);

	/*WiFi.begin(ssid, password);

	wl_status_t wifiResult;

	do
	{
		wifiResult = WiFi.status();
		delay(500);
		DLPRINT(1, ".");
		DLVARPRINTLN(2, "wifi status:", wifiResult);
	} while (wifiResult != WL_CONNECTED);

	DLPRINTLN(1, "<");
	*/

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

	DLPRINTLN(1, "starting Tasks");
	// create main tasks
	xTaskCreatePinnedToCore(taskXplane, "taskXPlane", 1000, NULL, 1, NULL, 0);
	xTaskCreatePinnedToCore(taskCanbus, "taskCanBus", 1000, NULL, 1, NULL, 1);

	// run forever :-)
	DLPRINTLN(1, "Running");

	DLPRINTINFO(2, "STOP");
}
//-------------------------------------------------------------------------------------------------
// Wifi Stuff
//-------------------------------------------------------------------------------------------------
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	//DLPRINTINFO(1, "START");
	//ESP_EARLY_LOGI(TAG, "running on core:%d", xPortGetCoreID());
	esp_task_wdt_reset();

	switch (event->event_id)
	{
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		_WifiIsConnected = true;
		ESP_EARLY_LOGI(TAG, "got ip:%s",
			ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
	{
		if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
		{
			esp_wifi_connect();
			xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
			s_retry_num++;
			ESP_EARLY_LOGI(TAG, "retry to connect to the AP");
			DLPRINTLN(1, "retry to connect to the AP");
		}
		ESP_EARLY_LOGI(TAG, "connect to the AP fail\n");
		break;
	}
	default:
		break;
	}
	//DLPRINTINFO(1, "STOP");
	return ESP_OK;
}

extern "C" void wifi_init_sta()
{
	DLPRINTINFO(1, "START");
	s_wifi_event_group = xEventGroupCreate();

	esp_task_wdt_init(30, false);

	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	wifi_config_t wifi_config;
	/*= {
		.sta = {
			.ssid = EXAMPLE_ESP_WIFI_SSID,
			.password = EXAMPLE_ESP_WIFI_PASS
		},
	};*/

	strncpy((char*)(wifi_config.sta.ssid), EXAMPLE_ESP_WIFI_SSID, sizeof(EXAMPLE_ESP_WIFI_SSID));
	strncpy((char*)(wifi_config.sta.password), EXAMPLE_ESP_WIFI_PASS, sizeof(EXAMPLE_ESP_WIFI_PASS));

	DLVARPRINTLN(1, "SSID:", (char*)(wifi_config.sta.ssid));
	DLVARPRINTLN(1, "PWD:", (char*)(wifi_config.sta.password));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_EARLY_LOGI(TAG, "WIFI Start");
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_EARLY_LOGI(TAG, "wifi_init_sta finished.");
	ESP_EARLY_LOGI(TAG, "connect to ap SSID:%s password:%s",
		EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);

	DLPRINTINFO(1, "STOP");
}

//-------------------------------------------------------------------------------------------------
// main
//-------------------------------------------------------------------------------------------------

extern "C" void app_main()
{
	// start up serial interface
	Serial.begin(115200);
	delay(10);

	esp_log_level_set("*", ESP_LOG_INFO);        // set all components to ERROR level
	esp_log_level_set("InstrumentMaster", ESP_LOG_VERBOSE);        // set all components to ERROR level

	ESP_EARLY_LOGI(TAG, "STARTING MAIN");

	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	ESP_EARLY_LOGI(TAG, "NVS Status %d", ret);

	//vTaskStartScheduler();

	// setup WiFi
	ESP_EARLY_LOGI(TAG, "ESP_WIFI_MODE_STA");

	wifi_init_sta();

	ESP_EARLY_LOGI(TAG, "Waiting for connect");
	while (xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, 100) != WIFI_CONNECTED_BIT);
	//while (!_WifiIsConnected);
	ESP_EARLY_LOGI(TAG, "Got connection");

	ESP_EARLY_LOGI(TAG, "start Setup");
	mySetup();
	ESP_EARLY_LOGI(TAG, "starting Tasks");
	// create main tasks
	xTaskCreatePinnedToCore(taskXplane, "taskXPlane", 1000, NULL, 1, NULL, 0);
	xTaskCreatePinnedToCore(taskCanbus, "taskCanBus", 1000, NULL, 1, NULL, 1);
	ESP_EARLY_LOGI(TAG, "Run State entered");
	//	for (;;) esp_task_wdt_reset();
}