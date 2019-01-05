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
static const char *TAG = "InstrumentMaster";
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include <stdlib.h>
#include <QList.h>
#include <esp32_can.h>
//#include "GenericDefs.h"
#include "XinstrumentsMaster.h"
//#include "mydebug.h"
#include "XpUDP.h"
#include <Can2XPlane.h>
#include <ICanBus.h>
#include "GenericDefs.h"

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"

#include "IniFile.h"

//#define UDP_TEST  // test udp interface without canbus
#define CAN_TEST // testing can part without xplane

#ifdef CAN_TEST
#include "XpUDPtest.h"
#endif

//#include <WiFi.h>
//#include <WiFiType.h>
//#include <WiFiMulti.h>
//#include <ETH.h>

char _ssid[32];
char _password[32];
//#define EXAMPLE_ESP_WIFI_SSID      "OnzeStek"
//#define EXAMPLE_ESP_WIFI_PASS      "MijnLief09"
#define EXAMPLE_ESP_MAXIMUM_RETRY  100

/* The event group allows multiple bits for each event, but we only care about one event
 * - are we connected to the AP with an IP? */
EventGroupHandle_t s_connection_event_group;

static int s_retry_num = 0;
// end wifi

XpUDP *myXpInterface = NULL;
ICanBus	*myCANbus = NULL;
#ifdef CAN_TEST
XpUDPtest *myUPDtest = NULL;
#endif

#define DEB_TM_STEP 50000 // test only
long tmLastBeacon = 0;
long sendVal = 10;  // test only
long timerVal = DEB_TM_STEP; // testonly;

QueueHandle_t xQueueRREF = NULL;
QueueHandle_t xQueueDREF = NULL;
QueueHandle_t xQueueDataSet = NULL;
//static QueueHandle_t xQueueStatus = NULL;

#if DEBUG_LEVEL > 0

short debugGetState()
{
	short state = XpUDP::getState();
	//DLVARPRINTLN(1, "RState:", state);
	return state;
}

