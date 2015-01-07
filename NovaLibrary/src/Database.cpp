//============================================================================
// Name        : Database.cpp
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
// Description : Wrapper for adding entries to the SQL database
//============================================================================

#include "Config.h"
#include "Database.h"
#include "NovaUtil.h"
#include "Logger.h"
#include "ClassificationEngine.h"

#include <iostream>
#include <sstream>
#include <string>


using namespace std;

namespace Nova
{

Database *Database::m_instance = NULL;

int Database::callback(void *NotUsed, int argc, char **argv, char **azColName){
	int i;
	for(i=0; i<argc; i++){
		cout << azColName[i] << "=" << (argv[i] ? argv[i] : "NULL") << endl;
	}
	cout << endl;
	return 0;
}

Database *Database::Inst(std::string databaseFile)
{
	if(m_instance == NULL)
	{
		m_instance = new Database(databaseFile);
		m_instance->Connect();
	}
	return m_instance;
}

Database::Database(std::string databaseFile)
{
	pthread_mutex_init(&this->m_lock, NULL);
	if (databaseFile == "")
	{
		databaseFile = Config::Inst()->GetPathHome() + "/data/novadDatabase.db";
	}
	m_databaseFile = databaseFile;
	m_count = 0;
}

Database::~Database()
{
	Disconnect();
}

void Database::StartTransaction()
{
	pthread_mutex_lock(&m_lock);
	sqlite3_exec(db, "BEGIN", 0, 0, 0);
}

void Database::StopTransaction()
{
	sqlite3_exec(db, "COMMIT", 0, 0, 0);
	pthread_mutex_unlock(&m_lock);
}

void Database::Connect()
{
	LOG(DEBUG, "Opening database " + m_databaseFile, "");
	int res;
	SQL_RUN(SQLITE_OK, sqlite3_open(m_databaseFile.c_str(), &db));

	// Enable foreign keys (useful for keeping database consistency)
	char *err = NULL;

	sqlite3_exec(db, "PRAGMA foreign_keys = ON", NULL, NULL, &err);
	if (err != NULL)
		LOG(ERROR, "Error when trying to enable foreign database keys: " + string(err), "");

	sqlite3_exec(db, "PRAGMA cache_size = 100000", NULL, NULL, &err);
	if (err != NULL)
		LOG(ERROR, "Error when trying to set cache_size: " + string(err), "");

	sqlite3_exec(db, "PRAGMA synchronous = OFF", NULL, NULL, &err);
	if (err != NULL)
		LOG(ERROR, "Error when setting pragma: " + string(err), "");

	sqlite3_exec(db, "PRAGMA temp_store = MEMORY", NULL, NULL, &err);
	if (err != NULL)
		LOG(ERROR, "Error when setting pragma: " + string(err), "");

	// Set up all our prepared queries
	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"INSERT INTO packet_counts VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?);",
		-1, &insertPacketCount,  NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"UPDATE packet_counts SET "
		" count_tcp = count_tcp + ?3"
		", count_udp = count_udp + ?4"
		", count_icmp = count_icmp + ?5"
		", count_other = count_other + ?6"
		", count_total = count_total + ?7"
		", count_tcpRst = count_tcpRst + ?8"
		", count_tcpAck = count_tcpAck + ?9"
		", count_tcpSyn = count_tcpSyn + ?10"
		", count_tcpFin = count_tcpFin + ?11"
		", count_tcpSynAck = count_tcpSynAck + ?12"
		", count_bytes = count_bytes + ?13"
		" WHERE ip = ?1 AND interface = ?2;",
		-1, &incrementPacketCount,  NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"INSERT OR IGNORE INTO suspects (ip, interface, startTime, endTime, lastTime, classification, hostileNeighbors, isHostile, classificationNotes) "
		"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
		-1, &insertSuspect,  NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"UPDATE suspects "
		" SET startTime = ?3, endTime = ?4, lastTime = ?5, mac = ?6"
		" WHERE ip = ?1 AND interface = ?2",
		-1, &updateSuspectTimestamps,  NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"INSERT INTO ip_port_counts VALUES(?1, ?2, ?3, ?4, ?5, 1)",
		-1, &insertPortContacted, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"UPDATE ip_port_counts SET count = count + ?6 WHERE ip = ?1 AND interface = ?2 AND type = ?3 AND dstip = ?4 AND port = ?5",
		-1, &incrementPortContacted, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"INSERT INTO packet_sizes VALUES(?1, ?2, ?3, ?4);",
		-1, &insertPacketSize,  NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"UPDATE packet_sizes SET count = count + ?4 WHERE ip = ?1 AND interface = ?2 AND packetSize = ?3;",
		-1, &incrementPacketSize,  NULL));

	// Ugh! I hate having features as columns, but trying to do EAV when sqlite doesn't have PIVOT makes the queries a pain (and slow),
	// and we can't blob them into a single column since we need to be able to sort by them individually. So, we're stuck with binding
	// 14+ doubles into the prepared statement. Urgh...
	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"UPDATE suspects SET"
		" ip_traffic_distribution = ?1"
		", port_traffic_distribution = ?2"
		", packet_size_mean = ?3"
		", packet_size_deviation = ?4"
		", distinct_ips = ?5"
		", distinct_tcp_ports = ?6"
		", distinct_udp_ports = ?7"
		", avg_tcp_ports_per_host = ?8"
		", avg_udp_ports_per_host = ?9"
		", tcp_percent_syn = ?10"
	    ", tcp_percent_fin = ?11"
		", tcp_percent_rst = ?12"
		", tcp_percent_synack = ?13"
		", haystack_percent_contacted = ?14"
		" WHERE ip = ?15 AND interface = ?16",
		-1, &setFeatureValues, NULL));


	// Queries for featureset computation
	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"SELECT (1.0*packet_counts.count_bytes) / (1.0*SUM(packet_sizes.count))"
		" FROM packet_counts INNER JOIN packet_sizes"
		" ON packet_counts.ip = packet_sizes.ip AND packet_counts.interface = packet_sizes.interface"
		" AND packet_counts.ip = ?1"
		" AND packet_counts.interface = ?2;",
		-1, &computePacketSizeMean, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"SELECT SUM((packet_sizes.count)*(packet_sizes.packetSize-?3)*(packet_sizes.packetSize-?3))/(1.0*packet_counts.count_total)"
		" FROM packet_counts INNER JOIN packet_sizes"
		" ON packet_counts.ip = packet_sizes.ip AND packet_counts.interface = packet_sizes.interface"
		" AND packet_counts.ip = ?1"
		" AND packet_counts.interface = ?2",
		-1, &computePacketSizeVariance, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"SELECT COUNT(DISTINCT dstip) FROM ip_port_counts WHERE ip = ?1 AND interface = ?2",
		-1, &computeDistinctIps, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"SELECT COUNT(DISTINCT port) FROM ip_port_counts WHERE ip = ?1 AND interface = ?2 AND type = ?3",
		-1, &computeDistinctPorts, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"SELECT COUNT(*) FROM (SELECT DISTINCT port, dstip FROM ip_port_counts WHERE ip = ?1 AND interface = ?2 AND type = ?3);",
		-1, &computeDistinctIpPorts, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"SELECT * FROM packet_counts WHERE ip = ?1 AND interface = ?2;",
		-1, &selectPacketCounts, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		" SELECT MAX(t) FROM (SELECT dstip, SUM(count) AS t FROM ip_port_counts WHERE ip = ?1 AND interface = ?2 GROUP BY dstip);",
		-1, &computeMaxPacketsToIp, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		" SELECT MAX(t) FROM (SELECT port, SUM(count) AS t FROM ip_port_counts WHERE ip = ?1 AND interface = ?2 AND TYPE = ?3 GROUP BY port);",
		-1, &computeMaxPacketsToPort, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"INSERT OR IGNORE INTO honeypots VALUES(?1, ?2)",
		-1, &insertHoneypotIp, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
			"DELETE FROM honeypots",
			-1, &clearHoneypots, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"SELECT 1.0*(SELECT COUNT(DISTINCT honeypots.ip)"
		" FROM honeypots, ip_port_counts "
		" WHERE ip_port_counts.ip = ?1 AND ip_port_counts.interface = ?2 AND honeypots.ip = ip_port_counts.dstip AND ip_port_counts.interface = honeypots.interface) / (1.0*(SELECT COUNT(ip) FROM honeypots WHERE interface = ?2));",
		-1, &computeHoneypotsContacted, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"UPDATE suspects "
		" SET classification = ?3, classificationNotes = ?4, hostileNeighbors = ?5, isHostile = ?6 "
		" WHERE ip = ?1 AND interface = ?2",
		-1, &updateClassification, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"INSERT INTO suspect_alerts (ip, interface, mac, startTime, endTime, lastTime, classification, hostileNeighbors, isHostile, classificationNotes, ip_traffic_distribution, port_traffic_distribution, packet_size_mean, packet_size_deviation, distinct_ips, distinct_tcp_ports, distinct_udp_ports, avg_tcp_ports_per_host, avg_udp_ports_per_host, tcp_percent_syn, tcp_percent_fin, tcp_percent_rst, tcp_percent_synack, haystack_percent_contacted) "
		" SELECT * FROM suspects"
		" WHERE ip = ? AND interface = ?",
		-1 , &createHostileAlert, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"SELECT isHostile, classification FROM suspects WHERE ip = ? AND interface = ?",
		-1, &isSuspectHostile, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"SELECT count_total FROM packet_counts WHERE ip = ? AND interface = ?",
		-1, &getTotalPackets, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,
		"SELECT ip, interface FROM suspects WHERE isHostile = 1",
		-1, &getHostileSuspects, NULL));
}

