//============================================================================
// Name        : Novad.h
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
// Description : Nova Daemon to perform network anti-reconnaissance
//============================================================================

#ifndef NOVAD_H_
#define NOVAD_H_

#include "HashMapStructs.h"
#include "Evidence.h"
#include "Suspect.h"
#include "protobuf/marshalled_classes.pb.h"

#include <arpa/inet.h>
#include <vector>
#include <string>

//Mode to knock on the silent alarm port
#define OPEN true
#define CLOSE false


//Hash table for current list of suspects
typedef Nova::HashMap<in_addr_t, ANNpoint, std::hash<in_addr_t>, eqaddr > lastPointHash;


namespace Nova
{

int RunNovaD();

void StartServer();

// Locks to ensure only one instance of novad running
bool LockNovad();

// Send a silent alarm
//		suspect - Suspect to send alarm about
void SilentAlarm(Suspect *suspect, int oldClassification);

// Knocks on the port of the neighboring nova instance to open or close it
//		mode - true for OPEN, false for CLOSE
bool KnockPort(bool mode);

// Receive featureData from another local component.
// This is a blocking function. If nothing is received, then wait on this thread for an answer
// Returns: false if any sort of error
bool Start_Packet_Handler();

void StartCapture();
void StopCapture();
void StopCapture_noLocking();

// Do any cleanup needed before exit when in training mode
void CloseTrainingCapture();

// Force a reload of NOVAConfig/Data.txt while running.
// This will reclassify all the suspects based on the new data.
void Reload();

// Parse through the honeyd config file and get the list of IP addresses used
//		honeyDConfigPath - path to honeyd configuration file
// Returns: vector containing IP addresses of all honeypots
std::vector <std::string> GetHaystackAddresses(std::string honeyDConfigPath);

// Reads in a file containing one IP per line and # prefix for comments
//	returns - vector representation of the IP addresses in the file
std::vector <std::string> GetIpAddresses(std::string honeyDConfigPath);

std::string ConstructFilterString(std::string captureIdentifier);

// Callback function that is passed to pcap_loop(..) and called each time a packet is received
//		useless - Unused
//		pkthdr - pcap packet header
//		packet - packet data
void Packet_Handler(u_char *useless,const struct pcap_pkthdr *pkthdr,const u_char *packet);

// Masks the kill signals of a thread so they will get
// sent to the main thread's signal handler.
void MaskKillSignals();

//Logs and prints if any packets were dropped since the last time this was called
void CheckForDroppedPackets();

// Call this to update the featuresets based on a haystack change
void UpdateHaystackFeatures();

}

#endif /* NOVAD_H_ */
