/* ArduinoFloppyReader (and writer)
*
* Copyright (C) 2017-2022 Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 3 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this library; if not, see http://www.gnu.org/licenses/
*/

/*
  This is a FTDI driver class, based on the FTDI2XX.h file from FTDI.
  This library dynamically loads the DLL/SO rather than static linking to it.
*/
#include "ftdi_impl.h"

using namespace FTDI;

#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <ftdi.h>

// Purge rx and tx buffers
#define FT_PURGE_RX         1
#define FT_PURGE_TX         2

#define CALLING_CONVENTION

typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_Open)(int deviceNumber, FT_HANDLE *pHandle) ;
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_OpenEx)(void* pArg1, uint32_t Flags, FT_HANDLE *pHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_ListDevices)( void* pArg1, void* pArg2, uint32_t Flags);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_Close)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_GetComPortNumber)(FT_HANDLE ftHandle, int32_t* lplComPortNumber);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_Read)(FT_HANDLE ftHandle, void* lpBuffer, uint32_t nBufferSize, uint32_t* lpBytesReturned);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_Write)(FT_HANDLE ftHandle, void* lpBuffer, uint32_t nBufferSize, uint32_t* lpBytesWritten);	 
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_IoCtl)(FT_HANDLE ftHandle, uint32_t dwIoControlCode, void* lpInBuf, uint32_t nInBufSize, void* lpOutBuf, uint32_t nOutBufSize, uint32_t* lpBytesReturned, void* lpOverlapped);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetBaudRate)(FT_HANDLE ftHandle, uint32_t BaudRate);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetDivisor)(FT_HANDLE ftHandle, uint16_t Divisor);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetDataCharacteristics)(FT_HANDLE ftHandle, unsigned char WordLength, unsigned char StopBits, unsigned char Parity);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetFlowControl)(FT_HANDLE ftHandle, uint16_t FlowControl, unsigned char XonChar, unsigned char XoffChar);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_ResetDevice)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetDtr)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_ClrDtr)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetRts)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_ClrRts)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_GetModemStatus)(FT_HANDLE ftHandle,uint32_t *pModemStatus);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetChars)(FT_HANDLE ftHandle,unsigned char EventChar,unsigned char EventCharEnabled,unsigned char ErrorChar,unsigned char ErrorCharEnabled);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_Purge)(FT_HANDLE ftHandle,uint32_t Mask);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetTimeouts)(FT_HANDLE ftHandle,uint32_t ReadTimeout,uint32_t WriteTimeout);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_GetQueueStatus)(FT_HANDLE ftHandle,uint32_t *dwRxBytes);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetEventNotification)(FT_HANDLE ftHandle,uint32_t Mask,void* Param);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_GetEventStatus)(FT_HANDLE ftHandle,uint32_t *dwEventDWord);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_GetStatus)(FT_HANDLE ftHandle,uint32_t *dwRxBytes,uint32_t *dwTxBytes,uint32_t *dwEventDWord);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetBreakOn)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetBreakOff)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetWaitMask)(FT_HANDLE ftHandle,uint32_t Mask);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_WaitOnMask)(FT_HANDLE ftHandle,uint32_t *Mask);

typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_CreateDeviceInfoList)(uint32_t* lpdwNumDevs);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_GetDeviceInfoList)(FT_DEVICE_LIST_INFO_NODE* pDest, uint32_t* lpdwNumDevs);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_GetDeviceInfoDetail)(uint32_t dwIndex, uint32_t* lpdwFlags, uint32_t* lpdwType, uint32_t* lpdwID, uint32_t* lpdwLocId, char* pcSerialNumber, char* pcDescription, FT_HANDLE* ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_GetDriverVersion)(FT_HANDLE ftHandle, uint32_t* lpdwDriverVersion);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_GetLibraryVersion)(uint32_t* lpdwDLLVersion);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_ResetPort)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_CyclePort)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_GetComPortNumber)(FT_HANDLE ftHandle, int32_t* port);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_SetUSBParameters)(FT_HANDLE ftHandle, uint32_t dwInTransferSize, uint32_t dwOutTransferSize);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_SetLatencyTimer)(FT_HANDLE ftHandle, unsigned char ucTimer);