void Database::WriteClassification(Suspect *s)
{
	int res;

	SQL_RUN(SQLITE_OK, sqlite3_bind_text(updateClassification, 1, s->GetIpString().c_str(), -1, SQLITE_TRANSIENT));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(updateClassification, 2, s->GetInterface().c_str(), -1, SQLITE_TRANSIENT));
	SQL_RUN(SQLITE_OK, sqlite3_bind_double(updateClassification, 3, s->GetClassification()));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(updateClassification, 4, s->m_classificationNotes.c_str(), -1, SQLITE_TRANSIENT));
	SQL_RUN(SQLITE_OK, sqlite3_bind_int(updateClassification, 5, s->GetHostileNeighbors()));
	SQL_RUN(SQLITE_OK, sqlite3_bind_int(updateClassification, 6, s->GetIsHostile()));

	m_count++;
	SQL_RUN(SQLITE_DONE,sqlite3_step(updateClassification));
	SQL_RUN(SQLITE_OK, sqlite3_reset(updateClassification));
}

void Database::WriteTimestamps(Suspect *s)
{
	int res;

	SQL_RUN(SQLITE_OK, sqlite3_bind_text(updateSuspectTimestamps, 1, s->GetIpString().c_str(), -1, SQLITE_TRANSIENT));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(updateSuspectTimestamps, 2, s->GetInterface().c_str(), -1, SQLITE_TRANSIENT));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(updateSuspectTimestamps, 3, static_cast<long int>(s->m_features.m_startTime)));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(updateSuspectTimestamps, 4, static_cast<long int>(s->m_features.m_endTime)));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(updateSuspectTimestamps, 5, static_cast<long int>(s->m_features.m_lastTime)));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(updateSuspectTimestamps, 6, s->m_lastMac));

	m_count++;
	SQL_RUN(SQLITE_DONE,sqlite3_step(updateSuspectTimestamps));
	SQL_RUN(SQLITE_OK, sqlite3_reset(updateSuspectTimestamps));
}

