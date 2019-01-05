#ifndef __GENERICDEFS_H_
#define __GENERICDEFS_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define INI_FILE_NAME "/sdcard/setting.ini"

extern QueueHandle_t xQueueRREF;	// queu for results of data items from Xplane
extern QueueHandle_t xQueueDREF;	// queue for data items to send to XPlane
extern QueueHandle_t xQueueDataSet;  // queue to  request of stop dataitems in XPlane interface
//extern QueueHandle_t xQueueStatus;	// queue for status messages

// event groups
/* FreeRTOS event group to signal when we are connected*/
extern EventGroupHandle_t s_connection_event_group;

const int WIFI_CONNECTED_BIT = BIT0;
const int UDP_CONNECTED_BIT = BIT1;
const int BEACON_CONNECTED_BIT = BIT2;
const int CLEANING_CONNECTED_BIT = BIT3;
const int PLANE_CONNECTED_BIT = BIT4;
const int RUNNING_CONNECTED_BIT = BIT5;
const int RUNNING_CANBUS_BIT = BIT6;

typedef struct queueDataSetItem {
	uint16_t	canId;
	int			interval = 1;
	bool		send = false;
};

typedef struct queueDataItem {
	uint16_t	canId;
	float		value;
};

#endif
