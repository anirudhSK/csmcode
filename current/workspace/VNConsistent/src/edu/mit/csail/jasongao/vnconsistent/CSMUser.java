package edu.mit.csail.jasongao.vnconsistent;

public interface CSMUser {
	CSMOp handleCSMRequest(CSMLayer.Block b, CSMOp c);

	void handleCSMReply(CSMOp c);
}
