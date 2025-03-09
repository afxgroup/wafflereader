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

////////////////////////////////////////////////////////////////////////////////////////
// Class to manage the communication between the computer and a serial port           //
////////////////////////////////////////////////////////////////////////////////////////
//
//

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include "SerialIO.h"

#include <string>
#include <string.h>
#include <locale>
#include <iostream>
#include <algorithm>

// Constructor etc
SerialIO::SerialIO() {
}

SerialIO::~SerialIO() {
	m_ftdi.FT_Close();

	closePort();
}

// Returns TRUE if the port is open
bool SerialIO::isPortOpen() const {
	if (m_ftdi.isOpen()) return true;

	return false;
}

// Purge any data left in the buffer
void SerialIO::purgeBuffers() {
	if (!isPortOpen()) return;

	if (m_ftdi.isOpen()) {
		m_ftdi.FT_Purge(true, true);
		return;
	}
}

void SerialIO::purgeRxBuffer() {
	if (!isPortOpen()) return;

	if (m_ftdi.isOpen()) {
		m_ftdi.FT_PurgeRx();
		return;
	}
}

void SerialIO::purgeTxBuffer() {
	if (!isPortOpen()) return;

	if (m_ftdi.isOpen()) {
		m_ftdi.FT_PurgeTx();
		return;
	}
}

// Sets the status of the DTR line
void SerialIO::setRTS(bool enableRTS) {
	if (!isPortOpen()) return;

	if (m_ftdi.isOpen()) {
		if (enableRTS) m_ftdi.FT_SetRts(); else m_ftdi.FT_ClrRts();
		return;
	}
}

// Sets the status of the DTR line
void SerialIO::setDTR(bool enableDTR) {
	if (!isPortOpen()) return;

	if (m_ftdi.isOpen()) {
		if (enableDTR) m_ftdi.FT_SetDtr(); else m_ftdi.FT_ClrDtr();
		return;
	}
}

// Returns the status of the CTS pin
bool SerialIO::getCTSStatus() {
	if (!isPortOpen()) return false;

	if (m_ftdi.isOpen()) {
		uint32_t status;
		if (m_ftdi.FT_GetModemStatus(&status) != FTDI::FT_STATUS::FT_OK) return false;
		return (status & FT_MODEM_STATUS_CTS) != 0;
	}

	return false;
}

// Returns a list of serial ports discovered on the system
void SerialIO::enumSerialPorts(std::vector<SerialPortInformation>& serialPorts) {
	serialPorts.clear();

	// Add in the FTDI ports detected
	uint32_t numDevs;
	FTDI::FT_STATUS status = m_ftdi.FT_CreateDeviceInfoList(&numDevs);
	if ((status == FTDI::FT_STATUS::FT_OK) && (numDevs)) {
		FTDI::FT_DEVICE_LIST_INFO_NODE* devList = (FTDI::FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FTDI::FT_DEVICE_LIST_INFO_NODE) * numDevs);
		if (devList) {

			status = m_ftdi.FT_GetDeviceInfoList(devList, &numDevs);
			if (status == FTDI::FT_STATUS::FT_OK) {
				for (unsigned int index = 0; index < numDevs; index++) {
					SerialPortInformation info;
					info.instanceID = devList[index].LocId;
					info.pid = devList[index].ID & 0xFFFF;
					info.vid = devList[index].ID >> 16;
					info.productName = devList[index].Description;

					// Ensure no duplicate port numbers names
					int portIndex = 0;
					do {
						// Create name
						std::string tmp = FTDI_PORT_PREFIX;
						if (portIndex) tmp += std::to_string(portIndex) + "-";
						portIndex++;
						tmp += std::string(devList[index].SerialNumber);

						// Convert to wide
						info.portName = tmp;

						// Finish it
						//info.portName += " (" + info.productName + ")";
						info.ftdiIndex = index;

						// Test if one with this name already exists
					} while (std::find_if(serialPorts.begin(), serialPorts.end(), [&info](const SerialPortInformation& port)->bool {
						return (info.portName == port.portName);
					}) != serialPorts.end());

					// Save
					serialPorts.push_back(info);
				}
			}
			free(devList);
		}
	}

	std::sort(serialPorts.begin(), serialPorts.end(), [](const SerialPortInformation& a, const SerialPortInformation& b)->int {
		return a.portName < b.portName;
	});
}

