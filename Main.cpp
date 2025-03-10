/* DRAWBRIDGE aka ArduinoFloppyReader (and writer)
 *
 * Copyright (C) 2017-2022 Robert Smith (@RobSmithDev)
 * https://amiga.robsmithdev.co.uk
 *
 * This program is free software; you can redistribute it and/or
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
 * License along with this program; if not see http://www.gnu.org/licenses/
 */

////////////////////////////////////////////////////////////////////////////////////////////
// Example console application for reading and writing floppy disks to and from ADF files //
////////////////////////////////////////////////////////////////////////////////////////////
#include "ADFWriter.h"
#include "ArduinoInterface.h"

#include "common.hpp"

#include <proto/exec.h>
#include <exec/types.h>
#include <proto/dos.h>

static const char __attribute__((used)) *version = "$VER: Waffle Copy Professional 2.8.8 for AmigaOS4 (" __DATE__ ")";

using namespace ArduinoFloppyReader;

extern ADFWriter writer;

int main(int argc, char *argv[])
{
	// Define the template for ReadArgs
	const char *argsTemplate = "COMPORT/K,FILE/K,WRITE/S,VERIFY/S,NOBANNER/S,LISTSERIALS/S,DIAGNOSTIC/S,CLEAN/S,SETTINGS/S,SETTINGNAME/K,SETTINGVALUE/S";
	struct RDArgs *rdargs;
	std::string settingName;
	std::string filename;
	std::string port;

	struct
	{
		STRPTR comport;
		STRPTR file;
		LONG write;
		LONG verify;
		LONG nobanner;
		LONG listSerials;
		LONG diagnostic;
		LONG clean;
		LONG settings;
		STRPTR settingsName;
		LONG settingsValue;
	} shell_args;
	memset(&shell_args,0,sizeof(shell_args));
	
	// Read the arguments
	rdargs = ReadArgs(argsTemplate, (LONG*) &shell_args, NULL);
	if (!rdargs)
	{
		PrintFault(IoErr(),NULL);
		return 0;
	}

	/* If NOBANNER is specified avoid to print all informations */
	if (!shell_args.nobanner)
	{
		printf("DrawBridge aka Arduino Floppy Disk Reader/Writer V2.8.8, Copyright (C) 2017-2022 Robert Smith\n");
		printf("Full sourcecode and documentation at https://amiga.robsmithdev.co.uk\n");
		printf("This is free software licenced under the GNU General Public Licence V3\n");
		printf("AmigaOS4 version by Andrea Palmate' - os4test@amigasoft.net\n");
	}

	/* Print serial ports and exit */
	if (shell_args.listSerials)
	{
		std::vector<std::string> portList;
		ArduinoFloppyReader::ArduinoInterface::enumeratePorts(portList);
		if (portList.size() > 0)
		{
			for (const std::string &port : portList)
				printf("%s\n", port.c_str());
		}
		if (rdargs)
		{
			FreeArgs(rdargs);
		}
		return 0;
	}

	/* Now check for required parameters */
	if (shell_args.comport == NULL)
	{
        if (!shell_args.nobanner)
            printf("\n");
		printf("Usage: %s %s\n", argv[0], argsTemplate);
		if (rdargs)
		{
			FreeArgs(rdargs);
		}
		return 0;
	}
	else
	{
		port = shell_args.comport;
	}

	if (!shell_args.settings && !shell_args.diagnostic && !shell_args.clean) {
		if (shell_args.file == NULL)
		{
			printf("No file specified.\n");
			if (rdargs)
			{
				FreeArgs(rdargs);
			}
			return 0;
		}
		else
		{
			filename = shell_args.file;
		}
	}

	/* User ask for diagnostic */
	if (shell_args.diagnostic)
	{
		runDiagnostics(port);
		if (rdargs)
		{
			FreeArgs(rdargs);
		}
		return 0;
	}

	if (shell_args.clean)
	{
		runCleaning(port);
		if (rdargs)
		{
			FreeArgs(rdargs);
		}
		return 0;
	}

	if (shell_args.settings)
	{

		if (argc && shell_args.settingsName)
		{
			std::string settingName = shell_args.settingsName;
			programmeSetting(port, settingName, shell_args.settingsValue);
			return 0;
		}

		listSettings(port);
		if (rdargs)
		{
			FreeArgs(rdargs);
		}
		return 0;
	}

	if (!writer.openDevice(port))
	{
		printf("\rError opening COM port: %s  ", writer.getLastError().c_str());
	}
	else
	{
		if (shell_args.write)
			file2Disk(filename.c_str(), shell_args.verify);
		else
			disk2file(filename.c_str());

		writer.closeDevice();
	}
	printf("\n");
	
	if (rdargs)
	{
		FreeArgs(rdargs);
	}
	return 0;
}
