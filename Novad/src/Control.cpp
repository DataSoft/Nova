//============================================================================
// Name        : Control.cpp
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
// Description : Set of functions Novad can call to perform tasks related to
//			controlling the Novad process and operation
//============================================================================

#include "Doppelganger.h"
#include "Control.h"
#include "Config.h"
#include "Logger.h"
#include "Novad.h"
#include "ClassificationEngine.h"
#include "Lock.h"

extern Nova::ClassificationEngine *engine;
extern pthread_t classificationLoopThread;

extern pthread_mutex_t shutdownClassificationMutex;
extern bool shutdownClassification;
extern pthread_cond_t shutdownClassificationCond;
extern bool classificationRunning;

using namespace std;

namespace Nova
{
void SaveAndExit(int param)
{	
	StopCapture();

	if(Config::Inst()->GetIsDmEnabled())
	{
		if(system("sudo iptables -F") == -1)
		{
			LOG(WARNING, "Failed to flush iptables rules", "Command sudo iptables -F failed");
		}
		if(system("sudo iptables -t nat -F") == -1)
		{
			LOG(WARNING, "Failed to flush nat table rules", "Command sudo iptables -t nat -F failed");
		}
		if(system("sudo iptables -t nat -X DOPP") == -1)
		{
			LOG(WARNING, "Failed to delete chain DOPP in nat table", "Command sudo iptables -t nat -X DOPP failed");
		}
		if(system(std::string("sudo route del " + Config::Inst()->GetDoppelIp()).c_str()) == -1)
		{
			LOG(WARNING, "Failed to delete Doppelganger route", "Command sudo route del " + (string)Config::Inst()->GetDoppelIp() + " failed");
		}
	}

	if(engine != NULL)
	{
		{
			Lock lock(&shutdownClassificationMutex);
			shutdownClassification = true;
		}
		pthread_cond_signal(&shutdownClassificationCond);

		pthread_cond_destroy(&shutdownClassificationCond);
		pthread_mutex_destroy(&shutdownClassificationMutex);

		delete engine;
	}
	annClose();
	LOG(ALERT, "Novad is exiting cleanly.", "");
	exit(EXIT_SUCCESS);
}
}