vector<SuspectID_pb> Database::GetHostileSuspects()
{
	int res;

	vector<SuspectID_pb> hostiles;

	m_count++;
	res = sqlite3_step(getHostileSuspects);

	while (res == SQLITE_ROW)
	{
		SuspectID_pb id;

		id.set_m_ip(inet_network((const char*)sqlite3_column_text(getHostileSuspects, 0)));
		id.set_m_ifname(string((const char*)sqlite3_column_text(getHostileSuspects, 1)));

		hostiles.push_back(id);

		res = sqlite3_step(getHostileSuspects);
	}

	SQL_RUN(SQLITE_OK, sqlite3_reset(getHostileSuspects));



	return hostiles;

}

uint64_t Database::GetTotalPacketCount(const string &ip, const string &interface)
{
	int res;
	uint64_t packets = 0;

	SQL_RUN(SQLITE_OK, sqlite3_bind_text(getTotalPackets, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(getTotalPackets, 2, interface.c_str(), -1, SQLITE_STATIC));

	m_count++;
	res = sqlite3_step(getTotalPackets);

	if (res == SQLITE_ROW)
	{
		packets = sqlite3_column_int64(getTotalPackets, 0);
	}

	SQL_RUN(SQLITE_OK, sqlite3_reset(getTotalPackets));

	return packets;
}

vector<double> Database::ComputeFeatures(const string &ip, const string &interface)
{
	int res;

	// Create someplace to store our computed results
	vector<double> featureValues(DIM, 0);

	// Compute the packet size mean
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computePacketSizeMean, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computePacketSizeMean, 2, interface.c_str(), -1, SQLITE_STATIC));

	m_count++;
	SQL_RUN(SQLITE_ROW, sqlite3_step(computePacketSizeMean));
	featureValues[PACKET_SIZE_MEAN] = sqlite3_column_double(computePacketSizeMean, 0);

	SQL_RUN(SQLITE_OK, sqlite3_reset(computePacketSizeMean));


	// Compute the packet size deviation
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computePacketSizeVariance, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computePacketSizeVariance, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_double(computePacketSizeVariance, 3, featureValues[PACKET_SIZE_MEAN]));

	// SQL query gives us the variance, take sqrt to get the standard deviation
	m_count++;
	SQL_RUN(SQLITE_ROW, sqlite3_step(computePacketSizeVariance));
	featureValues[PACKET_SIZE_DEVIATION] = sqrt(sqlite3_column_double(computePacketSizeVariance, 0));

	SQL_RUN(SQLITE_OK, sqlite3_reset(computePacketSizeVariance));


	// Compute the distinct IPs contacted
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIps, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIps, 2, interface.c_str(), -1, SQLITE_STATIC));

	m_count++;
	SQL_RUN(SQLITE_ROW, sqlite3_step(computeDistinctIps));
	featureValues[DISTINCT_IPS] = sqlite3_column_double(computeDistinctIps, 0);

	SQL_RUN(SQLITE_OK, sqlite3_reset(computeDistinctIps));


	// Compute the distinct TCP ports contacted
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctPorts, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctPorts, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctPorts, 3, "tcp", -1, SQLITE_STATIC));

	m_count++;
	SQL_RUN(SQLITE_ROW, sqlite3_step(computeDistinctPorts));
	featureValues[DISTINCT_TCP_PORTS] = sqlite3_column_double(computeDistinctPorts, 0);

	SQL_RUN(SQLITE_OK, sqlite3_reset(computeDistinctPorts));


	// Compute the distinct UDP ports contacted
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctPorts, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctPorts, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctPorts, 3, "udp", -1, SQLITE_STATIC));

	m_count++;
	SQL_RUN(SQLITE_ROW, sqlite3_step(computeDistinctPorts));
	featureValues[DISTINCT_UDP_PORTS] = sqlite3_column_double(computeDistinctPorts, 0);

	SQL_RUN(SQLITE_OK, sqlite3_reset(computeDistinctPorts));


	if (featureValues[DISTINCT_IPS] > 0) {
		// Compute the average TCP ports per host
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIpPorts, 1, ip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIpPorts, 2, interface.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIpPorts, 3, "tcp", -1, SQLITE_STATIC));

		m_count++;
		SQL_RUN(SQLITE_ROW, sqlite3_step(computeDistinctIpPorts));
		featureValues[AVG_TCP_PORTS_PER_HOST] = sqlite3_column_double(computeDistinctIpPorts, 0) / featureValues[DISTINCT_IPS];

		SQL_RUN(SQLITE_OK, sqlite3_reset(computeDistinctIpPorts));


		// Compute the average UDP ports per host
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIpPorts, 1, ip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIpPorts, 2, interface.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeDistinctIpPorts, 3, "udp", -1, SQLITE_STATIC));

		m_count++;
		SQL_RUN(SQLITE_ROW, sqlite3_step(computeDistinctIpPorts));
		featureValues[AVG_UDP_PORTS_PER_HOST] = sqlite3_column_double(computeDistinctIpPorts, 0) / featureValues[DISTINCT_IPS];

		SQL_RUN(SQLITE_OK, sqlite3_reset(computeDistinctIpPorts));
	}




	SQL_RUN(SQLITE_OK, sqlite3_bind_text(selectPacketCounts, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(selectPacketCounts, 2, interface.c_str(), -1, SQLITE_STATIC));

	double totalTcpPackets = 0, synPackets = 0, finPackets = 0, rstPackets = 0, synAckPackets = 0, totalPackets = 0;

	m_count++;
	res = sqlite3_step(selectPacketCounts);

	if (res == SQLITE_ROW)
	{
		totalTcpPackets = sqlite3_column_int64(selectPacketCounts, 2);
		totalPackets = sqlite3_column_int64(selectPacketCounts, 6);
		rstPackets = sqlite3_column_int64(selectPacketCounts, 7);
		synPackets = sqlite3_column_int64(selectPacketCounts, 9);
		finPackets = sqlite3_column_int64(selectPacketCounts, 10);
		synAckPackets = sqlite3_column_int64(selectPacketCounts, 11);
	}
	else
	{
		LOG(ERROR, "Unable to get packet counts from the database for suspect: " + ip + "/" + interface, "");
	}

	SQL_RUN(SQLITE_OK, sqlite3_reset(selectPacketCounts));

	if (totalTcpPackets != 0)
	{
		featureValues[TCP_PERCENT_SYN] = synPackets / totalTcpPackets;
		featureValues[TCP_PERCENT_FIN] = finPackets / totalTcpPackets;
		featureValues[TCP_PERCENT_RST] = rstPackets / totalTcpPackets;
		featureValues[TCP_PERCENT_SYNACK] = synAckPackets / totalTcpPackets;
	}


	if (featureValues[DISTINCT_IPS] > 0) {
		// Compute the "ip traffic distribution", which is basically just (TotalSuspectPackets/TotalSuspectDstIps)/maxPacketsToSingleDstIp
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToIp, 1, ip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToIp, 2, interface.c_str(), -1, SQLITE_STATIC));

		m_count++;
		SQL_RUN(SQLITE_ROW, sqlite3_step(computeMaxPacketsToIp));
		featureValues[IP_TRAFFIC_DISTRIBUTION] = totalPackets / featureValues[DISTINCT_IPS] / sqlite3_column_double(computeMaxPacketsToIp, 0);

		SQL_RUN(SQLITE_OK, sqlite3_reset(computeMaxPacketsToIp));

		if ((featureValues[DISTINCT_TCP_PORTS] + featureValues[DISTINCT_UDP_PORTS]) > 0)
		{
			// Compute the "port traffic distribution", which is basically just (TotalSuspectPackets/TotalSuspectDstPorts)/maxPacketsToSingleDstPort
			SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToPort, 1, ip.c_str(), -1, SQLITE_STATIC));
			SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToPort, 2, interface.c_str(), -1, SQLITE_STATIC));
			SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToPort, 3, "tcp", -1, SQLITE_STATIC));

			m_count++;
			SQL_RUN(SQLITE_ROW, sqlite3_step(computeMaxPacketsToPort));
			double maxPacketsToTcpPort = sqlite3_column_double(computeMaxPacketsToPort, 0);
			SQL_RUN(SQLITE_OK, sqlite3_reset(computeMaxPacketsToPort));

			SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToPort, 1, ip.c_str(), -1, SQLITE_STATIC));
			SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToPort, 2, interface.c_str(), -1, SQLITE_STATIC));
			SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeMaxPacketsToPort, 3, "udp", -1, SQLITE_STATIC));

			m_count++;
			SQL_RUN(SQLITE_ROW, sqlite3_step(computeMaxPacketsToPort));
			double maxPacketsToUdpPort = sqlite3_column_double(computeMaxPacketsToPort, 0);
			SQL_RUN(SQLITE_OK, sqlite3_reset(computeMaxPacketsToPort));

			featureValues[PORT_TRAFFIC_DISTRIBUTION] = totalPackets
					/ (featureValues[DISTINCT_TCP_PORTS] + featureValues[DISTINCT_UDP_PORTS])
					/ max(maxPacketsToTcpPort, maxPacketsToUdpPort);
		}


		// Compute the haystack percent contacted
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeHoneypotsContacted, 1, ip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK, sqlite3_bind_text(computeHoneypotsContacted, 2, interface.c_str(), -1, SQLITE_STATIC));
		m_count++;
		SQL_RUN(SQLITE_ROW, sqlite3_step(computeHoneypotsContacted));
		featureValues[HAYSTACK_PERCENT_CONTACTED] = sqlite3_column_double(computeHoneypotsContacted, 0);
		SQL_RUN(SQLITE_OK, sqlite3_reset(computeHoneypotsContacted));
	}


	// Dump all of our results back into the database
	SetFeatureSetValue(ip, interface, featureValues);

	return featureValues;

}

