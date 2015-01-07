package com.datasoft.ceres;

import java.io.IOException;
import java.util.ArrayList;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;
import com.loopj.android.http.AsyncHttpResponseHandler;
import com.loopj.android.http.RequestParams;
import android.app.AlertDialog;
import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.widget.Toast;

public class GridActivity extends ListActivity {
	CeresClient m_global;
	ProgressDialog m_wait;
	ClassificationGridAdapter m_gridAdapter;
	Context m_gridContext;
	String m_selected;
	ArrayList<String> m_gridValues;
	
	final Handler m_handler = new Handler();
	
	final Runnable m_updateGrid = new Runnable() {
		public void run() {
			resetAdapter();
		}
	};
	
	Thread m_updateThread = new Thread() {
		public void run() {
			try
			{
				while(true)
				{
					sleep(60000);
					m_global.clearXmlReceive();
					try
		    		{
		    			NetworkHandler.get(("https://" + m_global.getURL() + "/getAll"), null, new AsyncHttpResponseHandler(){
		    				@Override
		    				public void onSuccess(String xml)
		    				{
		    					m_global.setXmlBase(xml);
		    					m_global.setMessageReceived(true);
		    				}
		    				
		    				@Override
		    				public void onFailure(Throwable err, String content)
		    				{
		    					m_global.setXmlBase("");
		    					m_global.setMessageReceived(true);
		    				}
		    			});
		    			while(!m_global.checkMessageReceived()){};
		    		}
		    		catch(Exception e)
		    		{
		    			e.printStackTrace();
		    			return;
		    		}
		    		
		    		if(m_global.getXmlBase() != "")
		    		{
						m_gridValues = constructGridValues();
						if(m_gridValues != null)
						{
							m_handler.post(m_updateGrid);
						}
		    			System.out.println("returning 1");
		    			return;
		    		}
		    		else
		    		{
		    			System.out.println("returning 0");
		    			return;
		    		}
				}
			}
			catch(InterruptedException ie)
			{
				return;
			}
			catch(IOException ioe)
			{
				return;
			}
			catch(XmlPullParserException xpe)
			{
				return;
			}
		}
	};
	
    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }
	
    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
    	switch(item.getItemId())
    	{
    		case R.id.refresh:
    			new CeresClientConnect().execute();
    			return true;
    		default:
    			return super.onOptionsItemSelected(item);
    	}
    }
    
	private void resetAdapter()
	{
		if(m_wait.isShowing())
		{
			m_wait.cancel();
		}
		m_gridAdapter.clear();
		m_gridAdapter.addAll(m_gridValues);
		m_gridAdapter.notifyDataSetChanged();
	}
	
	private ArrayList<String> constructGridValues() throws XmlPullParserException, IOException 
	{
		XmlPullParserFactory factory = XmlPullParserFactory.newInstance();
		factory.setNamespaceAware(true);
		XmlPullParser xpp;
		if(m_global.checkMessageReceived())
		{
			xpp = factory.newPullParser();
			xpp.setInput(m_global.getXmlReceive());
			int evt = xpp.getEventType();
			ArrayList<String> al = new ArrayList<String>();
			// On this page, we're receiving a format containing three things:
			// ip, interface and classification
			String rowData = "";
			while(evt != XmlPullParser.END_DOCUMENT)
			{
				switch(evt)
				{
					case(XmlPullParser.START_DOCUMENT):
						break;
					case(XmlPullParser.START_TAG):
						if(xpp.getName().equals("suspect"))
						{
							rowData += xpp.getAttributeValue(null, "ipaddress") + ":";
							rowData += xpp.getAttributeValue(null, "alias") + ":";
							rowData += xpp.getAttributeValue(null, "hostile") + ":";
							rowData += xpp.getAttributeValue(null, "interface") + ":";
						}
						break;
					case(XmlPullParser.TEXT): 
						rowData += xpp.getText() + "%";
						al.add(rowData);
						rowData = "";
						break;
					default: 
						break;
				}
				evt = xpp.next();
			}
			if(al.size() > 0)
			{
				m_global.setGridCache(al);
			}
			return al;
		}
		else if(m_global.getGridCache().size() != 0)
		{
			return m_global.getGridCache();
		}
		else
		{
			return null;
		}
	}
	
	@Override
	protected void onPause()
	{
		super.onPause();
		m_updateThread.interrupt();
		m_global.setForeground(false);
	}
	
	@Override
	protected void onResume()
	{
		super.onResume();
		m_global.setForeground(true);
	}
	
	@Override
	protected void onStop()
	{
		super.onStop();
		if(m_updateThread.getState() != Thread.State.TERMINATED)
		{
			m_updateThread.interrupt();
		}
	}
	
	@Override
	protected void onRestart()
	{
		super.onRestart();
		if(m_global.getGridCache().size() > 0)
		{
			m_gridAdapter.clear();
			m_gridAdapter.addAll(m_global.getGridCache());
			m_gridAdapter.notifyDataSetChanged();
		}
		else
		{
			m_wait.setCancelable(false);
			m_wait.setMessage("Constructing Suspect List");
			m_wait.setProgressStyle(ProgressDialog.STYLE_SPINNER);
			m_wait.show();
		}
		if(m_updateThread.getState() != Thread.State.NEW)
		{
			m_updateThread.interrupt();
			m_updateThread = new Thread() {
				public void run() {
					try
					{
						while(true)
						{
							sleep(60000);
							m_global.clearXmlReceive();
							try
				    		{
				    			NetworkHandler.get(("https://" + m_global.getURL() + "/getAll"), null, new AsyncHttpResponseHandler(){
				    				@Override
				    				public void onSuccess(String xml)
				    				{
				    					m_global.setXmlBase(xml);
				    					m_global.setMessageReceived(true);
				    				}
				    				
				    				@Override
				    				public void onFailure(Throwable err, String content)
				    				{
				    					m_global.setXmlBase("");
				    					m_global.setMessageReceived(true);
				    				}
				    			});
				    			while(!m_global.checkMessageReceived()){};
				    		}
				    		catch(Exception e)
				    		{
				    			e.printStackTrace();
				    			return;
				    		}
				    		
				    		if(m_global.getXmlBase() != "")
				    		{
								m_gridValues = constructGridValues();
								if(m_gridValues != null)
								{
									m_handler.post(m_updateGrid);
								}
				    			return;
				    		}
				    		else
				    		{
				    			return;
				    		}
						}
					}
					catch(InterruptedException ie)
					{
						return;
					}
					catch(IOException ioe)
					{
						return;
					}
					catch(XmlPullParserException xpe)
					{
						return;
					}
				}
			};
			m_updateThread.start();
		}
		else
		{
			m_updateThread.start();
		}
	}
	
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent keyEvent)
    {
    	if(keyCode == KeyEvent.KEYCODE_HOME || keyCode == KeyEvent.KEYCODE_BACK)
    	{
    		if(m_updateThread.getState() != Thread.State.TERMINATED)
    		{
    			m_updateThread.interrupt();
    		}
    	}
    	return super.onKeyDown(keyCode, keyEvent);
    }
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        m_global = (CeresClient)getApplicationContext();
        m_wait = new ProgressDialog(this);
		m_gridContext = this;
		LayoutInflater inf = getLayoutInflater();
		View header = inf.inflate(R.layout.grid_header, (ViewGroup)findViewById(R.id.header_layout_root));
		getListView().addHeaderView(header, null, false);
		new ParseXml().execute();
	}
	
	@Override
	protected void onListItemClick(ListView l, View v, int position, long id) {
		String[] item = ((String)getListAdapter().getItem(position - 1)).split(":");
		m_selected = item[0] + ":" + item[3];
		Toast.makeText(this, m_selected + " selected", Toast.LENGTH_SHORT).show();

		new CeresSuspectRequest().execute();
	}
	
	private class CeresSuspectRequest extends AsyncTask<Void, Void, Integer> {
		@Override
		protected void onPreExecute()
		{
			m_global.clearXmlReceive();
			m_wait.setCancelable(true);
			m_wait.setMessage("Retrieving details for " + m_selected);
			m_wait.setProgressStyle(ProgressDialog.STYLE_SPINNER);
			m_wait.show();
			super.onPreExecute();
		}
		@Override
		protected Integer doInBackground(Void... vd)
		{
			try
    		{
				RequestParams params = new RequestParams("suspect", m_selected);
    			NetworkHandler.get(("https://" + m_global.getURL() + "/getSuspect"), params, new AsyncHttpResponseHandler(){
    				@Override
    				public void onSuccess(String xml)
    				{
    					m_global.setXmlBase(xml);
    					m_global.setMessageReceived(true);
    				}
    				
    				@Override
    				public void onFailure(Throwable err, String content)
    				{
    					m_global.setXmlBase("");
    					m_global.setMessageReceived(true);
    				}
    			});
    			while(!m_global.checkMessageReceived()){};
    		}
    		catch(Exception e)
    		{
    			e.printStackTrace();
    			return 0;
    		}
    		
    		if(m_global.getXmlBase() != "")
    		{
    			return 1;
    		}
    		else
    		{
    			return 0;
    		}
		}
		@Override
		protected void onPostExecute(Integer result)
		{
			if(result == 0)
			{
				m_wait.cancel();
				Toast.makeText(m_gridContext, "Could not get details for " + m_selected, Toast.LENGTH_LONG).show();
			}
			else
			{
				m_wait.cancel();
				m_updateThread.interrupt();
				Intent nextPage = new Intent(getApplicationContext(), DetailsActivity.class);
				nextPage.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				getApplicationContext().startActivity(nextPage);
			}
		}
	}
	
	private class CeresClientConnect extends AsyncTask<String, Void, Integer> {
    	@Override
    	protected void onPreExecute()
    	{
    		super.onPreExecute();
    	}
    	
    	@Override
    	protected Integer doInBackground(String... params)
    	{
    		try
    		{
    			m_global.clearXmlReceive();
    			NetworkHandler.get(("https://" + m_global.getURL() + "/getAll"), null, new AsyncHttpResponseHandler(){
    				@Override
    				public void onSuccess(String xml)
    				{
    					m_global.setXmlBase(xml);
    					m_global.setMessageReceived(true);
    				}
    				
    				@Override
    				public void onFailure(Throwable err, String content)
    				{
    					m_global.setXmlBase("");
    					m_global.setMessageReceived(true);
    				}
    			});
    			while(!m_global.checkMessageReceived()){};
    		}
    		catch(Exception e)
    		{
    			e.printStackTrace();
    			return 0;
    		}
    		
    		if(m_global.getXmlBase() != "")
    		{
    			System.out.println("returning 1");
    			return 1;
    		}
    		else
    		{
    			System.out.println("returning 0");
    			return 0;
    		}
    	}
    	@Override
    	protected void onPostExecute(Integer result)
    	{
			new ParseXml().execute();
    	}
    }
	
	private class ParseXml extends AsyncTask<Void, Integer, ArrayList<String>> {
		@Override
		protected void onPreExecute()
		{
			m_wait.setCancelable(false);
			m_wait.setMessage("Constructing Suspect List");
			m_wait.setProgressStyle(ProgressDialog.STYLE_SPINNER);
			m_wait.show();
    		super.onPreExecute();
		}
		
		@Override
		protected ArrayList<String> doInBackground(Void... vd)
		{
			try
			{
				m_gridValues = constructGridValues();
				return m_gridValues;
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
		protected void onPostExecute(ArrayList<String> gridPop)
		{
			if(gridPop == null || gridPop.size() == 0)
			{
				m_wait.cancel();
				AlertDialog.Builder build = new AlertDialog.Builder(m_gridContext);
				build
				.setTitle("Suspect list empty")
				.setMessage("Ceres server returned no suspects. Try again?")
				.setCancelable(false)
				.setPositiveButton("Yes", new DialogInterface.OnClickListener(){
					@Override
					public void onClick(DialogInterface dialog, int id)
					{
						m_global.clearXmlReceive();
						m_updateThread.interrupt();
					}
				})
				.setNegativeButton("No", new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which)
					{
						m_global.clearXmlReceive();
						m_updateThread.interrupt();
		    			Intent nextPage = new Intent(getApplicationContext(), MainActivity.class);
		    			nextPage.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
		    			nextPage.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		    			getApplicationContext().startActivity(nextPage);
					}
				});
				AlertDialog ad = build.create();
				ad.show();
			}
			else
			{
				Toast.makeText(m_gridContext, gridPop.size() + " suspects loaded", Toast.LENGTH_SHORT).show();
				m_gridAdapter = new ClassificationGridAdapter(m_gridContext, gridPop);
		        setListAdapter(m_gridAdapter);
		        if(!m_updateThread.isAlive())
		        {
		        	m_updateThread.start();
		        }
				m_wait.cancel();
			}
		}
	}
}
