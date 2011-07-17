package edu.mit.csail.jasongao.smcloud;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.Reader;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Random;

import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;

import android.app.Activity;
import android.content.Context;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.location.LocationProvider;
import android.os.AsyncTask;
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

import com.google.gson.Gson;

public class StatusActivity extends Activity implements LocationListener {
	final static private String TAG = "SMC:StatusActivity";
	final static String hostname = "ec2-122-248-219-48.ap-southeast-1.compute.amazonaws.com:4212";

	// Logging to file
	File myLogFile;
	PrintWriter myLogWriter;

	// Attributes
	private long maxRx = 1, maxRy = 1;
	private double readVsWriteDistribution = 0.9;

	// UI elements
	Button benchmark, releaseparking;
	Button requestparkingA, readparkingA;
	Button requestparkingB, readparkingB;
	Button requestparkingC, readparkingC;
	Button setdistribution30, setdistribution60, setdistribution90;
	TextView opCountTv, successCountTv, failureCountTv;
	ListView msgList;
	ArrayAdapter<String> receivedMessages;

	PowerManager.WakeLock wl = null;
	LocationManager lm;

	Random rand = new Random();

	private boolean writeTaskOutstanding = false; // in middle of request or
													// release?
	private boolean spotHeld = false; // are we currently holding a parking spot
	private RegionKey spotHeldRegion;

	private long successCount = 0;
	private long failureCount = 0;
	private long opCount = 0;
	private boolean benchmarkOn = false;
	final static private long benchmarkIterationDelay = 1000L;

	public class CloudResult {
		// Status codes
		final static int CR_ERROR = 13;
		final static int CR_OKAY = 12;
		final static int CR_NOCSM = 11;
		final static int CR_CSM = 10;

		public int status;
		public long spots;
		public long latency;

		CloudResult(int s, long r) {
			status = s;
			spots = r;
		}
	}

	/** Benchmark loop iteration */
	Runnable benchmarkIterationR = new Runnable() {
		@Override
		public void run() {
			if (benchmarkOn) {
				// Pick a random region to send request to
				long dstRx = rand.nextInt((int) (maxRx + 1));
				long dstRy = rand.nextInt((int) (maxRy + 1));

				// pick read or write according to distribution
				if (rand.nextDouble() < readVsWriteDistribution) {
					// make a read
					readParkingClick(dstRx, dstRy);
				} else {
					// make a write-involving operation (request or release)
					if (!spotHeld) {
						requestParkingClick(dstRx, dstRy);
					} else {
						releaseParkingClick();
					}
				}
			}
		}
	};

	private class ReadParkingTask extends AsyncTask<Long, Integer, CloudResult> {
		protected CloudResult doInBackground(Long... args) {
			opCount++;

			String url = String.format("http://" + hostname
					+ "/readparking/%d/%d/%d/%d/", args[0], args[1], args[2],
					args[3]);
			try {
				long t1 = System.currentTimeMillis();
				CloudResult cr = makeCloudRequest(url);
				cr.latency = System.currentTimeMillis() - t1;
				return cr;
			} catch (Exception e) {
				Log.e(TAG, "ReadParkingTask exception: " + e.getMessage());
				return null;
			}
		}

		protected void onPostExecute(CloudResult cr) {
			if (cr == null) {
				logMsg("Parking read failed, error contacting cloud.");
				failureCount++;
			} else if (cr.status == CloudResult.CR_OKAY) {
				logMsg("Parking read succeeded, spots=" + cr.spots
						+ ",latency=" + cr.latency);
				successCount++;
			} else if (cr.status == CloudResult.CR_ERROR) {
				logMsg("Parking read rejected, spots=" + cr.spots + ",latency="
						+ cr.latency);
				successCount++;
			}
			update();

			if (benchmarkOn) {
				myHandler.postDelayed(benchmarkIterationR,
						benchmarkIterationDelay);
			}
		}
	}