// Attempt ot change the size of the buffers used by the OS
void SerialIO::setBufferSizes(const unsigned int rxSize, const unsigned int txSize) {
	if (!isPortOpen()) {
		return;
	}
	if (m_ftdi.isOpen()) {
		// Larger than this size actually causes slowdowns.  This doesn't work the same as below.  Below is a buffer in Windows.  This is on the USB device I think
		m_ftdi.FT_SetUSBParameters(rxSize < 256 ? 256 : rxSize, txSize);
		return;
	}
}

// Open a port by name
SerialIO::Response SerialIO::openPort(const std::string& portName) {
	closePort();

	if (portName.length() > std::string(FTDI_PORT_PREFIX).length()) {
		// Is it FTDI?
		if (portName.substr(0, strlen(FTDI_PORT_PREFIX)) == FTDI_PORT_PREFIX) {
			std::vector<SerialPortInformation> serialPorts;
			enumSerialPorts(serialPorts);

			// See if it exists
			auto f = std::find_if(serialPorts.begin(), serialPorts.end(), [&portName](const SerialPortInformation& serialport)-> bool {
				return portName == serialport.portName;
			});

			// was it found?
			if (f != serialPorts.end()) {
				FTDI::FT_STATUS s = m_ftdi.FT_Open(f->vid, f->pid);
				switch (s) {
					case FTDI::FT_STATUS::FT_OK: break;
					case FTDI::FT_STATUS::FT_DEVICE_NOT_OPENED: return Response::rInUse;
					case FTDI::FT_STATUS::FT_DEVICE_NOT_FOUND: return Response::rNotFound;
					default: return Response::rUnknownError;
				}
				updateTimeouts();

				return Response::rOK;
			} else return Response::rNotFound;
		}
	}

	return Response::rNotImplemented;
}

// Shuts the port down
void SerialIO::closePort() {
	if (!isPortOpen()) return;

	if (m_ftdi.isOpen()) {
		m_ftdi.FT_Close();
		return;
	}
}

// Changes the configuration on the port
SerialIO::Response SerialIO::configurePort(const Configuration& configuration) {
	if (!isPortOpen()) return Response::rUnknownError;

	if (m_ftdi.isOpen()) {
		if (m_ftdi.FT_SetFlowControl(configuration.ctsFlowControl ? FT_FLOW_RTS_CTS : FT_FLOW_NONE, 0, 0) != FTDI::FT_STATUS::FT_OK) return SerialIO::Response::rUnknownError;
		if (m_ftdi.FT_SetDataCharacteristics(FTDI::FT_BITS::_8, FTDI::FT_STOP_BITS::_1, FTDI::FT_PARITY::NONE) != FTDI::FT_STATUS::FT_OK) return SerialIO::Response::rUnknownError;
		if (m_ftdi.FT_SetBaudRate(configuration.baudRate) != FTDI::FT_STATUS::FT_OK) return SerialIO::Response::rUnknownError;
		m_ftdi.FT_SetLatencyTimer(2);
		m_ftdi.FT_ClrDtr();
		m_ftdi.FT_ClrRts();
		return SerialIO::Response::rOK;
	}

	return Response::rNotImplemented;
}

// Check if we wrre quick enough reading the data
bool SerialIO::checkForOverrun() {
	if (!isPortOpen()) return false;

	if (m_ftdi.isOpen()) {
		uint32_t status;
		if (m_ftdi.FT_GetModemStatus(&status) != FTDI::FT_STATUS::FT_OK) return false;
		return (status & (FT_MODEM_STATUS_OE | FT_MODEM_STATUS_FE)) != 0;
	}
	return false;
}

