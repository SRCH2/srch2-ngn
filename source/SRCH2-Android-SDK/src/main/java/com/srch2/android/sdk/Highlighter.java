package com.srch2.android.sdk;

/**
 * Defines how the search results of a search on a particular index can be formatted to reflect the search input.
 * The SRCH2 search server can generate output that reflects the current search input by inserting leading and
 * trailing tags around the substring of text that matches for any field data value against the search input.
 * For instance, if the search input is 'Joh' and this results in record with a field whose data is 'John Smith',
 * if highlighting was enabled for this field (see {@link Field#enableHighlighting()}), then the SRCH2 search
 * server will produce in addition to the the original record a highlighted snippet. It will be of the form
 * 'PRETAG|matching-text|POSTTAG' and is available for each highlighted field for each record in the search results
 * (see {@link com.srch2.android.sdk.SearchResultsListener#onNewSearchResults(int, String, java.util.HashMap)} for
 * more details). In this case, if matching against exact text and applying a bold HTML tag, the output would be
 * '&lt;b&gt;Jo&lt;/b&gt;n Smith' or (visually, '<b>Jo</b>hn Smith').
 * <br><br>
 * Highlighting is applied per index and the same for all highlighted fields in that index's schema. It can be
 * differentiated between exact and fuzzy text matches. <b>By default</b> if a field has highlighting enabled,
 * it will be formatted according to the HTML bold tag. This can be used with {@link android.text.Html#fromHtml(String)}
 * and a {@link android.widget.TextView} such as <code>mTextView.setText(Html.fromHtml(mHighlightedString))</code>.
 * <br><br>
 * Highlighting does not have to be based on HTML tags: it can be whatever is useful.
 * <br><br>
 * Highlighting pre and post script for exact and fuzzy text matching <b>must never contain</b> single quotes. All
 * special characters should also be escaped.
 * <br><br>
 * Since highlighting will typically consist of colorization and making bold or italic, the class
 * {@link com.srch2.android.sdk.Highlighter.SimpleHighlighter} and its methods can be used to generate the proper
 * HTML tags.
 * Use {@link #createHighlighter()} to do so. Otherwise, custom pre and post script tags can be set by calling
 * {@link #createCustomHighlighter(String, String, String, String)} and passing in the corresponding tags.
 * <br><br>
 * A <code>Highlighter</code> object should only be returned from the method {@link Indexable#getHighlighter()}
 * or {@link SQLiteIndexable#getHighlighter()}
 * when setting the highlighting formatting for highlighted fields.
 */
public class Highlighter {

    static final String HIGHLIGHTING_BOLD_PRE_SCRIPT_TAG = "<b>";
    static final String HIGHLIGHTING_BOLD_POST_SCRIPT_TAG = "</b>";
    static final String HIGHLIGHTING_ITALIC_PRE_SCRIPT_TAG = "<i>";
    static final String HIGHLIGHTING_ITALIC_POST_SCRIPT_TAG = "</i>";

    String highlightFuzzyPreTag;
    String highlightFuzzyPostTag;
    String highlightExactPreTag;
    String highlightExactPostTag;

    boolean highlightingHasBeenCustomSet = false;
    boolean highlightBoldExact = true;
    boolean highlightBoldFuzzy = true;
    boolean highlightItalicExact = false;
    boolean highlightItalicFuzzy = false;
    String highlightColorExact = null;
    String highlightColorFuzzy = null;

    Highlighter() {
        highlightingHasBeenCustomSet = false;
    }
    /**
     * A simple highlighter with methods for building the highlighting format of exact and fuzzy text
     * matches: exact or fuzzy text matches can be made bold, italicized, or colorized. For example, using the
     * method {@link com.srch2.android.sdk.Highlighter.SimpleHighlighter#formatExactTextMatches(boolean, boolean)} with
     * the values <b>true</b> and <b>false</b> will set the output of the SRCH2 highlighter to make all exact text
     * matches bold: if the search input were 'Jo' matching a textual value of 'John' , the output of the highlighting
     * function will be '<b>Jo</b>n' (technically it would be
     * '&lt;b&gt;Jo&lt;/b&gt;n' without the auto-formatting of the javadoc's html).
     * <br><br>
     * The <code>SimpleHighlighter</code> uses HTML tags to format the output so it is recommended to use the
     * highlighting output with the
     * method {@link android.text.Html#fromHtml(String)} inside the {@link android.widget.TextView#setText(CharSequence)}
     * method so search results can be made to dynamically reflect the search input as it is entered by the user.
     * <br><br>
     * Use the methods:
     * <br>{@link com.srch2.android.sdk.Highlighter.SimpleHighlighter#formatExactTextMatches(boolean, boolean)}
     * <br>{@link com.srch2.android.sdk.Highlighter.SimpleHighlighter#formatExactTextMatches(boolean, boolean, String)}
     * <br>{@link com.srch2.android.sdk.Highlighter.SimpleHighlighter#formatFuzzyTextMatches(boolean, boolean)}
     * <br>{@link com.srch2.android.sdk.Highlighter.SimpleHighlighter#formatFuzzyTextMatches(boolean, boolean, String)}
     * <br>to format the highlighting.
     */
    public static class SimpleHighlighter extends Highlighter {
        SimpleHighlighter() {
            super();
        }

