package com.srch2.android.sdk;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

abstract class Term {
    public abstract String toString();

    abstract public Term AND(Term rightTerm);

    abstract public Term NOT(Term rightTerm);

    abstract public Term OR(Term rightTerm);

    abstract Term UNARY_NOT();
}

/**
 * Encapsulates the search string with all the advanced search
 * options. Enables configuring the search in way such as overriding
 * the default field boost values and fuzziness matching threshold value.
 * <br><br>
 * This class is used in combination with the {@link com.srch2.android.sdk.Query} class
 * to perform advanced searches.
 */
final public class SearchableTerm extends Term {

    static final float FLAG_USE_DEFAULT_FUZZY_SIMILARITY = -1f;
    static final float FLAG_DISABLE_FUZZY_MATCHING = -2f;
    /**
     * could be multiple words "George Lucas"
     */
    private final String keywords;

    private Boolean isPrefixMatching = null;
    private Integer boostValue = null;
    private Float fuzzySimilarity = null;
    private String filterField = null;

    /**
     * Creates the term into keyword. The string passed into this function is
     * treated as a single search term. For example, if "George Lucas" is passed, the two words will be
     * treated as one word.
     * @param keywords the search input
     */
    public SearchableTerm(String keywords) {
        checkString(keywords, "search keywords is not valid");
        try {
            this.keywords = URLEncoder.encode(keywords, "UTF-8");
        } catch (UnsupportedEncodingException ignore) {
            throw new IllegalArgumentException("Unbelievable！ UTF-8 encoding is not supported! A" +
                    "re you in android platform?");
        }
    }

    private static void checkString(String str, String msg) {
        if (str == null || str.length() < 0) {
            throw new IllegalArgumentException(msg);
        }
    }

    private static void checkSimilarity(float similarity) {
        if (similarity < 0 || similarity > 1) {
            throw new IllegalArgumentException(
                    "similarity should be in the [0,1] range");
        }
    }

    /**
     * Overrides the prefix matching setting. This is equivalent to having all search
     * results be matched against the search input concatenated with the {@code |*} regex
     * operation. Thus if the search input is "d" any word beginning with the letter d will
     * be returned as a search result.
     * <br><br>
     * By default this is enabled.
     * @param isPrefixMatching disable or enable prefix match on the current query
     * @return this
     */
    public SearchableTerm setIsPrefixMatching(boolean isPrefixMatching) {
        this.isPrefixMatching = isPrefixMatching;
        return this;
    }

    /**
     * Overrides the fuzziness similarity threshold value. This determines the number
     * of wildcard substitutions that can occur in the character set of the search input.
     * A {@code similarity} value of 0 is the same forcing the search result text to exactly
     * match the search input; a value of 1 will permit all characters to be substituted for
     * any other character.
     * <br><br>
     * By default it takes the value of
     * {@link com.srch2.android.sdk.Indexable#getFuzzinessSimilarityThreshold()} which by
     * default
     * approximately allows 1 in 3 characters to be substituted as wildcards.
     * @param similarity the fuzziness similarity threshold ratio
     * @return this
     */
    public SearchableTerm enableFuzzyMatching(float similarity) {
        checkSimilarity(similarity);
        this.fuzzySimilarity = similarity;
        return this;
    }

    /**
     * Overrides the fuzziness similarity threshold value. This determines the number
     * of wildcard substitutions that can occur in the character set of the search input.
     * This method will takes the value set from
     * {@link com.srch2.android.sdk.Indexable#getFuzzinessSimilarityThreshold()} which by
     * default
     * approximately allows 1 in 3 characters to be substituted as wildcards.
     * @return this
     */
     public SearchableTerm enableFuzzyMatching() {
        this.fuzzySimilarity = FLAG_USE_DEFAULT_FUZZY_SIMILARITY;
        return this;
    }

    /**
     * Disables the fuzzy matching meaning all text in index records will have to
     * exactly match the search input.
     * @return this
     */
    public SearchableTerm disableFuzzyMatching() {
        this.fuzzySimilarity = FLAG_DISABLE_FUZZY_MATCHING;
        return this;
    }

    /**
     * Boosts the importance of a given term. The boost value must be a
     * positive integer, and its default value is 1.
     * @param boostValue the importance of the current term
     * @return this
     */
    public SearchableTerm setBoostValue(int boostValue) {
        if (boostValue < 0) {
            throw new IllegalArgumentException(
                    "The boost value must be a positive integer");
        }
        this.boostValue = boostValue;
        return this;
    }

    /**
     * Specifies a field to search on. Otherwise the query is searched on all the
     * searchable fields.
     *
     * @param fieldName name of the searchable field
     * @return this
     */
    public SearchableTerm searchSpecificField(String fieldName) {
        checkString(fieldName, "The fieldName is invalid");
        this.filterField = fieldName;
        return this;
    }

