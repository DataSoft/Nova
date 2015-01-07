package com.datasoft.ceres;

public class Suspect
{
	String m_id = "";
	String m_ip = "";
	String m_iface = "";
	String m_ifaceAlias = "";
	String m_classification = "";
	Boolean m_hostility;
	String m_lastPacket = "";
	String m_rstCount = "";
	String m_ackCount = "";
	String m_synCount = "";
	String m_finCount = "";
	String m_synAckCount = "";
	String m_tcpCount = "";
	String m_udpCount = "";
	String m_icmpCount = "";
	String m_otherCount = "";
	
	public Suspect()
	{
		m_hostility = false;
	}
	
	public Suspect(String ip, String iface)
	{
		m_ip = ip;
		m_iface = iface;
		m_id = ip + ":" + iface;
		m_hostility = false;
	}
	
	public void setIp(String ip)
	{
		m_ip = ip;
	}
	
	public String getIp()
	{
		return m_ip;
	}

	public void setIface(String iface)
	{
		m_iface = iface;
	}
	
	public String getIface()
	{
		return m_iface;
	}
	
	public boolean isEmpty()
	{
		if(m_id.compareTo("") != 0 && m_iface.compareTo("") != 0 && m_ip.compareTo("") != 0
        && m_classification.compareTo("") != 0 && m_lastPacket.compareTo("") != 0 && m_rstCount.compareTo("") != 0
        && m_ackCount.compareTo("") != 0 && m_synCount.compareTo("") != 0 && m_finCount.compareTo("") != 0
        && m_synAckCount.compareTo("") != 0 && m_tcpCount.compareTo("") != 0 && m_udpCount.compareTo("") != 0
        && m_icmpCount.compareTo("") != 0 && m_otherCount.compareTo("") != 0)
		{
			return true;
		}
		return false;
	}
}