        /**
         * Sets the formatting for exact text matches for being bold, italicized and colorized. That is, if the
         * search input is 'Jo' and the highlighted field data that matches for this search is 'John Smith'
         * then the output, if bold is set true and italics false, would be '<b>Jo</b>hn Smith' (or
         * '&lt;b&gt;Jo&lt;/b&gt;n Smith' literally). Being bold and italicized is not mutually exclusive.
         * <br><br>
         * Note this formatting occurs by HTML tagging, thus {@link android.text.Html#fromHtml(String)} must be
         * used when using the highlighted output in a <code>TextView</code>.
         * <br><br>
         * This method will throw exceptions if the value of <code>htmlHexColorValue</code> is not a six
         * character (or seven if including the #), valid hex code for a color.
         * @param bold whether to make the exact matching text bold
         * @param italic whether to make the exact matching text italicized
         * @param htmlHexColorValue the html color code to use of the form '#FFFFFF' or 'FFFFFF'
         * @return this highlighter
         */
        public SimpleHighlighter formatExactTextMatches(boolean bold, boolean italic, String htmlHexColorValue) {
            highlightBoldExact = bold;
            highlightItalicExact = italic;
            htmlHexColorValue = checkHexColorValue("exact", htmlHexColorValue);
            highlightColorExact = htmlHexColorValue;
            return this;
        }

        /**
         * Sets the formatting for exact text matches for being bold and italicized. That is, if the
         * search input is 'Jo' and the highlighted field data that matches for this search is 'John Smith'
         * then the output, if bold is set true and italics false, would be '<b>Jo</b>hn Smith' (or
         * '&lt;b&gt;Jo&lt;/b&gt;n Smith' literally). Being bold and italicized is not mutually exclusive.
         * @param bold whether to make the exact matching text bold
         * @param italic whether to make the exact matching text italicized
         * @return this highlighter
         */
        public SimpleHighlighter formatExactTextMatches(boolean bold, boolean italic) {
            highlightBoldExact = bold;
            highlightItalicExact = italic;
            return this;
        }

        /**
         * Sets the formatting for fuzzy text matches for being bold, italicized and colorized. That is, if the
         * search input is 'Jon' and the highlighted field data that matches for this search is 'John Smith'
         * then the output, if bold is set true and italics false, would be '<b>Jo</b>hn Smith' (or
         * '&lt;b&gt;Jo&lt;/b&gt;n Smith' literally). Being bold and italicized is not mutually exclusive.
         * <br><br>
         * Note this formatting occurs by HTML tagging, thus {@link android.text.Html#fromHtml(String)} must be
         * used when using the highlighted output in a <code>TextView</code>.
         * <br><br>
         * This method will throw exceptions if the value of <code>htmlHexColorValue</code> is not a six
         * character (or seven if including the #), valid hex code for a color.
         * @param bold whether to make the exact matching text bold
         * @param italic whether to make the exact matching text italicized
         * @param htmlHexColorValue the html color code to use of the form '#FFFFFF' or 'FFFFFF'
         * @return this highlighter
         */
        public SimpleHighlighter formatFuzzyTextMatches(boolean bold, boolean italic, String htmlHexColorValue) {
            highlightBoldFuzzy = bold;
            highlightItalicFuzzy = italic;
            htmlHexColorValue = checkHexColorValue("fuzzy", htmlHexColorValue);
            highlightColorFuzzy= htmlHexColorValue;
            return this;
        }

        /**
         * Sets the formatting for exact fuzzy matches for being bold and italicized. That is, if the
         * search input is 'Jon' and the highlighted field data that matches for this search is 'John Smith'
         * then the output, if bold is set true and italics false, would be '<b>Jo</b>hn Smith' (or
         * '&lt;b&gt;Jo&lt;/b&gt;n Smith' literally). Being bold and italicized is not mutually exclusive.
         * <br><br>
         * @param bold whether to make the exact matching text bold
         * @param italic whether to make the exact matching text italicized
         * @return this highlighter
         */
        public SimpleHighlighter formatFuzzyTextMatches(boolean bold, boolean italic) {
            highlightBoldFuzzy = bold;
            highlightItalicFuzzy = italic;
            return this;
        }
    }