#endif
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// test function for XP interface
//-------------------------------------------------------------------------------------------------
#ifdef UDP_TEST
extern "C" void XP_testTask(void* parameter)
{
	queueDataSetItem dataItem;

	dataItem.interval = 1;

	ESP_LOGD(TAG, "Starting task test");

	for (int i = 200; i < 220; i++)
	{
		dataItem.canId = i;

		if (xQueueSendToBack(xQueueDataSet, &dataItem, 10) != pdTRUE)
			DO_LOGE("\nFailed to add new dataset %d", i);
		delay(20);
	}

	ESP_LOGD(TAG, "Task test is done");
	for (;;);
	vTaskDelete(NULL);
}
#endif
//-------------------------------------------------------------------------------------------------
// main task wrappers
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int initSD()
{
	int result = 0;
	sdmmc_host_t host = SDMMC_HOST_DEFAULT();

	// To use 1-line SD mode, uncomment the following line:
	host.flags = SDMMC_HOST_FLAG_1BIT;
	host.max_freq_khz = SDMMC_FREQ_PROBING;
	// This initializes the slot without card detect (CD) and write protect (WP) signals.
// Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
	sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

	// Options for mounting the filesystem.
	// If format_if_mount_failed is set to true, SD card will be partitioned and formatted
	// in case when mounting fails.
	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		.format_if_mount_failed = false,
		.max_files = 5
	};
	// Use settings defined above to initialize SD card and mount FAT filesystem.
	// Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
	// Please check its source code and implement error recovery when developing
	// production applications.
	sdmmc_card_t* card;
	esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
	if (ret != ESP_OK)
	{
		if (ret == ESP_FAIL)
		{
			ESP_LOGE(TAG, "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
		}
		else
		{
			ESP_LOGE(TAG, "Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
		}
		result = -1;
	}

	ESP_LOGI(TAG, "SD init ok");

	return result;
}

// function to get info from ini file
int ParseIniFile()
{
	const size_t bufferLen = 160;
	char buffer[bufferLen];
	int result = 0;
	ESP_LOGD(TAG, "start parse ini");

	IniFile ini(INI_FILE_NAME, "rb");

	delay(10);

	if (!ini.open())
	{
		ESP_LOGE(TAG, "Ini file does not exist");
		// Cannot do anything else
		return -1;
	}

	ESP_LOGV(TAG, "Inifile opened");

	// Fetch a value from a key which is present
	if (ini.getValue("network", "ssid", buffer, bufferLen))
	{
		ESP_LOGD(TAG, "setting ssid to %s", buffer);
		strcpy(_ssid, buffer);
	}
	else
	{
		ESP_LOGE(TAG, "Can not get info from file. Err= %d", ini.getError());
		result = -1;
	}

	if (ini.getValue("network", "password", buffer, bufferLen))
	{
		ESP_LOGD(TAG, "setting password to %s", buffer);
		strcpy(_password, buffer);
	}
	else
	{
		result = -1;
	}

	uint16_t val;
	if (ini.getValue("misc", "debug_level", buffer, bufferLen, val))
	{
		ESP_LOGD(TAG, "setting loglevel to %d", val);
		esp_log_level_set("*", (esp_log_level_t)val);
	};

	ini.close();
	ESP_LOGD(TAG, "parse ini done");

	return result;
}

// the setup function runs once when you press reset or power the board
void mySetup()
{
	//IPAddress ipOwn;

	// define all needed inter task queues;
	xQueueRREF = xQueueCreate(20, sizeof(queueDataItem));
	assert(xQueueRREF != NULL);
	xQueueDREF = xQueueCreate(20, sizeof(queueDataItem));
	assert(xQueueDREF != NULL);
	xQueueDataSet = xQueueCreate(50, sizeof(queueDataSetItem));
	assert(xQueueDataSet != NULL);

	//xQueueStatus = xQueueCreate(5, sizeof(uint32_t));
	//assert(xQueueStatus != NULL);

	// setup wifi connection
		//TODO: Store wifi details in perm memory
		//TODO: use wifi  autosetup for first connect
		  // Connect to WiFi network
	ESP_LOGV(TAG, "START");

	ESP_LOGV(TAG, "creating XPlane interface");

#ifdef CAN_TEST
	myUPDtest = new XpUDPtest();
#else
	myXpInterface = new XpUDP(INI_FILE_NAME);
#endif

	ESP_LOGV(TAG, "Creating Can Bus interface");
	myCANbus = new ICanBus(XI_Instrument_NodeID, XI_Hardware_Revision, XI_Software_Revision);
	assert(myCANbus != NULL);

	ESP_LOGV(TAG, "starting xp interface");
#ifdef CAN_TEST
	myUPDtest->start(0);
#else
	myXpInterface->start(0);
#endif

	// startup the CAN areospace bus
#ifndef UDP_TEST
	ESP_LOGV(TAG, "starting CanAs interface");
	// make sure right can pins for board aree set
	myCANbus->setCANPins(XI_CANBUS_RX, XI_CANBUS_TX);
	// start on differnt core as UDP bus so go for core 0
	myCANbus->start(XI_CANBUS_SPEED, 1);
#endif
	//DLPRINTLN(1, "starting Tasks");
	// create main tasks
	//xTaskCreatePinnedToCore(taskXplane, "taskXPlane", 1000, NULL, 1, NULL, 0);
	//xTaskCreatePinnedToCore(taskCanbus, "taskCanBus", 1000, NULL, 1, NULL, 1);

	// run forever :-)
	ESP_LOGV(TAG, "All Running");
}
//-------------------------------------------------------------------------------------------------
// Wifi Stuff
//-------------------------------------------------------------------------------------------------
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
	//DLPRINTINFO(1, "START");
	ESP_EARLY_LOGI(TAG, "running on core:%d", xPortGetCoreID());
	ESP_LOGI(TAG, "##running on core:%d", xPortGetCoreID());
	esp_task_wdt_reset();

	switch (event->event_id)
	{
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_EARLY_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_connection_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
	{
		if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
		{
			esp_wifi_connect();
			xEventGroupClearBits(s_connection_event_group, WIFI_CONNECTED_BIT);
			s_retry_num++;
			ESP_EARLY_LOGI(TAG, "retry to connect to the AP");
		}
		ESP_EARLY_LOGE(TAG, "connect to the AP fail\n");
		break;
	}
	default:
		break;
	}

	return ESP_OK;
}

