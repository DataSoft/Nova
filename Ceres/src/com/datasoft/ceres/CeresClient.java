package com.datasoft.ceres;

import java.io.StringReader;
import java.util.ArrayList;
import android.app.Activity;
import android.app.Application;

public class CeresClient extends Application {
	String m_clientId;
	String m_pass;
	String m_xmlBase;
	StringReader m_xmlReceive;
	Boolean m_messageReceived;
	ArrayList<String> m_gridCache;
	Boolean m_isInForeground;
	Activity m_onUiThread;
	String m_serverUrl;	
	
	@Override
	public void onCreate()
	{
		m_messageReceived = false;
		m_clientId = "";
		m_pass = "";
		m_gridCache = new ArrayList<String>();
		super.onCreate();
	}
	
	public ArrayList<String> getGridCache()
	{
		return m_gridCache;
	}
	
	public void setGridCache(ArrayList<String> newCache)
	{
		m_gridCache.clear();
		for(String s : newCache)
		{
			m_gridCache.add(s);
		}
	}
	
	/*
	Context ctx = getApplicationContext();
	Notification.Builder build = new Notification.Builder(ctx)
		.setSmallIcon(R.drawable.ic_launcher)
		.setContentTitle("Ceres Lost Connection")
		.setContentText("The Ceres app has lost connection with the server")
		.setAutoCancel(true);
	Intent intent = new Intent(ctx, MainActivity.class);
	TaskStackBuilder tsbuild = TaskStackBuilder.create(ctx);
	tsbuild.addParentStack(MainActivity.class);
	tsbuild.addNextIntent(intent);
	PendingIntent rpintent = tsbuild.getPendingIntent(0, PendingIntent.FLAG_UPDATE_CURRENT);
	build.setContentIntent(rpintent);
	NotificationManager nm = (NotificationManager)getSystemService(Context.NOTIFICATION_SERVICE);
	nm.notify(0, build.build());*/
	
	public void setURL(String url)
	{
		m_serverUrl = url;
	}
	
	public String getURL()
	{
		return m_serverUrl;
	}
	
	public Boolean checkMessageReceived()
	{
		return m_messageReceived;
	}
	
	public void setMessageReceived(Boolean msgRecv)
	{
		m_messageReceived = msgRecv;
	}
	
	public StringReader getXmlReceive()
	{
		m_xmlReceive = new StringReader(m_xmlBase);
		return m_xmlReceive;
	}
	
	public void setXmlBase(String base)
	{
		m_xmlBase = base;
	}
	
	public String getXmlBase()
	{
		return m_xmlBase;
	}
	
	public void clearXmlReceive()
	{
		m_xmlReceive = null;
		m_messageReceived = false;
	}
	
	public String getClientId()
	{
		return m_clientId;
	}
	
	public void setClientId(String id)
	{
		m_clientId = id;
	}
	
	public String getPass()
	{
		return m_pass;
	}
	
	public void setPass(String newPass)
	{
		m_pass = newPass;
	}
	
	public Boolean isInForeground()
	{
		return m_isInForeground;
	}
	
	public void setForeground(Boolean inForeground)
	{
		m_isInForeground = inForeground;
	}
}
