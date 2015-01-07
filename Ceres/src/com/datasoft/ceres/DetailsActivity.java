package com.datasoft.ceres;

import java.io.IOException;

import org.achartengine.ChartFactory;
import org.achartengine.GraphicalView;
import org.achartengine.model.CategorySeries;
import org.achartengine.renderer.DefaultRenderer;
import org.achartengine.renderer.SimpleSeriesRenderer;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Color;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.ViewGroup.LayoutParams;
import android.widget.LinearLayout;
import android.widget.TableRow;
import android.widget.TextView;
import android.widget.Toast;

public class DetailsActivity extends Activity
{
	TextView m_ip;
	TextView m_iface;
	TextView m_class;
	TextView m_lpt;
	ProgressDialog m_wait;
	Context m_detailsContext;
	CeresClient m_global;
	
	GraphicalView m_protocolPie;
	GraphicalView m_flagsPie;
	
	@Override
	protected void onPause()
	{
		super.onPause();
		m_global.setForeground(false);
	}

	@Override
	protected void onResume()
	{
		super.onResume();
		m_global.setForeground(true);
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_details);
		m_global = (CeresClient)getApplicationContext();
		m_wait = new ProgressDialog(this);
		m_ip = (TextView)findViewById(R.id.suspectIP);
		m_iface = (TextView)findViewById(R.id.suspectIface);
		m_class = (TextView)findViewById(R.id.suspectClassification);
		m_lpt = (TextView)findViewById(R.id.suspectLastPacket);
		m_detailsContext = this;
		new ParseXml().execute();
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent keyEvent)
	{
    	if(keyCode == KeyEvent.KEYCODE_BACK)
    	{
    		m_global.clearXmlReceive();
    	}
    	return super.onKeyDown(keyCode, keyEvent);
	}
	
	private class ParseXml extends AsyncTask<Void, Void, Suspect>
	{
		@Override
		protected void onPreExecute()
		{
			m_wait.setCancelable(true);
			m_wait.setMessage("Retrieving Suspect Data");
			m_wait.setProgressStyle(ProgressDialog.STYLE_SPINNER);
			m_wait.show();
    		super.onPreExecute();
		}
		
		@Override
		protected Suspect doInBackground(Void... vd)
		{
			try
			{
				XmlPullParserFactory factory = XmlPullParserFactory.newInstance();
				factory.setNamespaceAware(true);
				XmlPullParser xpp;
				
				if(m_global.checkMessageReceived())
				{
					xpp = factory.newPullParser();
					xpp.setInput(m_global.getXmlReceive());
					Suspect parsed = new Suspect();
					int evt = xpp.getEventType();
					// On this page, we're receiving a format containing three things:
					// ip, interface and classification
					while(evt != XmlPullParser.END_DOCUMENT)
					{
						switch(evt)
						{
							case(XmlPullParser.START_TAG):
								if(xpp.getName().equals("idInfo"))
								{
									parsed.m_id = xpp.getAttributeValue(null, "id");
									parsed.m_ip = xpp.getAttributeValue(null, "ip");
									parsed.m_iface = xpp.getAttributeValue(null, "iface");
									parsed.m_ifaceAlias = xpp.getAttributeValue(null, "alias");
									parsed.m_classification = xpp.getAttributeValue(null, "class");
									parsed.m_lastPacket = xpp.getAttributeValue(null, "lpt");
								}
								else if(xpp.getName().equals("protoInfo"))
								{
									parsed.m_tcpCount = xpp.getAttributeValue(null, "tcp");
									parsed.m_udpCount = xpp.getAttributeValue(null, "udp");
									parsed.m_icmpCount = xpp.getAttributeValue(null, "icmp");
									parsed.m_otherCount = xpp.getAttributeValue(null, "other");
								}
								else if(xpp.getName().equals("packetInfo"))
								{
									parsed.m_rstCount = xpp.getAttributeValue(null, "rst");
									parsed.m_ackCount = xpp.getAttributeValue(null, "ack");
									parsed.m_synCount = xpp.getAttributeValue(null, "syn");
									parsed.m_finCount = xpp.getAttributeValue(null, "fin");
									parsed.m_synAckCount = xpp.getAttributeValue(null, "synack");
								}
								break;
							default: 
								break;
						}
						evt = xpp.next();
					}
					return parsed;
				}
				else
				{
					return null;
				}
			}
			catch(XmlPullParserException xppe)
			{
				return null;
			}
			catch(IOException ioe)
			{
				return null;
			}
		}
		
	@Override
	protected void onPostExecute(Suspect suspect)
	{
		if(suspect == null)
		{
			m_wait.cancel();
			AlertDialog.Builder build = new AlertDialog.Builder(m_detailsContext);
			build
			.setTitle("Suspect Info Empty")
			.setMessage("Ceres server returned no data for suspect. Try again?")
			.setCancelable(false)
			.setPositiveButton("Yes", new DialogInterface.OnClickListener(){
				@Override
				public void onClick(DialogInterface dialog, int id)
				{
					m_global.clearXmlReceive();
					Intent nextPage = new Intent(getApplicationContext(), DetailsActivity.class);
					nextPage.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
					nextPage.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
					getApplicationContext().startActivity(nextPage);
				}
			})
			.setNegativeButton("No", new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog, int which)
				{
					m_global.clearXmlReceive();
					Intent nextPage = new Intent(getApplicationContext(), GridActivity.class);
					nextPage.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
					nextPage.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
					nextPage.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
					getApplicationContext().startActivity(nextPage);
				}
			});
			AlertDialog ad = build.create();
			ad.show();
		}
		else
		{
			m_ip.setText(suspect.m_ip);
			m_iface.setText(suspect.m_iface);
			m_class.setText(suspect.m_classification + "%");
			m_lpt.setText(suspect.m_lastPacket);
			Toast.makeText(m_detailsContext, "Suspect " + suspect.getIp() + ":" + suspect.getIface() + " loaded", Toast.LENGTH_LONG).show();
			m_wait.cancel();
			
			//Make the protocol Pie Chart
			if(m_protocolPie == null)
			{	
				try
				{
					CategorySeries protocolSeries = new CategorySeries("");
					
					DefaultRenderer defaultRenderer = new DefaultRenderer();
	
					defaultRenderer.setApplyBackgroundColor(true);
					defaultRenderer.setBackgroundColor(Color.argb(0xff, 0xe4, 0xf2, 0xff));
					defaultRenderer.setChartTitleTextSize(25);
					defaultRenderer.setChartTitle("Protocol Used");
					defaultRenderer.setLabelsTextSize(15);
					defaultRenderer.setLabelsColor(Color.BLACK);
					defaultRenderer.setShowLegend(false);
					defaultRenderer.setZoomButtonsVisible(false);
					defaultRenderer.setStartAngle(180);
					
					if(Integer.parseInt(suspect.m_tcpCount) > 0)
					{
						protocolSeries.add("TCP", Integer.parseInt(suspect.m_tcpCount));
						SimpleSeriesRenderer simpleSeriesRenderer = new SimpleSeriesRenderer();
						simpleSeriesRenderer.setColor(Color.argb(0xff, 0x00, 0x5a, 0xff));  //blue
						defaultRenderer.addSeriesRenderer(simpleSeriesRenderer);
					}
					if(Integer.parseInt(suspect.m_udpCount) > 0)
					{
						protocolSeries.add("UDP", Integer.parseInt(suspect.m_udpCount));
						SimpleSeriesRenderer simpleSeriesRenderer = new SimpleSeriesRenderer();
						simpleSeriesRenderer.setColor(Color.RED);
						defaultRenderer.addSeriesRenderer(simpleSeriesRenderer);
					}
					if(Integer.parseInt(suspect.m_icmpCount) > 0)
					{
						protocolSeries.add("ICMP", Integer.parseInt(suspect.m_icmpCount));
						SimpleSeriesRenderer simpleSeriesRenderer = new SimpleSeriesRenderer();
						simpleSeriesRenderer.setColor(Color.argb(0xff, 0x36, 0x8f, 0x00)); //green
						defaultRenderer.addSeriesRenderer(simpleSeriesRenderer);
					}
					if(Integer.parseInt(suspect.m_otherCount) > 0)
					{
						protocolSeries.add("Other", Integer.parseInt(suspect.m_otherCount));
						SimpleSeriesRenderer simpleSeriesRenderer = new SimpleSeriesRenderer();
						simpleSeriesRenderer.setColor(Color.argb(0xff, 0xfa, 0x7d, 0x00)); //orange
						defaultRenderer.addSeriesRenderer(simpleSeriesRenderer);
					}
					if(protocolSeries.getItemCount() == 0)
					{
						protocolSeries.add("No Packets", 1);
						SimpleSeriesRenderer simpleSeriesRenderer = new SimpleSeriesRenderer();
						simpleSeriesRenderer.setColor(Color.LTGRAY);
						defaultRenderer.addSeriesRenderer(simpleSeriesRenderer);
					}
					
					LinearLayout layout = (LinearLayout) findViewById(R.id.chartsLayout);
					m_protocolPie = ChartFactory.getPieChartView(DetailsActivity.this, protocolSeries, defaultRenderer);
					defaultRenderer.setClickEnabled(true);
					defaultRenderer.setSelectableBuffer(10);
					LinearLayout containerLayout = new LinearLayout(DetailsActivity.this);
					LayoutParams params = new TableRow.LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f);
					containerLayout.setLayoutParams(params);
					containerLayout.addView(m_protocolPie);
					layout.addView(containerLayout);
				}
				catch(NumberFormatException ex)
				{
					LinearLayout layout = (LinearLayout) findViewById(R.id.chartsLayout);
					TextView view = new TextView(DetailsActivity.this);
					view.setText("Invalid data found");
					layout.addView(view, 0);	
				}
			}
			else
			{
				m_protocolPie.repaint();
			}
			
			//Make the flags Pie Chart
			if(m_flagsPie == null)
			{
				try
				{
					CategorySeries protocolSeries = new CategorySeries("");
					DefaultRenderer defaultRenderer = new DefaultRenderer();
					
					defaultRenderer.setApplyBackgroundColor(true);
					defaultRenderer.setBackgroundColor(Color.argb(0xff, 0xe4, 0xf2, 0xff));
					defaultRenderer.setChartTitleTextSize(25);
					defaultRenderer.setChartTitle("TCP Flags Used");
					defaultRenderer.setLabelsTextSize(15);
					defaultRenderer.setLabelsColor(Color.BLACK);
					defaultRenderer.setShowLegend(false);
					defaultRenderer.setZoomButtonsVisible(false);
					defaultRenderer.setStartAngle(180);
					
					if(Integer.parseInt(suspect.m_rstCount) > 0)
					{
						protocolSeries.add("RST", Integer.parseInt(suspect.m_rstCount));
						SimpleSeriesRenderer simpleSeriesRenderer = new SimpleSeriesRenderer();
						simpleSeriesRenderer.setColor(Color.argb(0xff, 0x00, 0x5a, 0xff));  //blue
						defaultRenderer.addSeriesRenderer(simpleSeriesRenderer);
					}
					if(Integer.parseInt(suspect.m_synCount) > 0)
					{
						protocolSeries.add("SYN", Integer.parseInt(suspect.m_synCount));
						SimpleSeriesRenderer simpleSeriesRenderer = new SimpleSeriesRenderer();
						simpleSeriesRenderer.setColor(Color.RED);
						defaultRenderer.addSeriesRenderer(simpleSeriesRenderer);
					}
					if(Integer.parseInt(suspect.m_finCount) > 0)
					{
						protocolSeries.add("FIN", Integer.parseInt(suspect.m_finCount));
						SimpleSeriesRenderer simpleSeriesRenderer = new SimpleSeriesRenderer();
						simpleSeriesRenderer.setColor(Color.argb(0xff, 0x36, 0x8f, 0x00)); //green
						defaultRenderer.addSeriesRenderer(simpleSeriesRenderer);
					}
					if(Integer.parseInt(suspect.m_ackCount) > 0)
					{
						protocolSeries.add("ACK", Integer.parseInt(suspect.m_ackCount));
						SimpleSeriesRenderer simpleSeriesRenderer = new SimpleSeriesRenderer();
						simpleSeriesRenderer.setColor(Color.argb(0xff, 0xf9, 0xe4, 0x00)); //yellow
						defaultRenderer.addSeriesRenderer(simpleSeriesRenderer);
					}
					if(Integer.parseInt(suspect.m_synAckCount) > 0)
					{
						protocolSeries.add("SYN/ACK", Integer.parseInt(suspect.m_synAckCount));
						SimpleSeriesRenderer simpleSeriesRenderer = new SimpleSeriesRenderer();
						simpleSeriesRenderer.setColor(Color.argb(0xff, 0x8c, 0x00, 0xff)); //purple
						defaultRenderer.addSeriesRenderer(simpleSeriesRenderer);
					}
					if(protocolSeries.getItemCount() == 0)
					{
						protocolSeries.add("No TCP Flags", 1);
						SimpleSeriesRenderer simpleSeriesRenderer = new SimpleSeriesRenderer();
						simpleSeriesRenderer.setColor(Color.LTGRAY);
						defaultRenderer.addSeriesRenderer(simpleSeriesRenderer);
					}
					
					LinearLayout layout = (LinearLayout) findViewById(R.id.chartsLayout);
					m_flagsPie = ChartFactory.getPieChartView(DetailsActivity.this, protocolSeries, defaultRenderer);
					defaultRenderer.setClickEnabled(true);
					defaultRenderer.setSelectableBuffer(10);
					LinearLayout containerLayout = new LinearLayout(DetailsActivity.this);
					LayoutParams params = new TableRow.LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f);
					containerLayout.setLayoutParams(params);
					containerLayout.addView(m_flagsPie);
					layout.addView(containerLayout);
				}
				catch(NumberFormatException ex)
				{
					LinearLayout layout = (LinearLayout) findViewById(R.id.chartsLayout);
					TextView view = new TextView(DetailsActivity.this);
					view.setText("Invalid data found");
						layout.addView(view, 1);
					}
				}
				else
				{
					m_protocolPie.repaint();
				}
			}
		}
	}
}
