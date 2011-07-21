package edu.mit.csail.jasongao.vnconsistent;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import edu.mit.csail.jasongao.vnconsistent.Globals;

public class Mux extends Thread {
	private final static String TAG = "Mux";

	private final static long maxRx = Globals.MAX_X_REGIONS;
	private final static long maxRy = Globals.MAX_Y_REGIONS;

	private long nodeId;

	Handler myHandler;
	Handler activityHandler;

	// Mux message types
	protected final static int LOG_NODISPLAY = 27;
	protected final static int LOG = 3;
	protected final static int PACKET_RECV = 4;
	protected final static int PACKET_SEND = 5;
	protected final static int CSM_SEND = 19;
	protected final static int VNC_SEND = 18;
	protected final static int APP_SEND = 17;
	protected final static int VNC_STATUS_CHANGE = 6;
	protected final static int REGION_CHANGE = 7;
	protected final static int CLIENT_STATUS_CHANGE = 8;

	// Components TODO make private
	private NetworkThread netThread;
	public VNCDaemon vncDaemon;

	public UserClient userClient;

	/** Mux constructor */
	public Mux(long id, Handler a) {
		nodeId = id;
		activityHandler = a;
	}

	/** Log message to device display and to Android log. */
	public void logMsg(String line) {
		line = String.format("%d: %s", System.currentTimeMillis(), line);
		activityHandler.obtainMessage(Mux.LOG, line).sendToTarget();
		Log.i(TAG, line);
	}

	/** Take action on message sent to my handler. */
	private void processMessage(Message msg) {
		switch (msg.what) {
		case Mux.PACKET_RECV:
			Packet vnp = (Packet) msg.obj;
			if (vnp == null) { // due to some error in networkthread loop
				logMsg("Received null packet...");
				return;
			}
			switch (vnp.type) {
			case Packet.VNC_MSG:
				logMsg("Network -> VNC");
				vncDaemon.myHandler.sendMessage(Message.obtain(msg));
				break;
			case Packet.CSM_MSG:
				logMsg("Network -> CSM");
				if (vncDaemon.mState == VNCDaemon.LEADER
						&& vncDaemon.csm != null)
					vncDaemon.csm.handleCSMOp(vnp.csm_op);
				break;
			case Packet.APP_MSG:
				if (vnp.user_op.requesterRx != vncDaemon.myRegion.x
						|| vnp.user_op.requesterRy != vncDaemon.myRegion.y) {
					logMsg("Network -> User? of other region, ignoring");
					return; // ignore requests not originating from our region
				}
				if (vnp.user_op.request) {
					if (vncDaemon.mState == VNCDaemon.LEADER) {
						logMsg("Network -> UserServer");
						if (vncDaemon.csm != null
								&& vncDaemon.csm.userServer != null) {
							vncDaemon.csm.userServer
									.handleClientRequest(vnp.user_op);
						}
					}
					break;
				} else {
					logMsg("Network -> UserClient");
					if (userClient != null) {
						userClient.handleReply(vnp.user_op);
					}
				}
				break;
			} // end switch(vnp.type)

			break;
		case Mux.PACKET_SEND:
			logMsg("VNC -> Network"); // Only VNCDaemon uses PACKET_SEND
			this.netThread.sendPacket((Packet) msg.obj);
			break;
		case CSM_SEND:
			CSMOp op = (CSMOp) msg.obj;
			// outgoing to network if leader. otherwise, mute and buffer
			Packet p = new Packet(-1, -1, Packet.CSM_MSG, -1, op.srcRegion,
					op.dstRegion);
			p.csm_op = op;
			if (vncDaemon.mState == VNCDaemon.LEADER) {
				logMsg("CSM -> Network");
				this.netThread.sendPacket(p);
				vncDaemon.csm.handleCSMOp(op);
			} else if (vncDaemon.mState == VNCDaemon.NONLEADER) {
				logMsg("CSM -> Network (muted, buffered)");
				// TODO csmOpBuffer.add(op)
			}

			break;
		case APP_SEND:
			UserOp uop = (UserOp) msg.obj;
			Packet p1 = new Packet(-1, -1, Packet.APP_MSG, -1, new RegionKey(
					-1, -1), new RegionKey(-1, -1));
			p1.user_op = uop;

			// if it's a request from our UserClient, send to net + loopback
			if (uop.request) {
				logMsg("UserClient -> Network");
				this.netThread.sendPacket(p1);
				myHandler.obtainMessage(Mux.PACKET_RECV, p1).sendToTarget();
			} else { // if it's a reply from our UserServer, buffer if not lead
				if (vncDaemon.mState == VNCDaemon.LEADER) {
					logMsg("UserServer -> Network");
					this.netThread.sendPacket(p1);
					myHandler.obtainMessage(Mux.PACKET_RECV, p1).sendToTarget();
				} else if (vncDaemon.mState == VNCDaemon.NONLEADER) {
					logMsg("UserServer -> Network (muted, buffered)");
					// TODO userOpBuffer.add(uop)
				}
			}
			break;
		case Mux.LOG:
			activityHandler.sendMessage(Message.obtain(msg));
			break;
		case Mux.LOG_NODISPLAY:
			activityHandler.sendMessage(Message.obtain(msg));
			break;
		case Mux.VNC_STATUS_CHANGE:
			activityHandler.sendMessage(Message.obtain(msg));
			break;
		case Mux.REGION_CHANGE:
			activityHandler.sendMessage(Message.obtain(msg));
			userClient.myHandler.sendMessage(Message.obtain(msg));
			break;
		case Mux.CLIENT_STATUS_CHANGE: // TODO remove?
			activityHandler.sendMessage(Message.obtain(msg));
			break;
		}
	}

	/** Stuff to do right before we enter the run loop. */
	private void onStart() {
		Log.d(TAG, "Mux started");

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

		if (nodeId < 0) {
			nodeId = 1000 * netThread.getLocalAddress().getAddress()[2]
					+ netThread.getLocalAddress().getAddress()[3];
			// nodeId = netThread.getLocalAddress().getAddress()[3]; // lastoct
		}
		
		// Start outside active region
		long initRx = -1;
		long initRy = -1;

		vncDaemon = new VNCDaemon(this, nodeId, initRx, initRy, maxRx, maxRy);
		vncDaemon.start();

		userClient = new UserClient(this, nodeId, initRx, initRx, maxRx, maxRy);
		userClient.start();
	}

	/** Stuff to do right BEFORE exiting the run loop. */
	private void onRequestStop() {
		userClient.requestStop();
		while (userClient.isAlive()) {
			Log.d(TAG, "Waiting for UserClient to stop...");
			try {
				sleep(1000L);
			} catch (Exception e) {
				Log.d(TAG, "Exception: " + e.getLocalizedMessage());
			}
		}

		vncDaemon.requestStop();
		while (vncDaemon.isAlive()) {
			Log.d(TAG, "Waiting for VNCDaemon to stop...");
			try {
				sleep(1000L);
			} catch (Exception e) {
				Log.d(TAG, "Exception: " + e.getLocalizedMessage());
			}
		}
	}

	/**
	 * Stuff to do right AFTER exiting the run loop (when it has finished
	 * processing remaining tasks / messages / runnables)
	 **/
	private void onStop() {

		netThread.closeSocket();
		while (netThread.isAlive()) {
			Log.d(TAG, "Waiting for NetworkThread to stop...");
			try {
				sleep(1000L);
			} catch (Exception e) {
				Log.d(TAG, "Exception: " + e.getLocalizedMessage());
			}
		}
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
}