extern "C" void wifi_init_sta()
{
	ESP_EARLY_LOGI(TAG, "Starting Wifi");

	ESP_ERROR_CHECK(false);  // TESTING

	s_connection_event_group = xEventGroupCreate();

	tcpip_adapter_init();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_LOGI(TAG, "Wifi init");

	wifi_config_t wifi_config;
	strncpy((char*)(wifi_config.sta.ssid), _ssid, sizeof(_ssid));
	strncpy((char*)(wifi_config.sta.password), _password, sizeof(_password));
	wifi_config.sta.bssid_set = 0;

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_EARLY_LOGI(TAG, "WIFI Start");

	ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
	ESP_EARLY_LOGI(TAG, "Event loop started");

	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_EARLY_LOGI(TAG, "wifi_init_sta finished.");
	ESP_EARLY_LOGI(TAG, "connect to ap SSID:%s password:%s",
		wifi_config.sta.ssid, wifi_config.sta.password);
}

//-------------------------------------------------------------------------------------------------
// main
//-------------------------------------------------------------------------------------------------

extern "C" void app_main()
{
	// start up serial interface
	Serial.begin(115200);
	//Serial.setDebugOutput(true);
	delay(1000); // make sure Serial is initialized

	ESP_EARLY_LOGE(TAG, "debug level=%d", LOG_LOCAL_LEVEL);
	//	ESP_EARLY_LOGI(TAG, "Trace to %s", ESP_APPTRACE_DEST_TRAX);
		//esp_log_set_vprintf(WriteFormatted);

	esp_log_level_set("*", ESP_LOG_DEBUG);
	esp_log_level_set("IniFile", ESP_LOG_ERROR);
	esp_log_level_set("ICanBus", ESP_LOG_DEBUG);
	esp_log_level_set("XpUDPtest", ESP_LOG_ERROR);

	esp_task_wdt_init(120, false);

	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
	{
		assert(nvs_flash_erase() == ESP_OK);
		ret = nvs_flash_init();
	}
	ESP_EARLY_LOGI(TAG, "NVS return is %d", ret);
	ESP_ERROR_CHECK(ret);

	//assert(ret== ESP_ERR_NOT_FOUND);

	//vTaskStartScheduler();

	// get info from inifile on sd card

	assert(initSD() == 0);
	ESP_EARLY_LOGI(TAG, "Init SD card done");

	assert(ParseIniFile() == 0);
	ESP_LOGV(TAG, "Parse INI file done");

	// in ini file parse general log level is set.
	// set some specific log levels for modules
	esp_log_level_set("wifi", ESP_LOG_ERROR);						// set all components to ERROR level
	esp_log_level_set("IniFile", ESP_LOG_INFO);
	esp_log_level_set("XpPlaneInfo", ESP_LOG_INFO);

	// setup WiFi
	ESP_EARLY_LOGI(TAG, "ESP_WIFI_MODE_STA");

	wifi_init_sta();

	ESP_EARLY_LOGI(TAG, "Waiting for connect");
	while (xEventGroupWaitBits(s_connection_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, 100) != WIFI_CONNECTED_BIT);

	ESP_EARLY_LOGI(TAG, "Got connection");

	ESP_EARLY_LOGI(TAG, "start Setup");
	ESP_LOGI(TAG, "start Setup **");
	mySetup();

	ESP_EARLY_LOGI(TAG, "starting Tasks");

#ifdef UDP_TEST
	xTaskCreatePinnedToCore(XP_testTask, "XPtest", 10000, NULL, 1, NULL, 1);
#endif

	ESP_EARLY_LOGI(TAG, "Run State entered");

	for (;;)
	{
		esp_task_wdt_reset(); delay(1000);
	}
}