_FT_Open	FT_Open = nullptr;
_FT_OpenEx	FT_OpenEx = nullptr;
_FT_ListDevices	FT_ListDevices = nullptr;
_FT_Close	FT_Close = nullptr;
_FT_Read	FT_Read = nullptr;
_FT_Write	FT_Write = nullptr;
_FT_IoCtl 	FT_IoCtl = nullptr;
_FT_SetBaudRate	FT_SetBaudRate = nullptr;
_FT_SetDivisor	FT_SetDivisor = nullptr;
_FT_SetDataCharacteristics	FT_SetDataCharacteristics = nullptr;
_FT_SetFlowControl	FT_SetFlowControl = nullptr;
_FT_ResetDevice	FT_ResetDevice = nullptr;
_FT_SetDtr	FT_SetDtr = nullptr;
_FT_ClrDtr	FT_ClrDtr = nullptr;
_FT_SetRts	FT_SetRts = nullptr;
_FT_ClrRts	FT_ClrRts = nullptr;
_FT_GetModemStatus	FT_GetModemStatus = nullptr;
_FT_SetChars	FT_SetChars = nullptr;
_FT_Purge	FT_Purge = nullptr;
_FT_SetTimeouts	FT_SetTimeouts = nullptr;
_FT_GetQueueStatus	FT_GetQueueStatus = nullptr;
_FT_SetEventNotification	FT_SetEventNotification = nullptr;
_FT_GetEventStatus	FT_GetEventStatus = nullptr;
_FT_GetStatus	FT_GetStatus = nullptr;
_FT_SetBreakOn	FT_SetBreakOn = nullptr;
_FT_SetBreakOff	FT_SetBreakOff = nullptr;
_FT_SetWaitMask	FT_SetWaitMask = nullptr;
_FT_WaitOnMask	FT_WaitOnMask = nullptr;

_FT_CreateDeviceInfoList FT_CreateDeviceInfoList = nullptr;
_FT_GetDeviceInfoList FT_GetDeviceInfoList = nullptr;
_FT_GetDeviceInfoDetail FT_GetDeviceInfoDetail = nullptr;
_FT_GetDriverVersion FT_GetDriverVersion = nullptr;
_FT_GetLibraryVersion FT_GetLibraryVersion = nullptr;
_FT_ResetPort FT_ResetPort = nullptr;
_FT_CyclePort FT_CyclePort = nullptr;
_FT_GetComPortNumber FT_GetComPortNumber = nullptr;
_FT_SetUSBParameters FT_SetUSBParameters = nullptr;
_FT_SetLatencyTimer FT_SetLatencyTimer = nullptr;

void* m_dll = nullptr;


FTDIInterface::FTDIInterface() {
	open = true;

	if (libraryLoadCounter == 0) {
		if (ftdi_init(&ftdic) < 0)     {
			fprintf(stderr, "ftdi_init failed\n");
			open = false;
			return;
		}
	}
	libraryLoadCounter++;
}

FTDIInterface::~FTDIInterface() {
	if (open) {
		libraryLoadCounter--;
		if (devlist != nullptr) {
			ftdi_list_free(&devlist);
			devlist = nullptr;
		}
		if (ftdi_usb_close(&ftdic) < 0)
		{
			fprintf(stderr, "unable to close ftdi device: (%s)\n", ftdi_get_error_string(&ftdic));
		}
		if (libraryLoadCounter == 0) {
        	ftdi_deinit(&ftdic);
		}
		open = false;
	}
}