bool Database::Disconnect()
{
	sqlite3_finalize(insertSuspect);
	sqlite3_finalize(insertPacketCount);
	sqlite3_finalize(incrementPacketCount);
	sqlite3_finalize(insertPortContacted);
	sqlite3_finalize(incrementPortContacted);
	sqlite3_finalize(insertPacketSize);
	sqlite3_finalize(incrementPacketSize);
	sqlite3_finalize(setFeatureValues);
	sqlite3_finalize(insertFeatureValue);
	sqlite3_finalize(computePacketSizeMean);
	sqlite3_finalize(computePacketSizeVariance);
	sqlite3_finalize(computeDistinctIps);
	sqlite3_finalize(computeDistinctPorts);
	sqlite3_finalize(computeDistinctIpPorts);
	sqlite3_finalize(selectPacketCounts);
	sqlite3_finalize(computeMaxPacketsToIp);
	sqlite3_finalize(computeMaxPacketsToPort);
	sqlite3_finalize(computeHoneypotsContacted);
	sqlite3_finalize(insertHoneypotIp);
	sqlite3_finalize(clearHoneypots);
	sqlite3_finalize(updateClassification);
	sqlite3_finalize(updateSuspectTimestamps);
	sqlite3_finalize(createHostileAlert);
	sqlite3_finalize(isSuspectHostile);
	sqlite3_finalize(getTotalPackets);
	sqlite3_finalize(getHostileSuspects);

	if (sqlite3_close(db) != SQLITE_OK)
	{
		LOG(ERROR, "Unable to finalize sql statement: " + string(sqlite3_errmsg(db)), "");
	}

	return true;
}