    protected Highlighter(String exactPreTag, String exactPostTag, String fuzzyPreTag, String fuzzyPostTag) {
        checkHighlightingTags("fuzzyPreTag", fuzzyPreTag);
        checkHighlightingTags("fuzzyPostTag", fuzzyPostTag);
        checkHighlightingTags("exactPreTag", exactPreTag);
        checkHighlightingTags("exactPostTag", exactPostTag);
        highlightingHasBeenCustomSet = true;
        highlightFuzzyPreTag = fuzzyPreTag;
        highlightFuzzyPostTag = fuzzyPostTag;
        highlightExactPreTag = exactPreTag;
        highlightExactPostTag = exactPostTag;
    }

    /**
     * Returns a custom highlighter where the values for the pre and post script of highlighted exact and fuzzy text
     * matches is explicitly defined.  For example, <code>exactPreTag</code>
     * <code>exactPostTag</code> will be the leading and trailing tags of the substring of the search input keyword that
     * matches exactly: if the search input were 'Joh' matching a textual value of 'John' and the values of
     * <code>exactPreTag</code> and <code>exactPostTag</code> supplied to this method where &lt;b&gt; and &lt;/b&gt;,
     * respectively, the output of the highlighting function would be '<b>Joh</b>n' (technically it would be
     * '&lt;b&gt;Joh&lt;/b&gt;n' without the auto-formatting of the javadoc's html).
     * <br><br>
     * Thus by setting these values and enabling highlighting on a searchable field will cause search results to
     * be automatically formatted indicating the match between the current user's search input and every highlighted
     * field in a search result. In the example above, by using the value returned in the search result with the
     * method {@link android.text.Html#fromHtml(String)} inside the {@link android.widget.TextView#setText(CharSequence)}
     * method, search results can be made to dynamically reflect the search input as it is entered by the user. In fact,
     * any tags can be used and then reformatted during post-processing to fully customize the appearance of the highlighted
     * fields of the search results.
     * <br><br>
     * <b>If this method is not called when some schema's fields have {@link Field#enableHighlighting()} set</b>, the default
     * behavior of the highlighter will be to bold matching text.
     * <br><br>
     * This method will throw exceptions if any of the arguments are null or empty strings.
     * @param fuzzyPreTag specifies the tag value to be prefixed to a fuzzy keyword match
     * @param fuzzyPostTag specifies the tag value to be suffixed to a fuzzy keyword match
     * @param exactPreTag specifies the tag value to be prefixed to an exact keyword match
     * @param exactPostTag specifies the tag value to be suffixed to an exact keyword match
     */
    public static Highlighter createCustomHighlighter(String exactPreTag, String exactPostTag, String fuzzyPreTag, String fuzzyPostTag) {
        return new Highlighter(exactPreTag, exactPostTag, fuzzyPreTag, fuzzyPostTag);
    }

    /**
     * Returns a simple highlighter with methods for building the highlighting format of exact and fuzzy text
     * matches: exact or fuzzy text matches can be made bold, italicized, or colorized. For example, using the
     * method {@link com.srch2.android.sdk.Highlighter.SimpleHighlighter#formatExactTextMatches(boolean, boolean)} with
     * the values <b>true</b> and <b>false</b> will set the output of the SRCH2 highlighter to make all exact text
     * matches bold: if the search input were 'Jo' matching a textual value of 'John' , the output of the highlighting
     * function will be '<b>Jo</b>n' (technically it would be
     * '&lt;b&gt;Jo&lt;/b&gt;n' without the auto-formatting of the javadoc's html).
     * <br><br>
     * The <code>SimpleHighlighter</code> uses HTML tags to format the output so it is recommended to use the
     * highlighting output with the
     * method {@link android.text.Html#fromHtml(String)} inside the {@link android.widget.TextView#setText(CharSequence)}
     * method so search results can be made to dynamically reflect the search input as it is entered by the user.
     * <br><br>
     * Use the methods:
     * <br>{@link com.srch2.android.sdk.Highlighter.SimpleHighlighter#formatExactTextMatches(boolean, boolean)}
     * <br>{@link com.srch2.android.sdk.Highlighter.SimpleHighlighter#formatExactTextMatches(boolean, boolean, String)}
     * <br>{@link com.srch2.android.sdk.Highlighter.SimpleHighlighter#formatFuzzyTextMatches(boolean, boolean)}
     * <br>{@link com.srch2.android.sdk.Highlighter.SimpleHighlighter#formatFuzzyTextMatches(boolean, boolean, String)}
     * <br>to format the highlighting.
     * @return a simple highlighter with methods setting the highlight formatting
     */
    public static SimpleHighlighter createHighlighter() {
        return new SimpleHighlighter();
    }