	private class ReleaseParkingTask extends
			AsyncTask<Long, Integer, CloudResult> {
		protected CloudResult doInBackground(Long... args) {
			opCount++;

			String url = String.format("http://" + hostname
					+ "/releaseparking/%d/%d/%d/%d/", args[0], args[1],
					args[2], args[3]);
			try {
				long t1 = System.currentTimeMillis();
				CloudResult cr = makeCloudRequest(url);
				cr.latency = System.currentTimeMillis() - t1;
				return cr;
			} catch (Exception e) {
				Log.e(TAG, "ReleaseParkingTask exception: " + e.getMessage());
				return null;
			}
		}

		protected void onPostExecute(CloudResult cr) {
			if (cr == null) {
				logMsg("Parking release failed, error contacting cloud.");
				failureCount++;
			} else if (cr.status == CloudResult.CR_OKAY) {
				spotHeld = false;
				logMsg("Parking release succeeded, spots=" + cr.spots
						+ ",latency=" + cr.latency);
				successCount++;
			} else if (cr.status == CloudResult.CR_ERROR) {
				logMsg("Parking release rejected, spots=" + cr.spots
						+ ",latency=" + cr.latency);
				successCount++;
			}
			writeTaskOutstanding = false;
			update();

			if (benchmarkOn) {
				myHandler.postDelayed(benchmarkIterationR,
						benchmarkIterationDelay);
			}
		}
	}

	private class RequestParkingTask extends
			AsyncTask<Long, Integer, CloudResult> {
		protected CloudResult doInBackground(Long... args) {
			opCount++;

			String url = String.format("http://" + hostname
					+ "/requestparking/%d/%d/%d/%d/", args[0], args[1],
					args[2], args[3]);
			try {
				long t1 = System.currentTimeMillis();
				CloudResult cr = makeCloudRequest(url);
				cr.latency = System.currentTimeMillis() - t1;
				return cr;
			} catch (Exception e) {
				Log.e(TAG, "RequestParkingTask exception: " + e.getMessage());
				return null;
			}
		}

		protected void onPostExecute(CloudResult cr) {
			if (cr == null) {
				logMsg("Parking request failed, error contacting cloud.");
				failureCount++;
			} else if (cr.status == CloudResult.CR_OKAY) {
				spotHeld = true;
				logMsg("Parking request succeeded, spots=" + cr.spots
						+ ",latency=" + cr.latency);
				successCount++;
			} else if (cr.status == CloudResult.CR_ERROR) {
				logMsg("Parking request rejected, spots=" + cr.spots
						+ ",latency=" + cr.latency);
				successCount++;
			}
			writeTaskOutstanding = false;
			update();

			if (benchmarkOn) {
				myHandler.postDelayed(benchmarkIterationR,
						benchmarkIterationDelay);
			}
		}
	}

