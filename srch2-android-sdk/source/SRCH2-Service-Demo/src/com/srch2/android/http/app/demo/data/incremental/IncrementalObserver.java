/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
