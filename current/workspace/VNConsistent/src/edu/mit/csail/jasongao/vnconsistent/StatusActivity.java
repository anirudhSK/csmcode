package edu.mit.csail.jasongao.vnconsistent;

import java.io.File;
import java.io.PrintWriter;
import java.util.Map;

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
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

public class StatusActivity extends Activity implements LocationListener {
	final static private String TAG = "StatusActivity";

	// UI elements
	Button r00, r01, r10, r11;
	Button ticket_button;
	Button bench_button, cache_button, distribution_button;
	int distributionLevel = 3;
	TextView opCountTv, successCountTv, failureCountTv;
	TextView idTv, stateTv, regionTv, leaderTv;
	ListView msgList;
	ArrayAdapter<String> receivedMessages;

	PowerManager.WakeLock wl = null;
	LocationManager lm;

	// Logging to file
	File myLogFile;
	PrintWriter myLogWriter;

	// Mux
	Mux mux;

	/** Handle messages from various components */
	private final Handler myHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case Mux.LOG:
				receivedMessages.add((String) msg.obj);
				// Write to file
				if (myLogWriter != null) {
					myLogWriter.println((String) msg.obj);
				}
				break;
			case Mux.LOG_NODISPLAY:
				// receivedMessages.add((String) msg.obj);
				// Write to file
				if (myLogWriter != null) {
					myLogWriter.println((String) msg.obj);
				}
				break;
			case Mux.VNC_STATUS_CHANGE:
				update();
				break;
			case Mux.CLIENT_STATUS_CHANGE:
				Map<String, Long> data = (Map<String, Long>) msg.obj;
				opCountTv.setText("ops: " + String.valueOf(data.get("op")));
				successCountTv.setText("successes: "
						+ String.valueOf(data.get("success")));
				failureCountTv.setText("failures: "
						+ String.valueOf(data.get("failure")));

				boolean requestOutstanding = data.get("request_oustanding") == 1L;
				boolean ticketHeld = data.get("ticket_held") == 1L;
				