FTDI::FT_STATUS FTDIInterface::FT_Open(int vid, int pid) {
	if (open)
        ftdi_usb_close(&ftdic);

	int ret = ftdi_usb_open(&ftdic, vid, pid);
	if (ret < 0) {
		return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE; // TODO - Convert libftdi errors to ftdi2xx errors
	}
	else {
		if (ftdi_usb_reset(&ftdic)) {
        	fprintf(stderr, "Unable to reset ftdi device: %s\n", ftdi_get_error_string(&ftdic));
			return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE; // TODO - Convert libftdi errors to ftdi2xx errors
		}
		ftdi_usb_purge_buffers(&ftdic);
		ftdi_set_bitmode(&ftdic, 0x00, BITMODE_RESET);

		return FTDI::FT_STATUS::FT_OK;
	}
#if 0
	FT_Close(); 

	if (::FT_Open) {
		FTDI::FT_STATUS status = ::FT_Open(deviceNumber, &m_handle);
		if (status != FTDI::FT_STATUS::FT_OK) m_handle = 0;
		return status;
	}
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_OpenEx(void* pArg1, uint32_t Flags) { 
#if 0
	if (::FT_OpenEx) {
		FTDI::FT_STATUS status = ::FT_OpenEx(pArg1, Flags, &m_handle);
		if (status != FTDI::FT_STATUS::FT_OK) m_handle = 0;
		return status;
	}
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_ListDevices(void* pArg1, void* pArg2, uint32_t Flags) { 
#if 0
	if (::FT_ListDevices) return ::FT_ListDevices(pArg1, pArg2, Flags);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_Close() {
	if (open)
        ftdi_usb_close(&ftdic);
	return FTDI::FT_STATUS::FT_OK;
};

FTDI::FT_STATUS FTDIInterface::FT_Read(void* lpBuffer, uint32_t nBufferSize, uint32_t* lpBytesReturned) { 
	int32_t ret = ftdi_read_data(&ftdic, (unsigned char*) lpBuffer, nBufferSize);
	if (ret < 0) {
		*lpBytesReturned = 0;
		return FTDI::FT_STATUS::FT_IO_ERROR;
	}
	else {
		*lpBytesReturned = ret;
		return FTDI::FT_STATUS::FT_OK;		
	}
};

FTDI::FT_STATUS FTDIInterface::FT_Write(void* lpBuffer, uint32_t nBufferSize, uint32_t* lpBytesWritten) { 
	int32_t ret = ftdi_write_data(&ftdic, (unsigned char*) lpBuffer, nBufferSize);
	if (ret < 0) {
		*lpBytesWritten = 0;
		return FTDI::FT_STATUS::FT_IO_ERROR;
	}
	else {
		*lpBytesWritten = ret;
		return FTDI::FT_STATUS::FT_OK;		
	}
};

FTDI::FT_STATUS FTDIInterface::FT_IoCtl(uint32_t dwIoControlCode, void* lpInBuf, uint32_t nInBufSize, void* lpOutBuf, uint32_t nOutBufSize, uint32_t* lpBytesReturned, void* lpOverlapped) { 
#if 0
	if ((::FT_IoCtl) && (m_handle)) return ::FT_IoCtl(m_handle, dwIoControlCode, lpInBuf, nInBufSize, lpOutBuf, nOutBufSize, lpBytesReturned, lpOverlapped);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetBaudRate(uint32_t BaudRate) { 
	int ret = ftdi_set_baudrate(&ftdic, BaudRate);
	if (ret == 0)
		return FTDI::FT_STATUS::FT_OK;
	else
		return FTDI::FT_STATUS::FT_INVALID_ARGS;
#if 0
	if ((::FT_SetBaudRate) && (m_handle)) return ::FT_SetBaudRate(m_handle, BaudRate);
#endif
};

FTDI::FT_STATUS FTDIInterface::FT_SetDivisor(uint16_t Divisor) { 
#if 0
	if ((::FT_SetDivisor) && (m_handle)) return ::FT_SetDivisor(m_handle, Divisor);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetDataCharacteristics(FT_BITS WordLength, FT_STOP_BITS StopBits, FT_PARITY Parity) {
	ftdi_bits_type bits;
	ftdi_stopbits_type sbits;
	ftdi_parity_type pbits;

	switch (WordLength)
	{
		case FTDI::FT_BITS::_7:
			bits = BITS_7;
		break;
		case FTDI::FT_BITS::_8:
		default:
			bits = BITS_8;
		break;
	}

	switch (StopBits)
	{
		case FTDI::FT_STOP_BITS::_1:
			sbits = STOP_BIT_1;
		break;
		case FTDI::FT_STOP_BITS::_1_5:
			sbits = STOP_BIT_15;
		break;			
		case FTDI::FT_STOP_BITS::_2:
			sbits = STOP_BIT_2;
		break;			
		default:
			sbits = STOP_BIT_1;
		break;
	}

	switch (Parity)
	{
		case FTDI::FT_PARITY::NONE:
			pbits = NONE;
		break;
		case FTDI::FT_PARITY::ODD:
			pbits = ODD;
		break;
		case FTDI::FT_PARITY::MARK:
			pbits = MARK;
		break;
		case FTDI::FT_PARITY::SPACE:
			pbits = SPACE;
		break;
		default:
			pbits = NONE;
			break;
	}

	int ret = ftdi_set_line_property(&ftdic, bits, sbits, pbits);
	if (ret == 0)
		return FTDI::FT_STATUS::FT_OK;
	else		
		return FTDI::FT_STATUS::FT_NOT_SUPPORTED;
#if 0
	if ((::FT_SetDataCharacteristics) && (m_handle)) return ::FT_SetDataCharacteristics(m_handle, (unsigned char)WordLength, (unsigned char)StopBits, (unsigned char)Parity);
#endif
};

FTDI::FT_STATUS FTDIInterface::FT_SetFlowControl(uint16_t FlowControl, unsigned char XonChar, unsigned char XoffChar) { 
	uint16_t fc = SIO_DISABLE_FLOW_CTRL;
	int ret = -1;
	if (FlowControl == FT_FLOW_NONE) {
		fc = SIO_DISABLE_FLOW_CTRL;
		ret = ftdi_setflowctrl(&ftdic, fc);
	}
	else if (FlowControl == FT_FLOW_RTS_CTS) {
		fc = SIO_RTS_CTS_HS;
		ret = ftdi_setflowctrl(&ftdic, fc);
	}
	else if (FlowControl == FT_FLOW_DTR_DSR) {
		fc = SIO_DTR_DSR_HS;
		ret = ftdi_setflowctrl(&ftdic, fc);
	}
	else if (FlowControl == FT_FLOW_XON_XOFF) {
		fc = SIO_XON_XOFF_HS;
		ret = ftdi_setflowctrl(&ftdic, fc);
		//ret = ftdi_setflowctrl_xonxoff(&ftdic, XonChar, XoffChar);
	}
	if (ret == 0)
		return FTDI::FT_STATUS::FT_OK;
	else		
		return FTDI::FT_STATUS::FT_NOT_SUPPORTED;
#if 0
	if ((::FT_SetFlowControl) && (m_handle)) return ::FT_SetFlowControl(m_handle, FlowControl, XonChar, XoffChar);
#endif
};

FTDI::FT_STATUS FTDIInterface::FT_ResetDevice() { 
	int ret = ftdi_usb_reset(&ftdic);
	if (ret == 0)
		return FTDI::FT_STATUS::FT_OK;
	else
		return FTDI::FT_STATUS::FT_IO_ERROR;
#if 0
	if ((::FT_ResetDevice) && (m_handle)) return ::FT_ResetDevice(m_handle);
#endif
};

FTDI::FT_STATUS FTDIInterface::FT_SetDtr() { 
	int ret = ftdi_setdtr(&ftdic, 1);
	if (ret == 0)
		return FTDI::FT_STATUS::FT_OK;
	else		
		return FTDI::FT_STATUS::FT_INVALID_ARGS;
#if 0
	if ((::FT_SetDtr) && (m_handle)) return ::FT_SetDtr(m_handle);
#endif
};

FTDI::FT_STATUS FTDIInterface::FT_ClrDtr() {
	int ret = ftdi_setdtr(&ftdic, 0);
	if (ret == 0)
		return FTDI::FT_STATUS::FT_OK;
	else		
		return FTDI::FT_STATUS::FT_INVALID_ARGS;
#if 0
	if ((::FT_ClrDtr) && (m_handle)) return ::FT_ClrDtr(m_handle);
#endif
};

FTDI::FT_STATUS FTDIInterface::FT_SetRts() { 
	int ret = ftdi_setrts(&ftdic, 1);
	if (ret == 0)
		return FTDI::FT_STATUS::FT_OK;
	else		
		return FTDI::FT_STATUS::FT_INVALID_ARGS;
#if 0
	if ((::FT_SetRts) && (m_handle)) return ::FT_SetRts(m_handle);
#endif
};

FTDI::FT_STATUS FTDIInterface::FT_ClrRts() { 
	int ret = ftdi_setrts(&ftdic, 0);
	if (ret == 0)
		return FTDI::FT_STATUS::FT_OK;
	else		
		return FTDI::FT_STATUS::FT_INVALID_ARGS;

#if 0
	if ((::FT_ClrRts) && (m_handle)) return ::FT_ClrRts(m_handle);
#endif
};

FTDI::FT_STATUS FTDIInterface::FT_GetModemStatus(uint32_t* pModemStatus) { 
	unsigned short status = 0;
	int ret = ftdi_poll_modem_status(&ftdic, &status);
	if (ret == 0) {
		*pModemStatus = status;
		return FTDI::FT_STATUS::FT_OK;
	}
	else		
		return FTDI::FT_STATUS::FT_INVALID_ARGS;
#if 0
	if ((::FT_GetModemStatus) && (m_handle)) return ::FT_GetModemStatus(m_handle, pModemStatus);
#endif
};

FTDI::FT_STATUS FTDIInterface::FT_SetChars(unsigned char EventChar, unsigned char EventCharEnabled, unsigned char ErrorChar, unsigned char ErrorCharEnabled) { 
#if 0
	if ((::FT_SetChars) && (m_handle)) return ::FT_SetChars(m_handle, EventChar, EventCharEnabled, ErrorChar, ErrorCharEnabled);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_Purge(bool purgeRX, bool purgeTX) { 
	if (purgeRX) ftdi_usb_purge_rx_buffer(&ftdic);
	if (purgeTX) ftdi_usb_purge_tx_buffer(&ftdic);
#if 0
	if ((::FT_Purge) && (m_handle)) return ::FT_Purge(m_handle, (purgeRX ? FT_PURGE_RX : 0) | (purgeTX ? FT_PURGE_TX : 0));
#endif
	return FTDI::FT_STATUS::FT_OK;
};

FTDI::FT_STATUS FTDIInterface::FT_PurgeRx() { 
	ftdi_usb_purge_rx_buffer(&ftdic);
	return FTDI::FT_STATUS::FT_OK;
};

FTDI::FT_STATUS FTDIInterface::FT_PurgeTx() { 
	ftdi_usb_purge_tx_buffer(&ftdic);
	return FTDI::FT_STATUS::FT_OK;
};

FTDI::FT_STATUS FTDIInterface::FT_SetTimeouts(uint32_t ReadTimeout, uint32_t WriteTimeout) { 
	ftdic.usb_write_timeout = WriteTimeout;
	ftdic.usb_read_timeout = ReadTimeout;
	return FTDI::FT_STATUS::FT_OK;
};

FTDI::FT_STATUS FTDIInterface::FT_GetQueueStatus(uint32_t* dwRxBytes) {
#if 0
	if ((::FT_GetQueueStatus) && (m_handle)) return ::FT_GetQueueStatus(m_handle, dwRxBytes);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetEventNotification(uint32_t Mask, void* Param) { 
#if 0
	if ((::FT_SetEventNotification) && (m_handle)) return ::FT_SetEventNotification(m_handle, Mask, Param);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_GetEventStatus(uint32_t* dwEventDWord) {
#if 0
	if ((::FT_GetEventStatus) && (m_handle)) return ::FT_GetEventStatus(m_handle, dwEventDWord);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_GetStatus(uint32_t* dwRxBytes, uint32_t* dwTxBytes, uint32_t* dwEventDWord) { 
#if 0
	if ((::FT_GetStatus) && (m_handle)) return ::FT_GetStatus(m_handle, dwRxBytes, dwTxBytes, dwEventDWord);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetBreakOn() { 
#if 0
	if ((::FT_SetBreakOn) && (m_handle)) return ::FT_SetBreakOn(m_handle);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetBreakOff() { 
#if 0
	if ((::FT_SetBreakOff) && (m_handle)) return ::FT_SetBreakOff(m_handle);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetWaitMask(uint32_t Mask) { 
#if 0
	if ((::FT_SetWaitMask) && (m_handle)) return ::FT_SetWaitMask(m_handle, Mask);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_WaitOnMask(uint32_t* Mask) { 
#if 0
	if ((::FT_WaitOnMask) && (m_handle)) return ::FT_WaitOnMask(m_handle, Mask);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_CreateDeviceInfoList(uint32_t* lpdwNumDevs) {
    int ret;

	if ((ret = ftdi_usb_find_all(&ftdic, &devlist, 0x0403, 0x6001)) < 0)
    {
        fprintf(stderr, "ftdi_usb_find_all failed: %d (%s)\n", ret, ftdi_get_error_string(&ftdic));
        return FT_STATUS::FT_INVALID_PARAMETER;
    }
    *lpdwNumDevs = ret;

	return FTDI::FT_STATUS::FT_OK;
};

FTDI::FT_STATUS FTDIInterface::FT_GetDeviceInfoList(FT_DEVICE_LIST_INFO_NODE* pDest, uint32_t* lpdwNumDevs) {
	int i = 0, ret;
    bool ok = true;

	for (curdev = devlist; curdev != NULL; i++) {
        char manufacturer[65], description[64] = {0}, serial[16] = {0};

        if ((ret = ftdi_usb_get_strings(&ftdic, curdev->dev, manufacturer, 65, description, 65, serial, 17)) < 0)
        {
            fprintf(stderr, "ftdi_usb_get_strings failed: %d (%s)\n", ret, ftdi_get_error_string(&ftdic));
            ok = false;
            break;
        }

		pDest[i].ID = (curdev->dev->descriptor.idVendor << 16) | (curdev->dev->descriptor.idProduct & 0xFFFF);
		pDest[i].LocId = curdev->dev->devnum;
        strncpy(pDest[i].Description, description, 64);
        strncpy(pDest[i].SerialNumber, serial, 16);
        pDest[i].ftHandle = curdev->dev;
        curdev = curdev->next;
    }
	*lpdwNumDevs = i;

	if (ok)
        return FTDI::FT_STATUS::FT_OK;
    else        
        return FTDI::FT_STATUS::FT_OTHER_ERROR;
};

FTDI::FT_STATUS FTDIInterface::FT_GetComPortNumber(FT_HANDLE handle, int32_t* port) {
#if 0
	if ((::FT_GetComPortNumber) && (handle)) return ::FT_GetComPortNumber(handle, port);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
}

FTDI::FT_STATUS FTDIInterface::FT_GetDeviceInfoDetail(uint32_t dwIndex, uint32_t* lpdwFlags, uint32_t* lpdwType, uint32_t* lpdwID, uint32_t* lpdwLocId, char* pcSerialNumber, char* pcDescription, FT_HANDLE* handle) {
#if 0
	if (::FT_GetDeviceInfoDetail) return ::FT_GetDeviceInfoDetail(dwIndex, lpdwFlags, lpdwType, lpdwID, lpdwLocId, pcSerialNumber, pcDescription, handle);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_GetDriverVersion(uint32_t* lpdwDriverVersion) {
#if 0
	if ((::FT_GetDriverVersion) && (m_handle)) return ::FT_GetDriverVersion(m_handle, lpdwDriverVersion);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_GetLibraryVersion(uint32_t* lpdwDLLVersion) {
#if 0
	if ((::FT_GetLibraryVersion) && (m_handle)) return ::FT_GetLibraryVersion(lpdwDLLVersion);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_ResetPort() {
#if 0
	if ((::FT_ResetPort) && (m_handle)) return ::FT_ResetPort(m_handle);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_CyclePort() {
#if 0
	if ((::FT_CyclePort) && (m_handle)) return ::FT_CyclePort(m_handle);
#endif
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetUSBParameters(uint32_t dwInTransferSize, uint32_t dwOutTransferSize) {
	if (ftdi_write_data_set_chunksize(&ftdic, dwInTransferSize) < 0) {
		return FTDI::FT_STATUS::FT_INVALID_PARAMETER;
	}
	if (ftdi_read_data_set_chunksize(&ftdic, dwOutTransferSize) < 0) {
		return FTDI::FT_STATUS::FT_INVALID_PARAMETER;
	}
#if 0
	if ((::FT_SetUSBParameters) && (m_handle)) return ::FT_SetUSBParameters(m_handle, dwInTransferSize, dwOutTransferSize);
#endif
	return FTDI::FT_STATUS::FT_OK;
}

FTDI::FT_STATUS FTDIInterface::FT_SetLatencyTimer(unsigned char ucTimer) {
	int ret = ftdi_set_latency_timer(&ftdic, ucTimer);
	if (ret == 0)
		return FTDI::FT_STATUS::FT_OK;
	else
		return FTDI::FT_STATUS::FT_INVALID_ARGS;
#if 0
	if ((::FT_SetLatencyTimer) && (m_handle)) return ::FT_SetLatencyTimer(m_handle, ucTimer);
#endif
}
