//==================================================================================================
//  Franks Flightsim Intruments project
//  by Frank van Ierland
//
// This code is in the public domain.
//
//==================================================================================================
// Xinstruments.h
//
// version: 0.1
//
// Main configuration file to build main instruiment server
//
/// <remark>
//
///</remark>
//===================================================================================================================
/// \author  Frank van Ierland (frank@van-ierland.com) DO NOT CONTACT THE AUTHOR DIRECTLY: USE THE LISTS
// Copyright (C) 2018 Frank van Ierland

#ifndef _XINSTRUMENTS_MASTER_H_
#define _XINSTRUMENTS_MASTER_H_

#define XI_INSTRUMENT_MAIN_CONTROLER

#ifdef  XI_INSTRUMENT_MAIN_CONTROLER
#define XI_CONTROLER_MAIN
#endif

//-------------------------------------------------------------------------------------------------------------------
//
//  Settings for the OLIMEX ESP32-EVB controler
//
//-------------------------------------------------------------------------------------------------------------------

#ifdef XI_CONTROLER_MAIN
constexpr uint8_t XI_Hardware_Revision = 0XF1; 

#define XI_CANBUS_TX  5
#define XI_CANBUS_RX 35

#define XI_MAX_STEPPERS 0
#define XI_MAX_SERVO    0
#define XI_MAX_HAL      0
#define XI_MAX_RE       0


#endif

//===================================================================================================================
// generic defines
//===================================================================================================================
constexpr uint8_t XI_Software_Revision = 0x01;

//constexpr uint8_t XI_Base_NodeID = 1;

#define XI_INSTRUMENT_MAIN_CONTROLER

constexpr char XI_Instrument_Code[] = "MAIN_";
constexpr uint8_t XI_Instrument_NodeID = 1;
constexpr uint8_t XI_Instrument_Service_Chan = 100;
constexpr uint8_t XI_Instrument_Redun_Chan = 0;

#endif