    /** Call this method in {@link com.srch2.android.sdk.IndexDescription#IndexDescription(Indexable)} to
     * set the highlight fields before calling {@link IndexDescription#setQueryProperties()}.
     */
    void configureHighlightingForIndexDescription() {
        String exactPreTag = "";
        String exactPostTag = "";
        String fuzzyPreTag = "";
        String fuzzyPostTag = "";
        if (!highlightingHasBeenCustomSet) {
            if (highlightBoldExact) {
                exactPreTag = HIGHLIGHTING_BOLD_PRE_SCRIPT_TAG;
                exactPostTag = HIGHLIGHTING_BOLD_POST_SCRIPT_TAG;
            }

            if (highlightBoldFuzzy) {
                fuzzyPreTag = HIGHLIGHTING_BOLD_PRE_SCRIPT_TAG;
                fuzzyPostTag = HIGHLIGHTING_BOLD_POST_SCRIPT_TAG;
            }

            if (highlightItalicExact) {
                exactPreTag += HIGHLIGHTING_ITALIC_PRE_SCRIPT_TAG;
                exactPostTag += HIGHLIGHTING_ITALIC_POST_SCRIPT_TAG;
            }

            if (highlightItalicFuzzy) {
                fuzzyPreTag = HIGHLIGHTING_ITALIC_PRE_SCRIPT_TAG;
                fuzzyPostTag = HIGHLIGHTING_ITALIC_POST_SCRIPT_TAG;
            }

            if (highlightColorExact != null) {
                exactPreTag += "<font color=\"#" + highlightColorExact + "\">";
                exactPostTag += "</font>";
            }

            if (highlightColorFuzzy != null) {
                fuzzyPreTag += "<font color=\"#" + highlightColorFuzzy + "\">";
                fuzzyPostTag += "</font>";
            }
        } else {
            exactPreTag = highlightExactPreTag;
            exactPostTag = highlightExactPostTag;
            fuzzyPreTag = highlightFuzzyPreTag;
            fuzzyPostTag = highlightFuzzyPostTag;
        }

        // safety check --> default to bold if any problems
        if (exactPreTag == null || exactPreTag.length() < 1) {
            exactPreTag = HIGHLIGHTING_BOLD_PRE_SCRIPT_TAG;
        }
        if (exactPostTag == null || exactPostTag.length() < 1) {
            exactPreTag = HIGHLIGHTING_BOLD_POST_SCRIPT_TAG;
        }
        if (fuzzyPreTag == null || fuzzyPreTag.length() < 1) {
            fuzzyPreTag = HIGHLIGHTING_BOLD_PRE_SCRIPT_TAG;
        }
        if (fuzzyPostTag == null || fuzzyPostTag.length() < 1) {
            fuzzyPostTag = HIGHLIGHTING_BOLD_POST_SCRIPT_TAG;
        }

        highlightExactPreTag = exactPreTag;
        highlightExactPostTag = exactPostTag;
        highlightFuzzyPreTag = fuzzyPreTag;
        highlightFuzzyPostTag = fuzzyPostTag;
    }

    protected void checkHighlightingTags(String nameOfVariable, String tagToCheck) {
        if (tagToCheck == null) {
            throw new NullPointerException("Highlighter tag \'" + nameOfVariable + "\' cannot be null");
        } else if (tagToCheck.length() < 1) {
            throw new IllegalArgumentException("Highlighter tag \'" + nameOfVariable + "\' must be non-empty string");
        } else if (tagToCheck.contains("\'")) {
            throw new IllegalArgumentException("Highlighter tag \'" + nameOfVariable + "\' cannot contain \' (single quotes)");
        }
    }

    protected String checkHexColorValue(String whichTextToMatch, String hexColorValue) {
        if (hexColorValue != null && hexColorValue.length() > 1 && hexColorValue.charAt(0) == '#') {
            // strips the leading # if present
            hexColorValue = hexColorValue.substring(1, hexColorValue.length());
        }

        if (hexColorValue == null) {
            throw new NullPointerException("hexColorValue submitted for " + whichTextToMatch + " cannot be null");
        } else if (hexColorValue.length() != 6) {
            throw new IllegalArgumentException("hexColorValue: " + hexColorValue + " submitted for " + whichTextToMatch
                    + " must be a 6 character html color code");
        } else {
            try {
                Integer.parseInt(hexColorValue, 16); // verifies is valid hex
            } catch (NumberFormatException nan) {
                throw new IllegalArgumentException("hexColorValue: " + hexColorValue + " submitted for "
                        + whichTextToMatch + " must be a valid hex color representation");
            }
        }
        return hexColorValue;
    }

}
