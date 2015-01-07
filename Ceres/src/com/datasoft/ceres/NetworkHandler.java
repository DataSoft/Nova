package com.datasoft.ceres;

import java.io.IOException;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableKeyException;
import java.security.cert.CertificateException;
import org.apache.http.Header;
import org.apache.http.auth.UsernamePasswordCredentials;
import org.apache.http.conn.ssl.SSLSocketFactory;
import org.apache.http.conn.ssl.X509HostnameVerifier;
import org.apache.http.impl.auth.BasicScheme;
import android.content.Context;
import android.content.res.Resources.NotFoundException;
import com.loopj.android.http.*;

public class NetworkHandler {
	private static AsyncHttpClient m_client = new AsyncHttpClient();
	private static Header m_headerParam;
	private static Context m_ctx;
	
	public static void setSSL(Context ctx, int keyid, String pass, String user)
	{
		try
		{
			m_ctx = ctx;
			KeyStore keystore = KeyStore.getInstance("BKS");
			// TODO: When/If this app goes out to the world, have to find some way to procure
			// and store the password for the keystore; we're going to have basic authentication
			// (username and password) as well as the password for the keystore, which should only 
			// be entered once (if at all)
			char[] kspass = "toortoor".toCharArray();
			keystore.load(ctx.getResources().openRawResource(keyid), kspass);
			SSLSocketFactory sslsf = new SSLSocketFactory(keystore);
			// TODO: This will have to go away when the app hits production; a workaround will have to 
			// be formed. What this statement does, right now, is use SSL only as data encryption, without
			// authenticating that the server you're connecting to is the actual server. Unfortunately, to create
			// a development version of this app that acts as it should in a production environment, there would 
			// be a lot of code rewrite and probably required rewriting of some library code. In the meantime I 
			// intend to find a way to do this that will satisfy both a debug environment as well as a production one,
			// but for now this will have to suffice.
			sslsf.setHostnameVerifier((X509HostnameVerifier)SSLSocketFactory.ALLOW_ALL_HOSTNAME_VERIFIER);
			m_client.setSSLSocketFactory(sslsf);
			UsernamePasswordCredentials creds = new UsernamePasswordCredentials(user, pass);
			m_headerParam = BasicScheme.authenticate(creds, "UTF-8", false);
		}
		catch(KeyStoreException e)
		{
			e.printStackTrace();
		}
		catch(CertificateException e) 
		{
			e.printStackTrace();
		}
		catch(NoSuchAlgorithmException e)
		{
			e.printStackTrace();
		}
		catch(NotFoundException e)
		{
			e.printStackTrace();
		}
		catch (KeyManagementException e)
		{
			e.printStackTrace();
		}
		catch(IOException e)
		{
			e.printStackTrace();
		}
		catch (UnrecoverableKeyException e)
		{
			e.printStackTrace();
		}
	}
	
	public static void get(String url, RequestParams params, AsyncHttpResponseHandler response)
	{
		Header[] pass = {m_headerParam};
		m_client.get(m_ctx, url, pass, params, response);
	}
}
