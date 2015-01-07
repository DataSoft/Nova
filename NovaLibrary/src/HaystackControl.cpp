//============================================================================
// Name        : HaystackControl.cpp
// Copyright   : DataSoft Corporation 2011-2013
//	Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Controls the Honeyd Haystack and Doppelganger processes
//============================================================================

#include <sstream>
#include <boost/algorithm/string.hpp>

#include "HaystackControl.h"
#include "NovaUtil.h"
#include "Config.h"
#include "Logger.h"


using namespace std;

namespace Nova
{
bool StartHaystack(bool blocking)
{
	stringstream ss;
	ss << "sudo honeyd ";
	Config::Inst()->LoadInterfaces();
	vector<string> ifList = Config::Inst()->GetInterfaces();
	while(!ifList.empty())
	{
		ss << " -i " << ifList.back();
		ifList.pop_back();
	}
	ss << " --disable-webserver";
	ss << " -i " << Config::Inst()->GetDoppelInterface();
	ss << " -f " << "\"" << Config::Inst()->GetPathHome() << "/";
	ss << Config::Inst()->GetPathConfigHoneydHS() << "\"";


	ss << " -p " << "\"" << Config::Inst()->GetPathShared();
	ss << "/nmap-os-db\" -s /var/log/honeyd/honeydHaystackservice.log -t /var/log/honeyd/ipList";
	ss << " -m \"" << Config::Inst()->GetPathShared() << "/nmap-mac-prefixes\"";

	// Are there user specified additional arguements to send to honeyd?
	string additionalArgs = Config::Inst()->GetAdditionalHoneydArgs();
	boost::trim(additionalArgs);
	if (additionalArgs.size()) {
		ss << " " << additionalArgs;
	}

	if (blocking)
	{
		ss << " -d";
	}

	LOG(DEBUG, "Launching haystack with command: " + ss.str(), "");
	if(system(ss.str().c_str()) != 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool StopHaystack()
{
	// Kill honeyd processes
	FILE *out = popen("pidof honeyd","r");
	bool retSuccess = false;
	if(out != NULL)
	{
		char buffer[1024];
		char *line = fgets(buffer, sizeof(buffer), out);

		if(line != NULL)
		{
			string l(line);
			boost::trim(l);
			string cmd = "i=5; while [ $i -ne 0 ]; sudo kill " + l + "; if [ $? -ne 0 ]; then break; fi; i=$(expr $i - 1); do sleep 1; done";
			if(cmd.size() > 5)
			{
				if(system(cmd.c_str()) == EXIT_SUCCESS)
				{
					retSuccess = true;
				}
			}
		}
		else
		{
			//Haystack was already down
			retSuccess = true;
		}
	}
	pclose(out);
	return retSuccess;
}

bool IsHaystackUp()
{
	FILE *out = popen("pidof honeyd","r");
	bool retSuccess = false;
	if(out != NULL)
	{
		char buffer[1024];
		char *line = fgets(buffer, sizeof(buffer), out);

		if(line != NULL)
		{
			retSuccess = true;
		}
		else
		{
			retSuccess = false;
		}
	}
	pclose(out);
	return retSuccess;
}

}