				if (requestOutstanding) {
					if (ticketHeld)
						ticket_button.setText("Releasing ticket...");
					else
						ticket_button.setText("Taking ticket...");
				} else {
					if (ticketHeld)
						ticket_button.setText("Release ticket");
					else
						ticket_button.setText("Take ticket");
				}
			}
		}
	};

	/** Log message and also display on screen */
	public void logMsg(String msg) {
		receivedMessages.add(msg);
		Log.i(TAG, msg);
	}

	/** Force an update of the screen views */
	public void update() {
		idTv.setText(String.valueOf(mux.vncDaemon.mId));
		leaderTv.setText(String.valueOf(mux.vncDaemon.leaderId));
		regionTv.setText(String.format("(%d,%d)", mux.vncDaemon.myRegion.x,
				mux.vncDaemon.myRegion.y));

		switch (mux.vncDaemon.mState) {
		case VNCDaemon.DORMANT:
			stateTv.setText("DORMANT");
			break;
		case VNCDaemon.JOINING:
			stateTv.setText("JOINING");
			break;
		case VNCDaemon.LEADER:
			stateTv.setText("LEADER");
			break;
		case VNCDaemon.NONLEADER:
			stateTv.setText("NONLEADER");
			break;
		}
	}

	/**
	 * Android application lifecycle management
	 **/

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		// Buttons
		bench_button = (Button) findViewById(R.id.bench_button);
		bench_button.setOnClickListener(bench_button_listener);
		cache_button = (Button) findViewById(R.id.cache_button);
		cache_button.setOnClickListener(cache_button_listener);
		distribution_button = (Button) findViewById(R.id.distribution_button);
		distribution_button.setOnClickListener(distribution_button_listener);

		ticket_button = (Button) findViewById(R.id.ticket_button);
		ticket_button.setOnClickListener(ticket_button_listener);

		r00 = (Button) findViewById(R.id.r00_button);
		r00.setOnClickListener(r00_listener);
		r01 = (Button) findViewById(R.id.r01_button);
		r01.setOnClickListener(r01_listener);
		r10 = (Button) findViewById(R.id.r10_button);
		r10.setOnClickListener(r10_listener);
		r11 = (Button) findViewById(R.id.r11_button);
		r11.setOnClickListener(r11_listener);

		// Text views
		opCountTv = (TextView) findViewById(R.id.opcount_tv);
		successCountTv = (TextView) findViewById(R.id.successcount_tv);
		failureCountTv = (TextView) findViewById(R.id.failurecount_tv);

		// Text views
		idTv = (TextView) findViewById(R.id.id_tv);
		stateTv = (TextView) findViewById(R.id.state_tv);
		regionTv = (TextView) findViewById(R.id.region_tv);
		leaderTv = (TextView) findViewById(R.id.leader_tv);

		msgList = (ListView) findViewById(R.id.msgList);
		receivedMessages = new ArrayAdapter<String>(this, R.layout.message);
		msgList.setAdapter(receivedMessages);

		// Get a wakelock to keep everything running
		PowerManager pm = (PowerManager) getApplicationContext()
				.getSystemService(Context.POWER_SERVICE);
		wl = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK
				| PowerManager.ON_AFTER_RELEASE, TAG);
		wl.acquire();

		lm = (LocationManager) getSystemService(Context.LOCATION_SERVICE);

		// Setup writing to log file on sd card
		boolean mExternalStorageAvailable = false;
		boolean mExternalStorageWriteable = false;
		String state = Environment.getExternalStorageState();
		if (Environment.MEDIA_MOUNTED.equals(state)) {
			// We can read and write the media
			mExternalStorageAvailable = mExternalStorageWriteable = true;
		} else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
			// We can only read the media
			mExternalStorageAvailable = true;
			mExternalStorageWriteable = false;
		} else {
			// Something else is wrong. It may be one of many other states, but
			// all we need to know is we can neither read nor write
			mExternalStorageAvailable = mExternalStorageWriteable = false;
		}

		if (mExternalStorageAvailable && mExternalStorageWriteable) {
			myLogFile = new File(Environment.getExternalStorageDirectory(),
					String.format("csm-%d.txt", System.currentTimeMillis()));
			try {
				myLogWriter = new PrintWriter(myLogFile);
				logMsg("*** Opened log file for writing ***");
			} catch (Exception e) {
				myLogWriter = null;
				logMsg("*** Couldn't open log file for writing ***");
			}
		}

		// Start the mux, which will start the entire VNC, CSM, etc stack
		long id = -1;
		Bundle extras = getIntent().getExtras();
		if (extras != null && extras.containsKey("id")) {
			id = Long.valueOf(extras.getString("id"));
		}
		mux = new Mux(id, myHandler);
		mux.start();

		logMsg("*** Application started ***");
	} // end OnCreate()

	/**
	 * onResume is is always called after onStart, even if userServer's not
	 * paused
	 */
	@Override
	protected void onResume() {
		super.onResume();
		// update if phone moves 5m ( once GPS fix is acquired )
		// or if 5s has passed since last update
		lm.requestLocationUpdates(LocationManager.GPS_PROVIDER, 5000, 5f, this);
	}

	@Override
	protected void onPause() {
		super.onPause();
	}

	@Override
	public void onDestroy() {
		mux.requestStop();

		myLogWriter.flush();
		myLogWriter.close();

		lm.removeUpdates(this);
		if (wl != null)
			wl.release();
		super.onDestroy();
	}

	/*** UI Callbacks for Buttons, etc. ***/
	private OnClickListener bench_button_listener = new OnClickListener() {
		public void onClick(View v) {
			// toggle benchmark in userClient
			if (mux != null && mux.userClient != null) {
				if (!mux.userClient.isBenchmarkOn()) {
					bench_button.setText("Stop Bench");
					logMsg("*** benchmark starting ***");
					mux.userClient.startBenchmark();
				} else {
					bench_button.setText("Start Bench");
					mux.userClient.stopBenchmark();
					logMsg("*** benchmark stopped ***");
				}
			}
			update();
		}
	};

	private OnClickListener cache_button_listener = new OnClickListener() {
		// toggle caching in vncDaemon (which will toggle in CSMLayer)
		public void onClick(View v) {
			if (mux.vncDaemon.cacheEnabled) {
				cache_button.setText("Cache is OFF");
				mux.vncDaemon.disableCaching();
			} else {
				cache_button.setText("Cache is ON");
				mux.vncDaemon.enableCaching();
			}
		}
	};

	private OnClickListener distribution_button_listener = new OnClickListener() {
		public void onClick(View v) {
			switch (distributionLevel) {
			case 3:
				distributionLevel = 1;
				logMsg("Set distribution to 90% reads.");
				distribution_button.setText("90%");
				mux.userClient.setReadWriteDistribution(0.9);
				break;
			case 1:
				distributionLevel = 2;
				logMsg("Set distribution to 60% reads.");
				distribution_button.setText("60%");
				mux.userClient.setReadWriteDistribution(0.6);
				break;
			case 2:
				distributionLevel = 3;
				logMsg("Set distribution to 30% reads.");
				distribution_button.setText("30%");
				mux.userClient.setReadWriteDistribution(0.3);
			}
		}
	};

	private OnClickListener r00_listener = new OnClickListener() {
		public void onClick(View v) {
			mux.vncDaemon.changeRegion(new RegionKey(0, 0));
		}
	};
	private OnClickListener r01_listener = new OnClickListener() {
		public void onClick(View v) {
			mux.vncDaemon.changeRegion(new RegionKey(0, 1));
		}
	};
	private OnClickListener r10_listener = new OnClickListener() {
		public void onClick(View v) {
			mux.vncDaemon.changeRegion(new RegionKey(1, 0));
		}
	};
	private OnClickListener r11_listener = new OnClickListener() {
		public void onClick(View v) {
			mux.vncDaemon.changeRegion(new RegionKey(1, 1));
		}
	};

	private OnClickListener read_all_listener = new OnClickListener() {
		public void onClick(View v) {
			mux.userClient.requestRead(0, 0);
			mux.userClient.requestRead(0, 1);
			mux.userClient.requestRead(1, 0);
			mux.userClient.requestRead(1, 1);
		}
	};

	private OnClickListener ticket_button_listener = new OnClickListener() {
		public void onClick(View v) {
			if (!mux.userClient.isTicketHeld()) {
				ticket_button.setText("Taking ticket...");
				mux.userClient.requestDecrementInCurrentRegion();
			} else {
				ticket_button.setText("Releasing ticket...");
				mux.userClient.requestIncrement();
			}
		}
	};

	private OnClickListener readparkingA_listener = new OnClickListener() {
		public void onClick(View v) {
			mux.userClient.requestRead(0, 0);
		}
	};
	private OnClickListener requestparkingA_listener = new OnClickListener() {
		public void onClick(View v) {
			mux.userClient.requestDecrement(0, 0);
		}
	};

	/***
	 * Location / GPS Stuff adapted from
	 * http://hejp.co.uk/android/android-gps-example/
	 */

	/** Called when a location update is received */
	@Override
	public void onLocationChanged(Location loc) {
		
		if(loc!=null) {
			mux.vncDaemon.checkLocation(loc);
		}
		else  {
			logMsg("Null Location");
		}
	}

	@Override
	public void onProviderDisabled(String arg0) {
	}

	@Override
	public void onProviderEnabled(String arg0) {
	}

	/** Called upon change in GPS status */
	@Override
	public void onStatusChanged(String provider, int status, Bundle extras) {
		switch (status) {
		case LocationProvider.OUT_OF_SERVICE:
			logMsg("GPS out of service");
			break;
		case LocationProvider.TEMPORARILY_UNAVAILABLE:
			logMsg("GPS temporarily unavailable");
			break;
		case LocationProvider.AVAILABLE:
			logMsg("GPS available");
			break;
		}
	}
}