	/** Handle messages from various components */
	private final Handler myHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			// TODO
			}
		}
	};

	/** Log message and also display on screen */
	public void logMsg(String msg) {
		msg = String.format("%d: %s", System.currentTimeMillis(), msg);
		receivedMessages.add(msg);
		Log.i(TAG, msg);

		// Also write to file
		if (myLogWriter != null) {
			myLogWriter.println(msg);
		}
	}

	/**
	 * Android application lifecycle management
	 **/

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		Log.i(TAG, "onCreate");
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		// Buttons
		setdistribution30 = (Button) findViewById(R.id.setdistribution30_button);
		setdistribution30.setOnClickListener(setdistribution30_listener);
		setdistribution60 = (Button) findViewById(R.id.setdistribution60_button);
		setdistribution60.setOnClickListener(setdistribution60_listener);
		setdistribution90 = (Button) findViewById(R.id.setdistribution90_button);
		setdistribution90.setOnClickListener(setdistribution90_listener);

		benchmark = (Button) findViewById(R.id.testcsm_button);
		benchmark.setOnClickListener(benchmark_listener);
		releaseparking = (Button) findViewById(R.id.releaseparking_button);
		releaseparking.setOnClickListener(releaseparking_listener);

		requestparkingA = (Button) findViewById(R.id.requestparkingA_button);
		requestparkingA.setOnClickListener(requestparkingA_listener);
		readparkingA = (Button) findViewById(R.id.readparkingA_button);
		readparkingA.setOnClickListener(readparkingA_listener);

		requestparkingB = (Button) findViewById(R.id.requestparkingB_button);
		requestparkingB.setOnClickListener(requestparkingB_listener);
		readparkingB = (Button) findViewById(R.id.readparkingB_button);
		readparkingB.setOnClickListener(readparkingB_listener);

		requestparkingC = (Button) findViewById(R.id.requestparkingC_button);
		requestparkingC.setOnClickListener(requestparkingC_listener);
		readparkingC = (Button) findViewById(R.id.readparkingC_button);
		readparkingC.setOnClickListener(readparkingC_listener);

		// Text views
		opCountTv = (TextView) findViewById(R.id.opcount_tv);
		successCountTv = (TextView) findViewById(R.id.successcount_tv);
		failureCountTv = (TextView) findViewById(R.id.failurecount_tv);
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
					String.format("smcloud-%d.txt", System.currentTimeMillis()));
			try {
				myLogWriter = new PrintWriter(myLogFile);
				logMsg("*** Opened log file for writing ***");
			} catch (Exception e) {
				logMsg("*** Couldn't open log file for writing ***");
			}
		}

		logMsg("*** Application started ***");
	} // end OnCreate()

	/** Force an update of the screen views */
	public void update() {
		opCountTv.setText(String.format("ops: %d", opCount));
		successCountTv.setText(String.format("successes: %d", successCount));
		failureCountTv.setText(String.format("failures: %d", failureCount));
	}

	/**
	 * onResume is is always called after onStart, even if userServer's not
	 * paused
	 */
	@Override
	protected void onResume() {
		super.onResume();
		// request updates every 60s or 5m, whichever first.
		lm.requestLocationUpdates(LocationManager.GPS_PROVIDER, 60000, 5f, this);
	}

	@Override
	protected void onPause() {
		super.onPause();
	}

	@Override
	public void onDestroy() {
		benchmarkOn = false;
		if (spotHeld && !writeTaskOutstanding) {
			writeTaskOutstanding = true;
			logMsg("Releasing spot in " + spotHeldRegion);

			new ReleaseParkingTask().execute(spotHeldRegion.x,
					spotHeldRegion.y, 0L, System.currentTimeMillis());
		}
		myLogWriter.flush();
		myLogWriter.close();

		lm.removeUpdates(this);
		if (wl != null)
			wl.release();
		super.onDestroy();
	}

	/*** UI Callbacks for Buttons, etc. ***/
	private OnClickListener setdistribution30_listener = new OnClickListener() {
		public void onClick(View v) {
			logMsg("Set distribution to 30% reads.");
			readVsWriteDistribution = 0.3;
		}
	};
	private OnClickListener setdistribution60_listener = new OnClickListener() {
		public void onClick(View v) {
			logMsg("Set distribution to 60% reads.");
			readVsWriteDistribution = 0.6;
		}
	};
	private OnClickListener setdistribution90_listener = new OnClickListener() {
		public void onClick(View v) {
			logMsg("Set distribution to 90% reads.");
			readVsWriteDistribution = 0.9;
		}
	};

	private void readParkingClick(long rx, long ry) {
		RegionKey readSpotRegion = new RegionKey(rx, ry);
		logMsg("Reading spot in " + readSpotRegion);
		new ReadParkingTask().execute(readSpotRegion.x, readSpotRegion.y, 0L,
				System.currentTimeMillis());
	}

	private OnClickListener readparkingA_listener = new OnClickListener() {
		public void onClick(View v) {
			readParkingClick(0, 0);
		}
	};
	private OnClickListener readparkingB_listener = new OnClickListener() {
		public void onClick(View v) {
			readParkingClick(1, 0);
		}
	};
	private OnClickListener readparkingC_listener = new OnClickListener() {
		public void onClick(View v) {
			readParkingClick(2, 0);
		}
	};

	private void requestParkingClick(long rx, long ry) {
		if (!spotHeld && !writeTaskOutstanding) {
			writeTaskOutstanding = true;
			spotHeldRegion = new RegionKey(rx, ry);
			logMsg("Requesting spot in " + spotHeldRegion);
			new RequestParkingTask().execute(spotHeldRegion.x,
					spotHeldRegion.y, 0L, System.currentTimeMillis());
		} else if (writeTaskOutstanding) {
			logMsg("Wait for previous action to complete.");
		} else if (spotHeld) {
			logMsg("You are already holding a spot!" + spotHeldRegion);
		}
	}

	private OnClickListener requestparkingA_listener = new OnClickListener() {
		public void onClick(View v) {
			requestParkingClick(0, 0);
		}
	};
	private OnClickListener requestparkingB_listener = new OnClickListener() {
		public void onClick(View v) {
			requestParkingClick(1, 0);
		}
	};
	private OnClickListener requestparkingC_listener = new OnClickListener() {
		public void onClick(View v) {
			requestParkingClick(2, 0);
		}
	};

	private void releaseParkingClick() {
		if (spotHeld && !writeTaskOutstanding) {
			writeTaskOutstanding = true;
			logMsg("Releasing spot in " + spotHeldRegion);

			new ReleaseParkingTask().execute(spotHeldRegion.x,
					spotHeldRegion.y, 0L, System.currentTimeMillis());
		} else if (writeTaskOutstanding) {
			logMsg("Wait for previous action to complete.");
		} else if (!spotHeld) {
			logMsg("You are not holding a spot!");
		}
	}

	private OnClickListener releaseparking_listener = new OnClickListener() {
		public void onClick(View v) {
			releaseParkingClick();
		}
	};
	private OnClickListener benchmark_listener = new OnClickListener() {
		public void onClick(View v) {
			if (!benchmarkOn) {
				benchmark.setText("Stop Bench");
				logMsg("*** benchmark starting ***");
				startBenchmark();
			} else {
				benchmark.setText("Start Bench");
				stopBenchmark();
				logMsg("*** benchmark stopped ***");
			}
		}
	};

	/** Start the benchmark iteration loop */
	public synchronized void startBenchmark() {
		benchmarkOn = true;
		myHandler.post(benchmarkIterationR);
	}

	/** Stop the benchmark iteration loop */
	public synchronized void stopBenchmark() {
		benchmarkOn = false;
		myHandler.removeCallbacks(benchmarkIterationR);
	}

	/***
	 * Location / GPS Stuff adapted from
	 * http://hejp.co.uk/android/android-gps-example/
	 */

	/** Called when a location update is received */
	@Override
	public void onLocationChanged(Location loc) {
	}

	/** Bring up the GPS settings if/when the GPS is disabled. */
	@Override
	public void onProviderDisabled(String arg0) {
	}

	/** Called if/when the GPS is enabled in settings */
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

	/**
	 * Make an HTTP GET request to the cloud
	 * 
	 * @throws URISyntaxException
	 * @throws IOException
	 * @throws ClientProtocolException
	 */
	public CloudResult makeCloudRequest(String url) throws URISyntaxException,
			ClientProtocolException, IOException {
		InputStream data = null;
		URI uri = new URI(url);
		HttpGet method = new HttpGet(uri);
		DefaultHttpClient httpClient = new DefaultHttpClient();
		HttpResponse response = httpClient.execute(method);
		data = response.getEntity().getContent();
		Reader r = new InputStreamReader(data);
		Gson gson = new Gson();
		return gson.fromJson(r, CloudResult.class);
	}
}