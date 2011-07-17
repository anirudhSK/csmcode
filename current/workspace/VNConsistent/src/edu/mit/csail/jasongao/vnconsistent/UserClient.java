package edu.mit.csail.jasongao.vnconsistent;

import java.util.HashMap;
import java.util.Map;
import java.util.Random;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

/**
 * Sample benchmarking application
 * 
 * Spams the local region with reads and writes, expecting the VN-C layer to
 * handle them. This layer should not be aware of VN-C / only needs to send out
 * packets, and expect responses from some server in the region
 * 
 * @author jasongao
 * 
 */
public class UserClient extends Thread {
	private final static String TAG = "UserClient";

	Mux mux;
	Handler myHandler;
	Random rand;

	private long myRx, myRy;
	private long maxRx, maxRy;
	private long id;

	private double readVsWriteDistribution = 0.9;

	private boolean ticketHeld = false; // are we currently holding a ticket?
	private RegionKey ticketHeldRegion;
	private boolean requestOutstanding = false;
	private long successCount = 0;
	private long failureCount = 0;
	private long opCount = 0;

	private UserOp lastRequest = null; // last write requested
	private UserOp currentRequest = null;
	private long currentRequestTimestamp;

	private boolean benchmarkOn = false;

	final static private long benchmarkTimeoutCheckPeriod = 5000L;
	final static private long benchmarkIterationDelay = 1000L;

