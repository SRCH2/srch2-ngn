package com.srch2.android.http.app.demo;

import java.util.ArrayList;

import android.app.Activity;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

public class InstantSearchEditText extends EditText implements TextWatcher {

	private static final String TAG = "InstantSearchInputField";
	
	/** Interface for observing for when this text field receives new input from user. */
	public interface SearchInputEnteredObserver {
		public void onNewSearchInput(String newSearchText);
		public void onNewSearchInputIsBlank();
	}
	
	private ArrayList<SearchInputEnteredObserver> searchInputObservers;
    
	/** This is used to filter input for uniqueness. */
	private StringBuilder inputBuffer = new StringBuilder(); 
	
	private Drawable clearButton = null;
	private Drawable logoButton = null;
	
	public InstantSearchEditText(Context context, AttributeSet attrs, int defStyle) { super(context, attrs, defStyle); setupConstructors(context); }
	public InstantSearchEditText(Context context, AttributeSet attrs) { super(context, attrs); setupConstructors(context); }
	public InstantSearchEditText(Context context) { super(context); setupConstructors(context); }
	
	/** Add as many observers as needed. */
	public void addSearchInputObserver(SearchInputEnteredObserver observer) { 
		if (searchInputObservers == null) { searchInputObservers = new ArrayList<SearchInputEnteredObserver>(); }
		if (observer != null) { searchInputObservers.add(observer); }
	}
	
	/** Setups this object for each SDK called constructor: note, currently the activity hosting this text field must implement SearchInputEnteredObserver. */
    private void setupConstructors(Context context) {
    	searchInputObservers = new ArrayList<SearchInputEnteredObserver>();
    	addSearchInputObserver((SearchInputEnteredObserver) context);
    	
    	inputBuffer.setLength(0);
		clearButton = context.getResources().getDrawable(R.drawable.clear_input_icon);
		logoButton = context.getResources().getDrawable(R.drawable.ic_launcher);
	
		logoButton.setBounds(0, 0, clearButton.getIntrinsicWidth(), clearButton.getIntrinsicHeight());
		clearButton.setBounds(0, 0, clearButton.getIntrinsicWidth(), clearButton.getIntrinsicHeight());
		
		setOnTouchListener(new OnTouchListener() {
			@Override
			public boolean onTouch(View v, MotionEvent event) {
				if (getCompoundDrawables()[2] == null) {
					return false;
				}
				if (event.getAction() != MotionEvent.ACTION_UP) {
					return false;
				}
				if (event.getX() > getWidth() - getPaddingRight()
						- clearButton.getIntrinsicWidth()) {
					setText("");
					setCompoundDrawables(logoButton, null, null, null);
				}
				return false;
			}
		});
		setCompoundDrawables(logoButton, null, null, null);
		addTextChangedListener(this);
    }
	
	@Override 
	public void beforeTextChanged(CharSequence s, int start, int count, int after) { 
		inputBuffer.setLength(0);
		inputBuffer.append(s);	
	}

	@Override
	public void afterTextChanged(Editable s) {
		final String userInput = s.toString().trim();
		
		if (inputBuffer.toString().trim().equals(userInput)) {
			return;
		}
		
        if (s.length() > 0) {
            setCompoundDrawables(logoButton, null, clearButton, null);
            for (SearchInputEnteredObserver observer : searchInputObservers) {
    			observer.onNewSearchInput(userInput);
    		}
        } else {
        	if (!hasFocus()) {
        		requestFocus();
        	}
            setCompoundDrawables(logoButton, null, null, null);
            for (SearchInputEnteredObserver observer : searchInputObservers) {
    			observer.onNewSearchInputIsBlank();
    		}
        }
	}
	
    @Override public void onTextChanged(CharSequence s, int start, int before, int count) {   }
    
    /** Returns whether the input field currently has text input such that the input is not empty space and greater than zero in length. */
    public boolean hasRealInput() {
    	return getText().toString().trim().length() > 0;
    }
    
    /** Makes it so that every UI widget that is not InstantSearchEditText will cause the soft keyboard to close when touched by the user. 
	 * Originally the root view of the layout should be passed in. */
	public static void setOnHideSoftInputListenerForAllUIViews(View view) {
	    if(!(view instanceof InstantSearchEditText)) {
	        view.setOnTouchListener(new OnTouchListener() {
				@Override
				public boolean onTouch(View v, MotionEvent event) {
					hideSoftKeyboard(v);
					return false;
				}
			});
	    }
	    if (view instanceof ViewGroup) {
	        for (int i = 0; i < ((ViewGroup) view).getChildCount(); i++) {
	            View innerView = ((ViewGroup) view).getChildAt(i);
	            setOnHideSoftInputListenerForAllUIViews(innerView);
	        }
	    }
	}
	
	/** Utility method for opening soft keyboard depending on if the search input text field is empty: call, for instance, in onResume. */
	public static boolean checkIfSearchInputShouldOpenSoftKeyboard(final Context context, final InstantSearchEditText searchInputTextField) {
		if (!searchInputTextField.hasRealInput()) {
			searchInputTextField.requestFocus();
			searchInputTextField.postDelayed(new Runnable() {
                  @Override
                  public void run() {
              		try {
                        InputMethodManager keyboard = (InputMethodManager)
                        context.getSystemService(Context.INPUT_METHOD_SERVICE);
                        keyboard.showSoftInput(searchInputTextField, 0);
            		} catch (NullPointerException npe) {
            			npe.printStackTrace();
            		}
                  }
              },200);
		}
		return true;
	}
	
	/** Utility method that will open the soft keyboard. */
	public static void forceOpenSoftKeyboard(final EditText editText) {
		editText.requestFocus();
		editText.postDelayed(new Runnable() {
			@Override
			public void run() {
				try {
					InputMethodManager keyboard = (InputMethodManager) editText.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
					keyboard.showSoftInput(editText, 0);
				} catch (NullPointerException npe) {
					npe.printStackTrace();
				}
			}
		}, 200);
	}
	
	/** Utility method to hide the soft keyboard if visible: caller must supply a valid view. */
	public static void hideSoftKeyboard(final View view) {
		view.postDelayed(new Runnable() {
			@Override
			public void run() {
				try {
				    InputMethodManager inputMethodManager = (InputMethodManager) view.getContext().getSystemService(Activity.INPUT_METHOD_SERVICE);
				    inputMethodManager.hideSoftInputFromWindow(view.getWindowToken(), 0);
				} catch (NullPointerException npe) {
					npe.printStackTrace();
				}
			}
		}, 200);
	}
}