void Database::ClearAllSuspects()
{
	int rc;
	 char *szErrMsg = 0;

	  const char *pSQL[6];
	  pSQL[0] = "DELETE FROM ip_port_counts;";
	  pSQL[1] = "DELETE FROM packet_sizes;";
	  pSQL[2] = "DELETE FROM packet_counts;";
	  pSQL[3] = "DELETE FROM suspects;";

	  for(int i = 0; i < 4; i++)
	  {
	    rc = sqlite3_exec(db, pSQL[i], callback, 0, &szErrMsg);
	    if(rc != SQLITE_OK)
	    {
	      LOG(ERROR, "SQL Error: " + string(szErrMsg), "");
	      sqlite3_free(szErrMsg);
	      break;
	    }
	  }


	  // Delete from the suspect alerts database
	  sqlite3 *scriptDb;
	  rc = sqlite3_open(string(Config::Inst()->GetPathHome() + "/data/scriptAlerts.db").c_str(), &scriptDb);
	  const char *query = "DELETE FROM script_alerts;";

	  rc = sqlite3_exec(scriptDb, query, callback, 0, &szErrMsg);

	  if (rc != SQLITE_OK)
	  {
	      LOG(ERROR, "SQL Error: " + string(szErrMsg), "");
	      sqlite3_free(szErrMsg);
	  }
	  sqlite3_close(scriptDb);
}