    /**
     * Creates a composite term by <code>AND</code> operator.
     *
     * @param rightTerm the right operand
     * @return a new CompositeTerm as a result of <code>this AND rightTerm</code>
     */
    public CompositeTerm AND(Term rightTerm) {
        return new CompositeTerm(BooleanOperator.AND, this, rightTerm);
    }

    /**
     * Creates a composite term by <code>AND</code> operator, the logical negation of the matching right term.
     *
     * @param rightTerm the right operand
     * @return a new CompositeTerm as a result of <code>this AND NOT rightTerm</code>
     */
    public CompositeTerm NOT(Term rightTerm) {
        return new CompositeTerm(BooleanOperator.AND, this, rightTerm.UNARY_NOT());
    }

    /**
     * Creates a composite term by <code>OR</code> operator.
     *
     * @param rightTerm the right operand
     * @return a new CompositeTerm as a result of <code>this OR rightTerm</code>
     */
    public CompositeTerm OR(Term rightTerm) {
        return new CompositeTerm(BooleanOperator.OR, this, rightTerm);
    }
    /**
     * Creates a composite term by <code>NOT</code> operator.
     *
     * @return a new CompositeTerm as a result of <code>NOT this</code>.
     */
    CompositeTerm UNARY_NOT() {
        return new CompositeTerm(BooleanOperator.NOT, this, null);
    }

    public String toString() {
        /**
         * the order of modifiers must always be prefix, boost, and then fuzzy
         */
        StringBuilder restStr = new StringBuilder();
        if (filterField != null) {
            restStr.append(filterField).append(':');
        }

        if (keywords.contains("+") || keywords.contains(" ")) {
            restStr.append('"').append(keywords).append('"');
        } else {
            restStr.append(keywords);
        }
        if (isPrefixMatching != null && isPrefixMatching) {
            restStr.append('*');
        }
        if (boostValue != null) {
            restStr.append('^').append(boostValue);
        }
        if (fuzzySimilarity != null ) {
            if (fuzzySimilarity != FLAG_DISABLE_FUZZY_MATCHING) {
                restStr.append('~');
                if (fuzzySimilarity != FLAG_USE_DEFAULT_FUZZY_SIMILARITY) {
                    restStr.append(fuzzySimilarity);
                }
            }
        }
        return restStr.toString();
    }

    enum BooleanOperator {
        AND, OR, NOT,
    }

    /**
     * Enables boolean selection on the query terms.
     */
    final public static class CompositeTerm extends Term {

        private final BooleanOperator operator;
        private final Term left;
        private final Term right;

        CompositeTerm(BooleanOperator operator, Term left, Term right) {
            this.operator = operator;
            this.left = left;
            this.right = right;
        }

        public String toString() {
            StringBuilder sb = new StringBuilder();
            switch (operator) {
                case NOT:
                    sb.append(operator.name()).append(' ');
                    if (left.getClass() == CompositeTerm.class) {
                        sb.append('(').append(left.toString()).append(')');
                    } else {
                        sb.append(left.toString());
                    }
                    break;
                case AND:
                case OR:
                    if (left.getClass() == CompositeTerm.class) {
                        sb.append('(').append(left.toString()).append(')');
                    } else {
                        sb.append(left.toString());
                    }
                    sb.append(' ').append(operator.name()).append(' ');
                    if (left.getClass() == CompositeTerm.class) {
                        sb.append('(').append(right.toString()).append(')');
                    } else {
                        sb.append(right.toString());
                    }
                    break;
            }

            return sb.toString();
        }

        /**
         * Create a composite term by <code>AND</code> operator.
         *
         * @param rightTerm the right operand
         * @return a new CompositeTerm as a result of <code>this AND rightTerm</code>
         */
        @Override
        public Term AND(Term rightTerm) {
            return new CompositeTerm(BooleanOperator.AND, this, rightTerm);
        }

        /**
         * Create a composite term by <code>AND NOT</code> operator. The rightTerm will be inverse matching.
         *
         * @param rightTerm the right operand
         * @return a new CompositeTerm as a result of <code>this AND NOT rightTerm</code>
         */
        @Override
        public Term NOT(Term rightTerm) {
            return new CompositeTerm(BooleanOperator.AND, this, rightTerm.UNARY_NOT());
        }
        /**
         * Create a composite term by <code>OR</code> operator.
         *
         * @param rightTerm the right operand
         * @return a new CompositeTerm as a result of <code>this OR rightTerm</code>
         */
        @Override
        public Term OR(Term rightTerm) {
            return new CompositeTerm(BooleanOperator.OR, this, rightTerm);
        }

        /**
         * Create a composite term by <code>NOT</code> operator.
         *
         * @return a new CompositeTerm as a result of <code>NOT this</code>
         */
        @Override
        Term UNARY_NOT() {
            return new CompositeTerm(BooleanOperator.NOT, this, null);
        }
    }
}
