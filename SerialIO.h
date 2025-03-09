#ifndef DISKREADERWRITER_SERIAL_IO
#define DISKREADERWRITER_SERIAL_IO
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

#include <string>
#include <vector>

#include <termios.h>

#include "ftdi_impl.h"

#define FTDI_PORT_PREFIX "FTDI:"

class SerialIO {
private:
	unsigned int m_readTimeout = 0, m_readTimeoutMultiplier = 0;
	unsigned int m_writeTimeout = 0, m_writeTimeoutMultiplier = 0;
	FTDI::FTDIInterface m_ftdi;

	// Update timeouts
	void updateTimeouts();

public:
	// Definition of a serial port
	struct SerialPortInformation {
		// The "file" name of the port
		std::string portName;
		// Port details
		unsigned int vid = 0, pid = 0;
		// Product name
		std::string productName;
		// Instance ID
		std::string instanceID;
		// Set if this is an FTDI device index
		int ftdiIndex = -1;
	};

	// Configuration settings for the port
	struct Configuration {
		unsigned int baudRate	= 9600;		
		bool ctsFlowControl		= false;
	};

	enum class Response { rOK, rInUse, rNotFound, rUnknownError, rNotImplemented };

	// Constructor etc
	SerialIO();
	virtual ~SerialIO();

	// Sets the status of the DTR line
	void setDTR(bool enableDTR);

	// Sets the status of the RTS line
	void setRTS(bool enableRTS);

	// Returns the status of the CTS pin
	bool getCTSStatus();

	// Returns TRUE if the port is open
	bool isPortOpen() const;

	// Returns a list of serial ports discovered on the system
	void enumSerialPorts(std::vector<SerialPortInformation>& serialPorts);

	// Purge any data left in the buffer
	void purgeBuffers();
	void purgeRxBuffer();
	void purgeTxBuffer();

	// Returns the number of bytes waiting to be read
	unsigned int getBytesWaiting();

	// Check if we were quick enough reading the data
	bool checkForOverrun();

	// Open a port by name
	Response openPort(const std::string& portName);

	// Shuts the port down
	void closePort();

	// Attempt ot change the size of the buffers used by the OS
	void setBufferSizes(const unsigned int rxSize, const unsigned int txSize);

	// Changes the configuration on the port
	Response configurePort(const Configuration& configuration);

	// Attempts to write some data to the port.  Returns how much it actually wrote.
	unsigned int write(const void* data, unsigned int dataLength);

	// Attempts to read some data from the port.  Returns how much it actually read.
	// Returns how mcuh it actually read
	unsigned int read(void* data, unsigned int dataLength);

	// A very simple, uncluttered version of the above, mainly for linux
	unsigned int justRead(void* data, unsigned int dataLength);

	// Sets the read timeouts. The actual timeout is calculated as waitTimetimeout + (multiplier * num bytes)
	void setReadTimeouts(unsigned int waitTimetimeout, unsigned int multiplier);

	// Sets the write timeouts. The actual timeout is calculated as waitTimetimeout + (multiplier * num bytes)
	void setWriteTimeouts(unsigned int waitTimetimeout, unsigned int multiplier);
};
 

#endif