void Database::ClearSuspect(const string &ip, const string &interface)
{
	cout << "Clearing suspect " << ip << " on interface " << interface << endl;
	int res;
	sqlite3_stmt *deleteFromIpPortCounts,*deleteFromPacketSizes,*deleteFromPacketCounts,*deleteFromSuspects;

	// Prepare the statements
	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,"DELETE FROM ip_port_counts WHERE ip = ? AND interface = ?;",
		-1, &deleteFromIpPortCounts,  NULL));
	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,"DELETE FROM packet_sizes WHERE ip = ? AND interface = ?;",
		-1, &deleteFromPacketSizes,  NULL));
	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,"DELETE FROM packet_counts WHERE ip = ? AND interface = ?;",
		-1, &deleteFromPacketCounts,  NULL));
	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db,"DELETE FROM suspects WHERE ip = ? AND interface = ?;",
		-1, &deleteFromSuspects,  NULL));

	// Bind the IP and interface into the statements
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(deleteFromIpPortCounts, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(deleteFromPacketSizes, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(deleteFromPacketCounts, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(deleteFromSuspects, 1, ip.c_str(), -1, SQLITE_STATIC));

	SQL_RUN(SQLITE_OK,sqlite3_bind_text(deleteFromIpPortCounts, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(deleteFromPacketSizes, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(deleteFromPacketCounts, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(deleteFromSuspects, 2, interface.c_str(), -1, SQLITE_STATIC));

	// Step and reset them. Make sure we get rid of foreign key references and do suspects table last
	m_count++;
	SQL_RUN(SQLITE_DONE,sqlite3_step(deleteFromIpPortCounts));
	SQL_RUN(SQLITE_OK, sqlite3_reset(deleteFromIpPortCounts));
	m_count++;
	SQL_RUN(SQLITE_DONE,sqlite3_step(deleteFromPacketSizes));
	SQL_RUN(SQLITE_OK, sqlite3_reset(deleteFromPacketSizes));
	m_count++;
	SQL_RUN(SQLITE_DONE,sqlite3_step(deleteFromPacketCounts));
	SQL_RUN(SQLITE_OK, sqlite3_reset(deleteFromPacketCounts));
	m_count++;
	SQL_RUN(SQLITE_DONE,sqlite3_step(deleteFromSuspects));
	SQL_RUN(SQLITE_OK, sqlite3_reset(deleteFromSuspects));

	// Finalize all the statements
	sqlite3_finalize(deleteFromIpPortCounts);
	sqlite3_finalize(deleteFromPacketSizes);
	sqlite3_finalize(deleteFromPacketCounts);
	sqlite3_finalize(deleteFromSuspects);



	// Delete from the suspect alerts database
	sqlite3 *scriptDb;
	SQL_RUN(SQLITE_OK,sqlite3_open(string(Config::Inst()->GetPathHome() + "/data/scriptAlerts.db").c_str(), &scriptDb));

	sqlite3_stmt *deleteFromScriptAlerts;
	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(scriptDb,"DELETE FROM script_alerts WHERE ip = ? AND interface = ?;",
		-1, &deleteFromScriptAlerts,  NULL));

	SQL_RUN(SQLITE_OK,sqlite3_bind_text(deleteFromScriptAlerts, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(deleteFromScriptAlerts, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_DONE,sqlite3_step(deleteFromScriptAlerts));
	SQL_RUN(SQLITE_OK, sqlite3_reset(deleteFromScriptAlerts));
	sqlite3_finalize(deleteFromScriptAlerts);

	sqlite3_close(scriptDb);
}

void Database::InsertSuspect(Suspect *suspect)
{
	int res;

	SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertSuspect, 1, suspect->GetIpString().c_str(), -1, SQLITE_TRANSIENT));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertSuspect, 2, suspect->GetInterface().c_str(), -1, SQLITE_TRANSIENT));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertSuspect, 3, static_cast<long int>(suspect->m_features.m_startTime)));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertSuspect, 4, static_cast<long int>(suspect->m_features.m_endTime)));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertSuspect, 5, static_cast<long int>(suspect->m_features.m_lastTime)));
	SQL_RUN(SQLITE_OK,sqlite3_bind_double(insertSuspect, 6, suspect->GetClassification()));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int(insertSuspect, 7, suspect->GetHostileNeighbors()));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int(insertSuspect, 8, suspect->GetIsHostile()));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertSuspect, 9, suspect->m_classificationNotes.c_str(), -1, SQLITE_TRANSIENT));

	m_count++;
	SQL_RUN(SQLITE_DONE,sqlite3_step(insertSuspect));
	SQL_RUN(SQLITE_OK, sqlite3_reset(insertSuspect));
}

void Database::IncrementPacketCount(const string &ip, const string &interface, const EvidenceAccumulator &e)
{
	int res;

	SQL_RUN(SQLITE_OK,sqlite3_bind_text(incrementPacketCount, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(incrementPacketCount, 2, interface.c_str(), -1, SQLITE_STATIC));

	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(incrementPacketCount, 3, e.m_tcpPacketCount));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(incrementPacketCount, 4, e.m_udpPacketCount));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(incrementPacketCount, 5, e.m_icmpPacketCount));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(incrementPacketCount, 6, e.m_otherPacketCount));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(incrementPacketCount, 7, e.m_packetCount));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(incrementPacketCount, 8, e.m_rstCount));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(incrementPacketCount, 9, e.m_ackCount));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(incrementPacketCount, 10, e.m_synCount));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(incrementPacketCount, 11, e.m_finCount));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(incrementPacketCount, 12, e.m_synAckCount));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int64(incrementPacketCount, 13, e.m_bytesTotal));

	m_count++;
	SQL_RUN(SQLITE_DONE, sqlite3_step(incrementPacketCount));
	SQL_RUN(SQLITE_OK, sqlite3_reset(incrementPacketCount));


	// If the update failed, we need to insert the port contacted count and set it
	if (sqlite3_changes(db) == 0)
	{
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPacketCount, 1, ip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPacketCount, 2, interface.c_str(), -1, SQLITE_STATIC));

		SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertPacketCount, 3, e.m_tcpPacketCount));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertPacketCount, 4, e.m_udpPacketCount));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertPacketCount, 5, e.m_icmpPacketCount));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertPacketCount, 6, e.m_otherPacketCount));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertPacketCount, 7, e.m_packetCount));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertPacketCount, 8, e.m_rstCount));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertPacketCount, 9, e.m_ackCount));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertPacketCount, 10, e.m_synCount));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertPacketCount, 11, e.m_finCount));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertPacketCount, 12, e.m_synAckCount));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int64(insertPacketCount, 13, e.m_bytesTotal));

		m_count++;
		SQL_RUN(SQLITE_DONE, sqlite3_step(insertPacketCount));
		SQL_RUN(SQLITE_OK, sqlite3_reset(insertPacketCount));
	}
}

