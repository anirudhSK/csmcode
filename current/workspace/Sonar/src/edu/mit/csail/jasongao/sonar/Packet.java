package edu.mit.csail.jasongao.sonar;

import java.io.Serializable;

import android.location.Location;

public class Packet implements Serializable {
	private static final long serialVersionUID = 201L;

	public long timestamp;
	public long senderId;
	public double longitude;
	public double latitude;

	/** Construct Packet with values */
	public Packet(long id) {
		timestamp = System.currentTimeMillis();
		senderId = id;
		latitude = -1;
		longitude = -1;
	}

	/**
	 * @param senderLoc
	 *            the sender's Location
	 */
	public void setSenderLoc(Location loc) {
		if (loc != null) {
			longitude = loc.getLongitude();
			latitude = loc.getLatitude();
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString() {
		return "Packet [timestamp=" + timestamp + ", senderId=" + senderId
				+ ", longitude=" + longitude + ", latitude=" + latitude + "]";
	}
}
