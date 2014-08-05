package com.srch2.android.http.app.demo;

import android.content.Context;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;

import com.srch2.android.http.app.demo.data.ViewableResult;

public class TouchController implements OnTouchListener, OnClickListener {

	private static final String TAG = "TouchController";
	
	private int lastSwipeViewId = -10;
	private float downX;
	private float maximumSwipeThreshold;
	private float minimumThreshold;
	private float verticalThreshold;
	private float swipeDisplacement;
	
	public TouchController(Context context) {
		maximumSwipeThreshold = DefaultUiResources.convertDpToPixels(context, 100);
		minimumThreshold = DefaultUiResources.convertDpToPixels(context, 10);
		verticalThreshold = DefaultUiResources.convertDpToPixels(context, 160);
	}
	
	private void invalidateForSwipe(View content, float currentSwipeDisplacement) {
		content.setTranslationX(currentSwipeDisplacement);
		swipeDisplacement = currentSwipeDisplacement;
	}
	
	private void restoreOriginalState(View rowContent) {
		downX = 0;
		swipeDisplacement = 0;
		rowContent.setTranslationX(0);
	}
	
	private boolean isSwipeEventTriggered(View v) {
		if ((maximumSwipeThreshold - Math.abs(swipeDisplacement)) < minimumThreshold) {
			if (swipeDisplacement < 0) {
				ViewableResult vr = (ViewableResult) v.getTag(R.id.tag_viewable_result);
				if (vr != null) {
					vr.onSwipeLeft(v.getContext(), vr);
				}
			} else {
				ViewableResult vr = (ViewableResult) v.getTag(R.id.tag_viewable_result);
				if (vr != null) {
					vr.onSwipeRight(v.getContext(), vr);
				}
			}
			return true;
		} else {
			return false;
		}
	}
	
	private void isClickEventTriggered(View v) {
		if (Math.abs(swipeDisplacement) < minimumThreshold) {
			ViewableResult vr = (ViewableResult) v.getTag(R.id.tag_viewable_result);
			if (vr != null) {
				vr.onRowClick(v.getContext(), vr);
			}
		}
	}
	
	@Override
	public boolean onTouch(View v, MotionEvent event) {
		final int actionEvent = event.getAction();
		final ViewableResult thisViewableResult = (ViewableResult) v.getTag(R.id.tag_viewable_result);
		if (thisViewableResult.isSwippable) {
			if (v.getId() != lastSwipeViewId) {
				restoreOriginalState((View) v.getTag(R.id.tag_row_content));
				lastSwipeViewId = v.getId();
			}
			
			try {
				Thread.currentThread().sleep(5);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
			
			switch (actionEvent) {
				case MotionEvent.ACTION_DOWN:
					downX = event.getX();
					v.getParent().requestDisallowInterceptTouchEvent(true);
					return true;
				case MotionEvent.ACTION_MOVE:
					final float x = event.getX();
					final float y = event.getY();
					if (y < ((v.getBottom() - v.getY()) + verticalThreshold) && y >( v.getTop() - v.getY() - verticalThreshold)) {
						float currentDisplacement = x - downX;
						if (Math.abs(currentDisplacement) > maximumSwipeThreshold) {
							final int sign = currentDisplacement > 0 ? 1 : -1;
							currentDisplacement = maximumSwipeThreshold * sign;
						}
						invalidateForSwipe((View) v.getTag(R.id.tag_row_content), currentDisplacement);
					} else {
						restoreOriginalState((View) v.getTag(R.id.tag_row_content));
					}
//					vrfObserver.onViewableResultHasFocus();
					return true;
				case MotionEvent.ACTION_UP:
					if (!isSwipeEventTriggered(v)) {
						isClickEventTriggered(v);
					}
					restoreOriginalState((View) v.getTag(R.id.tag_row_content));
				//	vrfObserver.onViewableResultHasFocus();
					return true;
			//	case MotionEvent.ACTION_CANCEL:
				//	restoreOriginalState((View) v.getTag(R.id.tag_row_content));
				//	vrfObserver.onViewableResultHasFocus();
				//	return true;
			}
		} else {
			switch (actionEvent) {
				case MotionEvent.ACTION_DOWN:
					return true;
				case MotionEvent.ACTION_UP:
					ViewableResult vr = ((ViewableResult) v.getTag(R.id.tag_viewable_result));
					vr.onRowClick(v.getContext(), vr);
				
				//	vrfObserver.onViewableResultHasFocus();
					return true;
			}
		}
		return false;
	}

	@Override
	public void onClick(View v) {
		
		ViewableResult vr = (ViewableResult) v.getTag(R.id.tag_viewable_result);
		if (vr != null) {
			vr.onRowButtonClick(v.getContext(), v.getId(), vr);
		}
	}
}