void Database::IncrementPacketSizeCount(const string &ip, const string &interface, uint16_t size, uint64_t increment)
{
	if (!increment)
		return;

	int res;

	SQL_RUN(SQLITE_OK, sqlite3_bind_text(incrementPacketSize, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(incrementPacketSize, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_int(incrementPacketSize, 3, size));
	SQL_RUN(SQLITE_OK, sqlite3_bind_int(incrementPacketSize, 4, increment));

	m_count++;
	SQL_RUN(SQLITE_DONE, sqlite3_step(incrementPacketSize));
	SQL_RUN(SQLITE_OK, sqlite3_reset(incrementPacketSize));

	if (sqlite3_changes(db) == 0)
	{
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPacketSize, 1, ip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPacketSize, 2, interface.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int(insertPacketSize, 3, size));
		SQL_RUN(SQLITE_OK, sqlite3_bind_int(insertPacketSize, 4, increment));

		m_count++;
		SQL_RUN(SQLITE_DONE, sqlite3_step(insertPacketSize));
		SQL_RUN(SQLITE_OK, sqlite3_reset(insertPacketSize));
	}
}

void Database::IncrementPortContactedCount(const string &ip, const string &interface, const string &protocol, const string &dstip, int port, uint64_t increment)
{
	if (!increment)
		return;

	int res;

	// Try to increment the port count if it exists
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(incrementPortContacted, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(incrementPortContacted, 2, interface.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(incrementPortContacted, 3, protocol.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_text(incrementPortContacted, 4, dstip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int(incrementPortContacted, 5, port));
	SQL_RUN(SQLITE_OK,sqlite3_bind_int(incrementPortContacted, 6, increment));

	m_count++;
	SQL_RUN(SQLITE_DONE, sqlite3_step(incrementPortContacted));
	SQL_RUN(SQLITE_OK, sqlite3_reset(incrementPortContacted));

	// If the update failed, we need to insert the port contacted count and set it to 1
	if (sqlite3_changes(db) == 0)
	{
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPortContacted, 1, ip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPortContacted, 2, interface.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPortContacted, 3, protocol.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_text(insertPortContacted, 4, dstip.c_str(), -1, SQLITE_STATIC));
		SQL_RUN(SQLITE_OK,sqlite3_bind_int(insertPortContacted, 5, port));

		m_count++;
		SQL_RUN(SQLITE_DONE, sqlite3_step(insertPortContacted));
		SQL_RUN(SQLITE_OK, sqlite3_reset(insertPortContacted));
	}
}

void Database::InsertHoneypotIp(std::string ip, std::string interface)
{
	int res;

	SQL_RUN(SQLITE_OK, sqlite3_bind_text(insertHoneypotIp, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(insertHoneypotIp, 2, interface.c_str(), -1 ,SQLITE_STATIC));

	m_count++;
	SQL_RUN(SQLITE_DONE, sqlite3_step(insertHoneypotIp));
	SQL_RUN(SQLITE_OK, sqlite3_reset(insertHoneypotIp));
}

// Deletes entries from the table that keeps track of the current honeypot IPs
void Database::ClearHoneypots() {
	int res;
	m_count++;

	SQL_RUN(SQLITE_DONE, sqlite3_step(clearHoneypots));
	SQL_RUN(SQLITE_OK, sqlite3_reset(clearHoneypots));
}

void Database::SetFeatureSetValue(const string &ip, const string &interface, const vector<double> &features)
{
	int res;

	if (features.size() != DIM)
	{
		LOG(ERROR, "Features must have DIM entries. This shouldn't happen.", "");
		return;
	}

	SQL_RUN(SQLITE_OK, sqlite3_bind_text(setFeatureValues, 15, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(setFeatureValues, 16, interface.c_str(), -1, SQLITE_STATIC));


	for (int i = 0; i < DIM; i++)
	{
		SQL_RUN(SQLITE_OK, sqlite3_bind_double(setFeatureValues, i + 1, features[i]));
	}

	m_count++;
	SQL_RUN(SQLITE_DONE, sqlite3_step(setFeatureValues));
	SQL_RUN(SQLITE_OK, sqlite3_reset(setFeatureValues));
}


void Database::InsertSuspectHostileAlert(const std::string &ip, const std::string &interface)
{
	int res;

	SQL_RUN(SQLITE_OK, sqlite3_bind_text(createHostileAlert, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(createHostileAlert, 2, interface.c_str(), -1, SQLITE_STATIC));

	m_count++;
	SQL_RUN(SQLITE_DONE, sqlite3_step(createHostileAlert));
	SQL_RUN(SQLITE_OK, sqlite3_reset(createHostileAlert));

}

bool Database::IsSuspectHostile(const std::string &ip, const std::string &interface)
{
	int res;
	bool suspectHostile;

	SQL_RUN(SQLITE_OK, sqlite3_bind_text(isSuspectHostile, 1, ip.c_str(), -1, SQLITE_STATIC));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(isSuspectHostile, 2, interface.c_str(), -1, SQLITE_STATIC));


	m_count++;
	res = sqlite3_step(isSuspectHostile);

	if (res == SQLITE_ROW)
	{
		suspectHostile = sqlite3_column_int(isSuspectHostile, 1);
	}
	else
	{
		suspectHostile = false;
	}

	SQL_RUN(SQLITE_OK, sqlite3_reset(isSuspectHostile));

	return suspectHostile;
}


std::vector<string> Database::GetSuspectList(enum SuspectListType listType)
{
	vector<string> suspects;
	int res;
	sqlite3_stmt *stmt;

	if (listType == SUSPECTLIST_ALL)
	{
	   SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db, "SELECT ip, interface FROM suspects", -1, &stmt, NULL));
	}
	else if (listType == SUSPECTLIST_HOSTILE)
	{
	   SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db, "SELECT ip, interface FROM suspects WHERE isHostile = 1", -1, &stmt, NULL));
	}
	else if (listType == SUSPECTLIST_BENIGN)
	{
	   SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db, "SELECT ip, interface FROM suspects WHERE isHostile = 0", -1, &stmt, NULL));
	}

	res = sqlite3_step(stmt);

	while (res == SQLITE_ROW)
	{
		string str;
		str += string((const char*)sqlite3_column_text(stmt, 1));
		str += " ";
		str += string((const char*)sqlite3_column_text(stmt, 0));

		suspects.push_back(str);

		res = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return suspects;
}

