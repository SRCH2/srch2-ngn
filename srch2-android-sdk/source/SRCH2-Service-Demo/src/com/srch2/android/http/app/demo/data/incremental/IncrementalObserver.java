package com.srch2.android.http.app.demo.data.incremental;

import android.content.Context;
import android.database.ContentObserver;
import android.os.Handler;

public abstract class IncrementalObserver extends ContentObserver {
	
	protected abstract void registerObserver(final Context context);
	protected abstract void unregisterObserver(final Context context);
	
	protected boolean isRegistered = false;
	
	/** Duration to delay incremental update requests by. */
	private static int POST_DELAY_UPDATE_REQUEST_TIME = 3000;
	
	/** Maximum trigger count before forcing incremental update request. */
	private static int MAX_ON_CHANGE_TRIGGERED_COUNT = 20;

	/** Current count of triggers that client data has changed. */
	private int onChangeTriggerCount = 0;
	 
	private Handler handler;
	
	private IncrementalUpdateObserver updateObserver;
	
	public IncrementalObserver(IncrementalUpdateObserver contentChangedObserver) {
		super(null);
		updateObserver = contentChangedObserver;
		handler = new Handler();
	}
	
	
	public void startObserving(final Context context) {
		registerObserver(context);
	}
	
	public void stopObserving(final Context context) {
		unregisterObserver(context);
	}
	
	
	
	/** Used by incremental observers using implementation extending ContentObserver. */
	@Override
	public boolean deliverSelfNotifications() {
		return false;
	}
	
	/** Used by incremental observers actually using implementation extending ContentObserver. */
	@Override
	public void onChange(boolean selfChange) {
		super.onChange(selfChange);	
		processOnChange();
	}
	
	/** Used by incremental observers not using implementation extending ContentObserver. */
	public void onChange() { 
		processOnChange();
	}
	
	/**
	 * Handles whether incremental update request should be processed now, or delayed if many
	 * changes are happening at once. Each time this is called, any pending incremental update
	 * requests are cleared from the handler queue and a new one is scheduled to occur by the 
	 * specified delay of <code>POST_DELAY_UPDATE_REQUEST_TIME</code>; will force an incremental 
	 * update request if the number of times the observer has been triggered that there is a change 
	 * exceeds <code>MAX_ON_CHANGE_TRIGGERED_COUNT</code>.
	 */
	private void processOnChange() {

		++onChangeTriggerCount;
		handler.removeCallbacks(postDelayRequestUpdateRunnable);
		if (onChangeTriggerCount < MAX_ON_CHANGE_TRIGGERED_COUNT) {
			handler.postDelayed(postDelayRequestUpdateRunnable, POST_DELAY_UPDATE_REQUEST_TIME);
		} else {
			onChangeTriggerCount = 0;
			triggerIncrementalUpdate();
		}
	}
	
	private Runnable postDelayRequestUpdateRunnable = new Runnable() {
		@Override
		public void run() {
			triggerIncrementalUpdate();
		}
	};
	
	protected void triggerIncrementalUpdate() {
		updateObserver.onIncrementalDifferenceDetected();
	}
}