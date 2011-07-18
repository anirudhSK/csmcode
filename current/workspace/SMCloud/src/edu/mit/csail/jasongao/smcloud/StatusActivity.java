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
	RegionKey myRegion;
	private long maxRx = 1, maxRy = 1;
	private double readVsWriteDistribution = 0.9;

	// UI elements
	Button r00, r01, r10, r11;
	Button ticket_button, read_button;
	Button bench_button, distribution_button;
	int distributionLevel = 3;
	int readButtonState = 0;
	TextView opCountTv, successCountTv, failureCountTv;
	ListView msgList;
	ArrayAdapter<String> receivedMessages;

	PowerManager.WakeLock wl = null;
	LocationManager lm;

	Random rand = new Random();

	private boolean requestOutstanding = false; // in middle of inc or dec?
	private boolean ticketHeld = false; // are we currently holding a ticket?
	private RegionKey ticketRegion; // where are we holding a ticket for?

	private long successCount = 0;
	private long failureCount = 0;
	private long opCount = 0;
	private boolean benchmarkOn = false;
	final static private long benchmarkIterationDelay = 2000L;

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
					readClick(dstRx, dstRy);
				} else {
					// make a write-involving operation (request or release)
					if (!ticketHeld) {
						requestClick(dstRx, dstRy);
					} else {
						releaseClick();
					}
				}
			}
		}
	};

	private class ReadTask extends AsyncTask<Long, Integer, CloudResult> {
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
				Log.e(TAG, "ReadTask exception: " + e.getMessage());
				return null;
			}
		}

		protected void onPostExecute(CloudResult cr) {
			if (cr == null) {
				logMsg("UserClient read failed, error contacting cloud.");
				failureCount++;
			} else if (cr.status == CloudResult.CR_OKAY) {
				logMsg("UserClient read succeeded, value=" + cr.spots
						+ ",latency=" + cr.latency);
				successCount++;
			} else if (cr.status == CloudResult.CR_ERROR) {
				logMsg("UserClient read rejected, value=" + cr.spots
						+ ",latency=" + cr.latency);
				successCount++;
			}
			requestOutstanding = false;
			update();

			if (benchmarkOn) {
				myHandler.postDelayed(benchmarkIterationR,
						benchmarkIterationDelay);
			}
		}
	}

	private class ReleaseTask extends AsyncTask<Long, Integer, CloudResult> {
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
				Log.e(TAG, "ReleaseTask exception: " + e.getMessage());
				return null;
			}
		}

		protected void onPostExecute(CloudResult cr) {
			if (cr == null) {
				logMsg("Ticket release failed, error contacting cloud.");
				failureCount++;
			} else if (cr.status == CloudResult.CR_OKAY) {
				ticketHeld = false;
				ticket_button.setText("Take ticket");
				logMsg("Ticket release succeeded, value=" + cr.spots
						+ ",latency=" + cr.latency);
				successCount++;
			} else if (cr.status == CloudResult.CR_ERROR) {
				logMsg("Ticket release rejected, value=" + cr.spots
						+ ",latency=" + cr.latency);
				successCount++;
			}
			requestOutstanding = false;
			update();

			if (benchmarkOn) {
				myHandler.postDelayed(benchmarkIterationR,
						benchmarkIterationDelay);
			}
		}
	}

	private class RequestTask extends AsyncTask<Long, Integer, CloudResult> {
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
				Log.e(TAG, "RequestTask exception: " + e.getMessage());
				return null;
			}
		}

		protected void onPostExecute(CloudResult cr) {
			if (cr == null) {
				logMsg("Ticket request failed, error contacting cloud.");
				failureCount++;
			} else if (cr.status == CloudResult.CR_OKAY) {
				ticketHeld = true;
				ticket_button.setText("Release ticket");
				logMsg("Ticket request succeeded, value=" + cr.spots
						+ ",latency=" + cr.latency);
				successCount++;
			} else if (cr.status == CloudResult.CR_ERROR) {
				logMsg("Ticket request rejected, value=" + cr.spots
						+ ",latency=" + cr.latency);
				successCount++;
			}
			requestOutstanding = false;
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

		// Default region to start off in
		myRegion = new RegionKey(0, 0);

		// Buttons
		bench_button = (Button) findViewById(R.id.bench_button);
		bench_button.setOnClickListener(bench_button_listener);
		distribution_button = (Button) findViewById(R.id.distribution_button);
		distribution_button.setOnClickListener(distribution_button_listener);
		ticket_button = (Button) findViewById(R.id.ticket_button);
		ticket_button.setOnClickListener(ticket_button_listener);
		read_button = (Button) findViewById(R.id.read_button);
		read_button.setOnClickListener(read_button_listener);

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
		// request updates every 5s or 5m, whichever first.
		lm.requestLocationUpdates(LocationManager.GPS_PROVIDER, 5000, 5f, this);
	}

	@Override
	protected void onPause() {
		super.onPause();
	}

	@Override
	public void onDestroy() {
		benchmarkOn = false;
		if (ticketHeld && !requestOutstanding) {
			requestOutstanding = true;
			logMsg("Releasing ticket in " + ticketRegion);

			new ReleaseTask().execute(ticketRegion.x, ticketRegion.y, 0L,
					System.currentTimeMillis());
		}
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
			if (!benchmarkOn) {
				bench_button.setText("Stop Bench");
				logMsg("*** benchmark starting ***");
				startBenchmark();
			} else {
				bench_button.setText("Start Bench");
				stopBenchmark();
				logMsg("*** benchmark stopped ***");
			}
			update();
		}
	};

	private OnClickListener distribution_button_listener = new OnClickListener() {
		public void onClick(View v) {
			switch (distributionLevel) {
			case 3:
				distributionLevel = 1;
				logMsg("Set distribution to 90% reads.");
				distribution_button.setText("90%");
				readVsWriteDistribution = 0.9;
				break;
			case 1:
				distributionLevel = 2;
				logMsg("Set distribution to 60% reads.");
				distribution_button.setText("60%");
				readVsWriteDistribution = 0.6;
				break;
			case 2:
				distributionLevel = 3;
				logMsg("Set distribution to 30% reads.");
				distribution_button.setText("30%");
				readVsWriteDistribution = 0.3;
			}
		}
	};

	private OnClickListener r00_listener = new OnClickListener() {
		public void onClick(View v) {
			myRegion = new RegionKey(0, 0);
			logMsg("Moved to region " + myRegion);
		}
	};
	private OnClickListener r01_listener = new OnClickListener() {
		public void onClick(View v) {
			myRegion = new RegionKey(0, 1);
			logMsg("Moved to region " + myRegion);
		}
	};
	private OnClickListener r10_listener = new OnClickListener() {
		public void onClick(View v) {
			myRegion = new RegionKey(1, 0);
			logMsg("Moved to region " + myRegion);
		}
	};
	private OnClickListener r11_listener = new OnClickListener() {
		public void onClick(View v) {
			myRegion = new RegionKey(1, 1);
			logMsg("Moved to region " + myRegion);
		}
	};

	private OnClickListener read_button_listener = new OnClickListener() {
		public void onClick(View v) {
			switch (readButtonState) {
			case 0:
				readButtonState = 1;
				readClick(0, 0);
				break;
			case 1:
				readButtonState = 2;
				readClick(1, 0);
				break;
			case 2:
				readButtonState = 3;
				readClick(0, 1);
				break;
			case 3:
				readButtonState = 0;
				readClick(1, 1);
				break;
			}
		}
	};

	private OnClickListener ticket_button_listener = new OnClickListener() {
		public void onClick(View v) {
			if (!ticketHeld) {
				requestClick(myRegion.x, myRegion.y);
			} else {
				releaseClick();
			}
		}
	};

	private void readClick(long rx, long ry) {
		if (!requestOutstanding) {
			requestOutstanding = true;
			logMsg(String.format("Reading (%d,%d)", rx, ry));
			new ReadTask().execute(rx, ry, 0L, System.currentTimeMillis());
		} else {
			logMsg("Wait for previous action to complete.");
		}
	}

	private void requestClick(long rx, long ry) {
		if (!ticketHeld && !requestOutstanding) {
			requestOutstanding = true;
			ticket_button.setText("Taking ticket...");
			ticketRegion = new RegionKey(rx, ry);
			logMsg("Requesting ticket in " + ticketRegion);
			new RequestTask().execute(ticketRegion.x, ticketRegion.y, 0L,
					System.currentTimeMillis());
		} else if (requestOutstanding) {
			logMsg("Wait for previous action to complete.");
		} else if (ticketHeld) {
			logMsg("You are already holding a ticket!" + ticketRegion);
		}
	}

	private void releaseClick() {
		if (ticketHeld && !requestOutstanding) {
			requestOutstanding = true;
			ticket_button.setText("Releasing ticket...");
			logMsg("Releasing ticket in " + ticketRegion);
			new ReleaseTask().execute(ticketRegion.x, ticketRegion.y, 0L,
					System.currentTimeMillis());
		} else if (requestOutstanding) {
			logMsg("Wait for previous action to complete.");
		} else if (!ticketHeld) {
			logMsg("You are not holding a ticket!");
		}
	}

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