std::vector<Suspect> Database::GetSuspects(enum SuspectListType listType)
{
	vector<Suspect> suspects;
	int res;
	sqlite3_stmt *stmt;

	if (listType == SUSPECTLIST_ALL)
	{
	   SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db, "SELECT * FROM suspects ORDER BY classification DESC", -1, &stmt, NULL));
	}
	else if (listType == SUSPECTLIST_HOSTILE)
	{
	   SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db, "SELECT * FROM suspects WHERE isHostile = 1 ORDER BY classification DESC", -1, &stmt, NULL));
	}
	else if (listType == SUSPECTLIST_BENIGN)
	{
	   SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db, "SELECT * FROM suspects WHERE isHostile = 0 ORDER BY classification DESC", -1, &stmt, NULL));
	}

	res = sqlite3_step(stmt);

	while (res == SQLITE_ROW)
	{
		Suspect s;

		SuspectID_pb id;
		id.set_m_ip(htonl(inet_addr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)))));
		id.set_m_ifname(string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))));
		s.SetIdentifier(id);

		s.m_lastMac = sqlite3_column_int64(stmt, 2);
		s.m_features.m_startTime = sqlite3_column_int(stmt, 3);
		s.m_features.m_endTime = sqlite3_column_int(stmt, 4);
		s.m_features.m_lastTime = sqlite3_column_int(stmt, 5);
		s.SetClassification(sqlite3_column_double(stmt, 6));
		s.SetIsHostile(sqlite3_column_double(stmt, 8));
		s.m_classificationNotes = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)));
		for (int i = 10; i < 10 + DIM; i++)
		{
			s.m_features.m_features[i - 10] = sqlite3_column_double(stmt, i);
		}

		suspects.push_back(s);

		res = sqlite3_step(stmt);
	}

	sqlite3_finalize(stmt);

	return suspects;
}

Suspect Database::GetSuspect(SuspectID_pb id)
{
	Suspect s;
	int res;
	sqlite3_stmt *stmt;

	SQL_RUN(SQLITE_OK, sqlite3_prepare_v2(db, "SELECT * FROM suspects WHERE ip = ? AND interface = ?", -1, &stmt, NULL));

	SQL_RUN(SQLITE_OK, sqlite3_bind_text(stmt, 1, Suspect::GetIpString(id).c_str(), -1, SQLITE_TRANSIENT));
	SQL_RUN(SQLITE_OK, sqlite3_bind_text(stmt, 2, id.m_ifname().c_str(), -1, SQLITE_TRANSIENT));

	res = sqlite3_step(stmt);

	if (res == SQLITE_ROW)
	{
		s.SetIdentifier(id);
		s.m_lastMac = sqlite3_column_int64(stmt, 2);
		s.m_features.m_startTime = sqlite3_column_int(stmt, 3);
		s.m_features.m_endTime = sqlite3_column_int(stmt, 4);
		s.m_features.m_lastTime = sqlite3_column_int(stmt, 5);
		s.SetClassification(sqlite3_column_double(stmt, 6));
		s.SetIsHostile(sqlite3_column_double(stmt, 8));
		s.m_classificationNotes = string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)));
		for (int i = 10; i < 10 + DIM; i++)
		{
			s.m_features.m_features[i - 10] = sqlite3_column_double(stmt, i);
		}
	}

	sqlite3_finalize(stmt);

	return s;
}



}/* namespace Nova */
