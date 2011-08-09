package edu.mit.csail.jasongao.sonar;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutput;
import java.io.ObjectOutputStream;
import java.io.PrintWriter;

import android.app.Activity;
import android.content.Context;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.location.LocationProvider;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;
import android.widget.ArrayAdapter;
import android.widget.ListView;

public class SonarActivity extends Activity implements LocationListener {
	final static private String TAG = "SonarActivity";

	// Attributes
	private long nodeId = -1;
	private Location myLoc;

	// UI - Log display
	ListView msgList;
	ArrayAdapter<String> receivedMessages;

	// Logging to file
	File logFile;
	PrintWriter logWriter;

	// Network
	private NetworkThread netThread;

	// Android components
	PowerManager.WakeLock wakeLock = null;
	LocationManager locManager;

	private boolean recurringPingEnabled = false;
	private final static long recurringPingPeriod = 1000L;

	// Keep sending a ping packet
	private Runnable recurringPingR = new Runnable() {
		@Override
		public void run() {
			sendPing();
			if (recurringPingEnabled) {
				myHandler.postDelayed(recurringPingR, recurringPingPeriod);
			}
		}
	};

	// Handler message types
	protected final static int LOG = 3;
	protected final static int PACKET_RECV = 4;

	/** Handle messages from various components */
	private final Handler myHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case PACKET_RECV:
				Packet rxPacket= readPacket((byte[]) msg.obj);
				if(rxPacket.senderId != nodeId) logMsg("Received " + rxPacket.toString());
				// Log the packet iff it's not a packet that you yourself sent out. 
				break;
			case LOG: // Write a string to log file and UI log display
				logMsg((String) msg.obj);
				break;
			}
		}
	};

	/** Log message and also display on screen */
	public void logMsg(String line) {
		line = String.format("%d: %s", System.currentTimeMillis(), line);
		Log.i(TAG, line);
		receivedMessages.add((String) line);
		if (logWriter != null) {
			logWriter.println((String) line);
		}
	}

	/**
	 * Read back a packet.
	 * 
	 * @throws Exception
	 */
	private Packet readPacket(byte[] data) {
		Packet packet = null;
		ByteArrayInputStream bis = new ByteArrayInputStream(data);
		ObjectInputStream ois = null;
		try {
			ois = new ObjectInputStream(bis);
		} catch (IOException e) {
			Log.e(TAG, "error on new ObjectInputStream: " + e.getMessage());
			return null;
		}
		try {
			packet = (Packet) ois.readObject();
		} catch (ClassNotFoundException e) {
			Log.e(TAG,
					"ClassNotFoundException reading object from ois: "
							+ e.getMessage());
			e.printStackTrace();
		} catch (IOException e) {
			Log.e(TAG, "IOException reading object from ois: " + e.getClass()
					+ ": " + e.getMessage());
		}
		try {
			ois.close();
		} catch (IOException e) {
			Log.e(TAG, "error closing ois: " + e.getMessage());
		}
		return packet;
	}

	/** Send ping */
	private void sendPing() {
		Packet ping = new Packet(nodeId);
		ping.setSenderLoc(myLoc);
		logMsg("Sending " + ping);
		sendPacket(ping);
	}

	/** Send a packet to the network thread */
	private void sendPacket(Packet packet) {
		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		try {
			ObjectOutput out = new ObjectOutputStream(bos);
			out.writeObject(packet);
			out.close();
			byte[] data = bos.toByteArray();
			netThread.sendData(data);
		} catch (IOException e) {
			Log.e(TAG, "error sending packet:" + e.getMessage());
		}
	}

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		msgList = (ListView) findViewById(R.id.msgList);
		receivedMessages = new ArrayAdapter<String>(this, R.layout.message);
		msgList.setAdapter(receivedMessages);

		logMsg("*** Application started ***");

		// Setup writing to log file on sd card
		String state = Environment.getExternalStorageState();
		if (Environment.MEDIA_MOUNTED.equals(state)) {
			// We can read and write the media
			logFile = new File(Environment.getExternalStorageDirectory(),
					String.format("sonar-%d.txt", System.currentTimeMillis()));
			try {
				logWriter = new PrintWriter(logFile);
				logMsg("*** Opened log file for writing ***");
			} catch (Exception e) {
				logWriter = null;
				logMsg("*** Couldn't open log file for writing ***");
			}
		} else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
			// We can only read the media
		} else {
			// One of many other states, but we can neither read nor write
		}

		// Get a wakelock to keep everything running
		PowerManager pm = (PowerManager) getApplicationContext()
				.getSystemService(Context.POWER_SERVICE);
		wakeLock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK
				| PowerManager.ON_AFTER_RELEASE, TAG);

		// Location / GPS
		locManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);

		// Start the network thread and ensure it's running
		netThread = new NetworkThread(myHandler);
		if (!netThread.socketIsOK()) {
			Log.e(TAG, "Cannot start server: socket not ok.");
			return; // quit out
		}
		netThread.start();
		if (netThread.getLocalAddress() == null) {
			Log.e(TAG, "Couldn't get my IP address.");
			return; // quit out
		}

		Bundle extras = getIntent().getExtras();
		if (extras != null && extras.containsKey("id")) {
			nodeId = Long.valueOf(extras.getString("id"));
		} else {
			nodeId = 1000 * netThread.getLocalAddress().getAddress()[2]
					+ netThread.getLocalAddress().getAddress()[3];
		}
		
		logMsg("nodeId=" + nodeId);
	}

	/** Always called after onStart, even if activity is not paused. */
	@Override
	protected void onResume() {
		super.onResume();
		wakeLock.acquire();
		locManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 5000,
				5f, this);

		// Start the ping
		recurringPingEnabled = true;
		myHandler.postDelayed(recurringPingR, recurringPingPeriod);
	}

	@Override
	protected void onPause() {
		recurringPingEnabled = false;
		myHandler.removeCallbacks(recurringPingR);
		locManager.removeUpdates(this);
		wakeLock.release();

		super.onPause();
	}

	@Override
	public void onDestroy() {
		netThread.closeSocket();
		
		logWriter.flush();
		logWriter.close();

		super.onDestroy();
	}

	/** Location - location changed */
	@Override
	public void onLocationChanged(Location loc) {
		myLoc = loc;
	}

	/** Location - provider disabled */
	@Override
	public void onProviderDisabled(String arg0) {
	}

	/** Location - provider disabled */
	@Override
	public void onProviderEnabled(String arg0) {
	}

	/** Location - provider status changed */
	@Override
	public void onStatusChanged(String provider, int status, Bundle extras) {
		switch (status) {
		case LocationProvider.OUT_OF_SERVICE:
			logMsg("LocationProvider out of service");
			break;
		case LocationProvider.TEMPORARILY_UNAVAILABLE:
			logMsg("LocationProvider temporarily unavailable");
			break;
		case LocationProvider.AVAILABLE:
			logMsg("LocationProvider available");
			break;
		}
	}
}