// Returns the number of bytes waiting to be read
unsigned int SerialIO::getBytesWaiting() {
	if (!isPortOpen()) return 0;

	if (m_ftdi.isOpen()) {
		uint32_t queueSize = 0;
		if (m_ftdi.FT_GetQueueStatus(&queueSize) != FTDI::FT_STATUS::FT_OK) return 0;
		return queueSize;
	}

	return 0;
}

// Attempts to write some data to the port.  Returns how much it actually wrote.
// If writeAll is not TRUE then it will write what it can until it times out
unsigned int SerialIO::write(const void* data, unsigned int dataLength) {
	if ((data == nullptr) || (dataLength == 0)) return 0;
	if (!isPortOpen()) return 0;

	if (m_ftdi.isOpen()) {
		m_ftdi.FT_SetTimeouts(m_readTimeout + (m_readTimeoutMultiplier * dataLength), m_writeTimeout + (m_writeTimeoutMultiplier * dataLength));

		uint32_t written = 0;
		if (m_ftdi.FT_Write((void*)data, dataLength, &written) != FTDI::FT_STATUS::FT_OK) written = 0;
		return written;
	}

	return 0;
}

// A very simple, uncluttered version of the below, mainly for linux
unsigned int SerialIO::justRead(void* data, unsigned int dataLength) {
	if ((data == nullptr) || (dataLength == 0)) return 0;
	if (!isPortOpen()) return 0;

	if (m_ftdi.isOpen()) {
		m_ftdi.FT_SetTimeouts(m_readTimeout + (m_readTimeoutMultiplier * dataLength), m_writeTimeout + (m_writeTimeoutMultiplier * dataLength));

		uint32_t dataRead = 0;
		if (m_ftdi.FT_Read((void*)data, dataLength, &dataRead) != FTDI::FT_STATUS::FT_OK) dataRead = 0;
		return dataRead;
	}

	return 0;

}

// Attempts to read some data from the port.  Returns how much it actually read.
// Returns how much it actually read
unsigned int SerialIO::read(void* data, unsigned int dataLength) {
	if ((data == nullptr) || (dataLength == 0)) 
		return 0;
	if (!isPortOpen()) 
		return 0;

	if (m_ftdi.isOpen()) {
		m_ftdi.FT_SetTimeouts(m_readTimeout + (m_readTimeoutMultiplier * dataLength), m_writeTimeout + (m_writeTimeoutMultiplier * dataLength));
		uint32_t dataRead = 0;
		if (m_ftdi.FT_Read((void*)data, dataLength, &dataRead) != FTDI::FT_STATUS::FT_OK) {
			dataRead = 0;
		}
		return dataRead;
	}

	return 0;
}

// Update timeouts
void SerialIO::updateTimeouts() {
	if (!isPortOpen()) return;

	m_ftdi.FT_SetTimeouts(m_readTimeout + (m_readTimeoutMultiplier), m_writeTimeout + (m_writeTimeoutMultiplier));
}

// Sets the read timeouts. The actual timeout is calculated as waitTimetimeout + (multiplier * num bytes)
void SerialIO::setReadTimeouts(unsigned int waitTimetimeout, unsigned int multiplier) {
	m_readTimeout = waitTimetimeout;
	m_readTimeoutMultiplier = multiplier;

	updateTimeouts();
}

// Sets the write timeouts. The actual timeout is calculated as waitTimetimeout + (multiplier * num bytes)
void SerialIO::setWriteTimeouts(unsigned int waitTimetimeout, unsigned int multiplier) {
	m_writeTimeout = waitTimetimeout;
	m_writeTimeoutMultiplier = multiplier;

	updateTimeouts();
}
