package com.srch2.android.demo.helloworld;

import android.app.Activity;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

public class InstantSearchEditText extends EditText implements TextWatcher {

    private static final String TAG = "InstantSearchInputField";

    /**
     * Interface for observing for when the input field of this <code>Edit Text</code> receives new input from
     * user.
     */
    public interface SearchInputEnteredObserver {

        /**
         * Called when the input field changes and is not empty.
         *
         * @param newSearchText the new text input
         */
        public void onNewSearchInput(String newSearchText);

        /**
         * Called when the input field changes and is empty.
         */
        public void onNewSearchInputIsBlank();
    }

    /**
     * The observer of changes to the input field of this <code>EditText</code>.
     */
    private SearchInputEnteredObserver mSearchInputObserver;

    /**
     * Used to filter input for uniqueness.
     */
    private StringBuilder mInputBuffer = new StringBuilder();

    /**
     * Taken from the standard set of Android icons: originally named ic_menu_clear_action.
     * Has a transparent background.
     */
    private Drawable mClearButton = null;

    /**
     * Called when this <code>InstantSearchEditText</code> is inflated in code.
     *
     * @param context the context of the activity inflating this widget
     */
    public InstantSearchEditText(Context context) {
        super(context);
        setupConstructors(context);
    }

    /**
     * Called when this <code>InstantSearchEditText</code> is inflated by XML.
     *
     * @param context the context of the activity inflating this widget
     * @param attrs the attribute set styling this widget
     */
    public InstantSearchEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
        setupConstructors(context);
    }

    /**
     * Called when this <code>InstantSearchEditText</code> is inflated by XML with an assigned style.
     *
     * @param context the context of the activity inflating this widget
     * @param attrs the attribute set styling this widget
     * @param defStyle the style identifier
     */
    public InstantSearchEditText(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        setupConstructors(context);
    }

    /**
     * Sets up this widget for each constructor: note, currently the
     * activity hosting this text field must implement
     * <code>SearchInputEnteredObserver</code>.
     *
     * @param context the context of the activity inflating this widget
     */
    private void setupConstructors(Context context) {
        mSearchInputObserver = (SearchInputEnteredObserver) context;

        mInputBuffer.setLength(0);
        mClearButton = context.getResources().getDrawable(
                R.drawable.instant_search_edit_text_clear_input_action);
        mClearButton.setBounds(0, 0, mClearButton.getIntrinsicWidth(),
                mClearButton.getIntrinsicHeight());

        // Clears the input field when the right compound drawable is clicked
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
                        - mClearButton.getIntrinsicWidth()) {
                    setText("");
                    setCompoundDrawables(null, null, null, null);
                }
                return false;
            }
        });
        setCompoundDrawables(null, null, null, null);
        addTextChangedListener(this);
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count,
                                  int after) {
        // Prepare the buffer
        mInputBuffer.setLength(0);
        mInputBuffer.append(s);
    }

    @Override
    public void afterTextChanged(Editable s) {
        final String userInput = s.toString().trim();

        // If the buffer text matches the current input text, do nothing
        if (mInputBuffer.toString().trim().equals(userInput)) {
            return;
        }

        // If the input text is different from the buffer, redisplay the clear input icon
        // if the input text is not empty; propagate the text of the input text to the observer
        if (s.length() > 0) {
            setCompoundDrawables(null, null, mClearButton, null);
            mSearchInputObserver.onNewSearchInput(userInput);
        } else {
            if (!hasFocus()) {
                requestFocus();
            }
            setCompoundDrawables(null, null, null, null);
            mSearchInputObserver.onNewSearchInputIsBlank();
        }
    }

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) {
        // Do nothing
    }

    /**
     * Utility method for checking the state of the current text.
     *
     * @return <b>true</b> if calling getText() wil not return an empty string;
     *         <b>false</b> otherwise
     */
    public boolean hasRealInput() {
        return getText().toString().trim().length() > 0;
    }

    /**
     * Makes it so that every UI widget that is not InstantSearchEditText will
     * cause the soft keyboard to close when touched by the user.
     *
     * @param view the root view of the layout
     */
    public static void setOnHideSoftInputListenerForAllUIViews(View view) {
        if (!(view instanceof InstantSearchEditText)) {
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

    /**
     * Utility method for opening soft keyboard depending on if the search input
     * text field is empty: call, for instance, in an activity's onResume.
     *
     * @param context the activity context
     * @param searchInputTextField the reference to this widget
     * @return <bold>true</bold> if the soft keyboard will be opened, <bold>false</bold> otherwise
     */
    public static boolean checkIfSearchInputShouldOpenSoftKeyboard(
            final Context context,
            final InstantSearchEditText searchInputTextField) {
        if (!searchInputTextField.hasRealInput()) {
            searchInputTextField.requestFocus();
            searchInputTextField.postDelayed(new Runnable() {
                @Override
                public void run() {
                    try {
                        InputMethodManager keyboard = (InputMethodManager) context
                                .getSystemService(Context.INPUT_METHOD_SERVICE);
                        keyboard.showSoftInput(searchInputTextField, 0);
                    } catch (NullPointerException npe) {
                        npe.printStackTrace();
                    }
                }
            }, 200);
        }
        return true;
    }

    /**
     * Utility method that will open the soft keyboard.
     * @param editText the reference to this widget
     */
    public static void forceOpenSoftKeyboard(final EditText editText) {
        editText.requestFocus();
        editText.postDelayed(new Runnable() {
            @Override
            public void run() {
                try {
                    InputMethodManager keyboard = (InputMethodManager) editText
                            .getContext().getSystemService(
                                    Context.INPUT_METHOD_SERVICE);
                    keyboard.showSoftInput(editText, 0);
                } catch (NullPointerException npe) {
                    npe.printStackTrace();
                }
            }
        }, 200);
    }

    /**
     * Utility method to hide the soft keyboard if visible: caller must supply a
     * valid view.
     * @param view a reference to a view in the currently visible layout
     */
    public static void hideSoftKeyboard(final View view) {
        view.postDelayed(new Runnable() {
            @Override
            public void run() {
                try {
                    InputMethodManager inputMethodManager = (InputMethodManager) view
                            .getContext().getSystemService(
                                    Activity.INPUT_METHOD_SERVICE);
                    inputMethodManager.hideSoftInputFromWindow(
                            view.getWindowToken(), 0);
                } catch (NullPointerException npe) {
                    npe.printStackTrace();
                }
            }
        }, 200);
    }
}