	/**
	 * Checks to see if currentRequest has been outstanding too long. If so,
	 * makes it as timed out and makes a new request
	 */
	Runnable benchmarkRequestTimeoutCheckR = new Runnable() {
		@Override
		public void run() {
			if (currentRequest == lastRequest) { // comparing pointers is fine
				failureCount++;
				logMsg(String.format("UserClient request timed out. (%d,%d)=?",
						currentRequest.targetRx, currentRequest.targetRy,
						currentRequest.value));
				myHandler.removeCallbacks(benchmarkIterationR);

				// reset flags
				requestOutstanding = false;

				myHandler.post(benchmarkIterationR);
			}

			lastRequest = currentRequest;

			// Update display
			updateDisplay();

			myHandler.postDelayed(benchmarkRequestTimeoutCheckR,
					benchmarkTimeoutCheckPeriod);
		}
	};

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
					requestRead(dstRx, dstRy);
				} else {
					// make a write-involving operation (request or release)
					if (!ticketHeld) {
						requestDecrement(dstRx, dstRy);
					} else {
						requestIncrement();
					}
				}
			}
		}
	};

	/** Constructor */
	public UserClient(Mux m, long id, long myRx_, long myRy_, long maxRx_,
			long maxRy_) {
		this.id = id;
		this.mux = m;
		this.myRx = myRx_;
		this.myRy = myRy_;
		this.maxRx = maxRx_;
		this.maxRy = maxRy_;
		this.rand = new Random();
	}

	/** Take action on message sent to my handler. */
	private void processMessage(Message msg) {
		switch (msg.what) {
		case Mux.REGION_CHANGE:
			RegionKey newRegion = (RegionKey) msg.obj;
			this.myRx = newRegion.x;
			this.myRy = newRegion.y;
			logMsg("UserClient received REGION_CHANGE message.");
			break;
		}
	}

	/** Send data to the StatusActivity for display */
	private void updateDisplay() {
		Map<String, Long> userData = new HashMap<String, Long>();
		userData.put("op", opCount);
		userData.put("success", successCount);
		userData.put("failure", failureCount);
		userData.put("ticket_held", ticketHeld ? 1L : 0L);
		userData.put("request_oustanding", requestOutstanding ? 1L : 0L);
		mux.myHandler.obtainMessage(Mux.CLIENT_STATUS_CHANGE, userData)
				.sendToTarget();
	}

	/** Send a CSMOp to the Mux to broadcast the request */
	private void sendUserOp(UserOp uop) {
		opCount++;
		uop.request = true;
		currentRequestTimestamp = System.currentTimeMillis();
		mux.myHandler.obtainMessage(Mux.APP_SEND, uop).sendToTarget();
	}

	/** Take action on reply from server and make another request */
	public synchronized void handleReply(UserOp uop) {
		if (uop.requesterId != id)
			return;

		long latency = System.currentTimeMillis() - currentRequestTimestamp;

		if (uop.type == UserOp.DECREMENT) {
			if (uop.success) {
				successCount++;
				requestOutstanding = false;
				ticketHeld = true;
				logMsg(String
						.format("UserClient decrement on (%d,%d) from (%d,%d) succeeded, value=%d,latency=%d",
								uop.targetRx, uop.targetRy, uop.requesterRx,
								uop.requesterRy, uop.value, latency));
			} else {
				failureCount++;
				logMsg(String
						.format("UserClient decrement on (%d,%d) failed, value=?,latency=%d",
								uop.targetRx, uop.targetRy, latency));
			}
		} else if (uop.type == UserOp.INCREMENT) {
			if (uop.success) {
				successCount++;
				requestOutstanding = false;
				ticketHeld = false;
				logMsg(String
						.format("UserClient increment on (%d,%d) from (%d,%d) succeeded, value=%d,latency=%d",
								uop.targetRx, uop.targetRy, uop.requesterRx,
								uop.requesterRy, uop.value, latency));
			} else {
				failureCount++;
				logMsg(String
						.format("UserClient increment on (%d,%d) failed, value=?,latency=%d",
								uop.targetRx, uop.targetRy, latency));
			}
		} else if (uop.type == UserOp.READ) {
			if (uop.success) {
				successCount++;
				logMsg(String
						.format("UserClient read on (%d,%d) from (%d,%d)succeeded, value=%d,latency=%d",
								uop.targetRx, uop.targetRy, uop.requesterRx,
								uop.requesterRy, uop.value, latency));
			} else {
				failureCount++;
				logMsg(String
						.format("UserClient read on (%d,%d) failed, value=?,latency=%d",
								uop.targetRx, uop.targetRy, latency));
			}
		}

		// Update display
		updateDisplay();

		// Set up next iteration
		if (benchmarkOn) {
			myHandler.postDelayed(benchmarkIterationR, benchmarkIterationDelay);
		}
	}

	public synchronized boolean isTicketHeld() {
		return this.ticketHeld;
	}

	public synchronized boolean isRequestOutstanding() {
		return this.requestOutstanding;
	}

	/** Read a parking spot */
	public synchronized void requestRead(long rx, long ry) {
		if (rx > maxRx || ry > maxRy) {
			logMsg("Invalid region for ticket read.");
			return;
		} else if (requestOutstanding) {
			logMsg("Wait for previous action to complete.");
		} else {
			RegionKey readSpotRegion = new RegionKey(rx, ry);
			logMsg("Reading spot in " + readSpotRegion);
			UserOp userRequest = new UserOp(id, UserOp.READ, myRx, myRy, rx, ry);
			currentRequest = userRequest;
			sendUserOp(userRequest);
		}
	}

	/** Request a parking spot */
	public synchronized void requestDecrement(long rx, long ry) {
		if (rx > maxRx || ry > maxRy) {
			logMsg("Invalid region for ticket request.");
			return;
		}
		if (ticketHeld) {
			logMsg("Already holding a ticket!");
		} else {
			if (!ticketHeld && !requestOutstanding) {
				requestOutstanding = true;
				ticketHeldRegion = new RegionKey(rx, ry);
				logMsg("Requesting spot in " + ticketHeldRegion);
				UserOp userRequest = new UserOp(id, UserOp.DECREMENT, myRx,
						myRy, rx, ry);
				currentRequest = userRequest;
				sendUserOp(userRequest);
			} else if (requestOutstanding) {
				logMsg("Wait for previous action to complete.");
			} else if (ticketHeld) {
				logMsg("You are already holding a ticket!" + ticketHeldRegion);
			}
		}
	}

	/** Request a parking spot */
	public synchronized void requestDecrementInCurrentRegion() {
		long rx = this.myRx;
		long ry = this.myRy;

		if (rx > maxRx || ry > maxRy) {
			logMsg("Invalid region for ticket request.");
			return;
		}

		if (ticketHeld) {
			logMsg("Already holding a ticket!");
		} else {
			if (!ticketHeld && !requestOutstanding) {
				requestOutstanding = true;
				ticketHeldRegion = new RegionKey(rx, ry);
				logMsg("Requesting spot in " + ticketHeldRegion);
				UserOp userRequest = new UserOp(id, UserOp.DECREMENT, myRx,
						myRy, rx, ry);
				currentRequest = userRequest;
				sendUserOp(userRequest);
			} else if (requestOutstanding) {
				logMsg("Wait for previous action to complete.");
			} else if (ticketHeld) {
				logMsg("You are already holding a ticket!" + ticketHeldRegion);
			}
		}
	}

	/** Release a parking spot */
	public synchronized void requestIncrement() {
		if (ticketHeld && !requestOutstanding) {
			requestOutstanding = true;
			logMsg("Releasing ticket in " + ticketHeldRegion);

			UserOp userRequest = new UserOp(id, UserOp.INCREMENT, myRx, myRy,
					ticketHeldRegion.x, ticketHeldRegion.y);
			currentRequest = userRequest;
			sendUserOp(userRequest);
		} else if (requestOutstanding) {
			logMsg("Wait for previous action to complete.");
		} else if (!ticketHeld) {
			logMsg("You are not holding a ticket!");
		}
	}

	/** Return if the benchmark is currently enabled */
	public synchronized boolean isBenchmarkOn() {
		return this.benchmarkOn;
	}

	/** Start the benchmark iteration loop */
	public synchronized void startBenchmark() {
		this.benchmarkOn = true;
		myHandler.post(benchmarkIterationR);
		myHandler.postDelayed(benchmarkRequestTimeoutCheckR,
				benchmarkTimeoutCheckPeriod);
	}

	/** Stop the benchmark iteration loop */
	public synchronized void stopBenchmark() {
		this.benchmarkOn = false;
		myHandler.removeCallbacks(benchmarkRequestTimeoutCheckR);
		myHandler.removeCallbacks(benchmarkIterationR);
	}

	public synchronized void setReadWriteDistribution(double dist) {
		readVsWriteDistribution = dist;
	}

	/** Stuff to do right before we enter the run loop. */
	private void onStart() {
		logMsg("UserClient started");
		// TODO
	}

	/** Stuff to do right BEFORE exiting the run loop. */
	private void onRequestStop() {
		// TODO
		logMsg("UserClient stopped");
	}

	/**
	 * Stuff to do right AFTER exiting the run loop (when it has finished
	 * processing remaining tasks / messages / runnables)
	 **/
	private void onStop() {
		// TODO
	}

	/** Exit after all queued tasks are done. */
	public synchronized void requestStop() {
		myHandler.post(new Runnable() {
			@Override
			public void run() {
				Log.i(TAG, "Stop request encountered.");
				onRequestStop();
				Looper.myLooper().quit();
			}
		});
	}

	/** Thread's run method */
	@Override
	public void run() {
		// Prepare looper and handler on current thread
		Looper.prepare();
		myHandler = new Handler() {
			@Override
			public void handleMessage(Message msg) {
				processMessage(msg);
			}
		};
		onStart(); // Start up
		Looper.loop();
		onStop();
		Log.i(TAG, "Thread exiting");
	}

	private void logMsg(String line) {
		line = String.format("%d: %s", System.currentTimeMillis(), line);
		mux.myHandler.obtainMessage(Mux.LOG, line).sendToTarget();
		Log.i(TAG, line);
	}
}
