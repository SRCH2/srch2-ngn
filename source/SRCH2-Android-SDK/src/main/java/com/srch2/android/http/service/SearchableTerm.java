package com.srch2.android.http.service;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

abstract class Term {
    public abstract String toString();

    abstract public Term AND(Term rightTerm);

    abstract public Term AND_NOT(Term rightTerm);

    abstract public Term OR(Term rightTerm);

    abstract Term NOT();
}

/**
 * The SearchableTerm encapsulates the search string with all the advanced search
 * options. The default setting is copied from the override method from
 * <code>Indexable</code>. User can call the setting functions to override
 * the default settings.
 */
final public class SearchableTerm extends Term {

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
     * <p/>
     * The prefix, fuzziness, boostValue is set using the default value present
     * in <code>IndexDescription</code>
     *
     * @param keywords the search key word[s].
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
     * Overrides the prefix matching setting.
     *
     * @param isPrefixMatching disable or enable prefix match on the current query
     * @return this
     */
    public SearchableTerm setIsPrefixMatching(boolean isPrefixMatching) {
        this.isPrefixMatching = isPrefixMatching;
        return this;
    }

    /**
     * Overrides the default Index fuzziness similarity setting
     *
     * @param similarity the fuzziness similarity
     * @return this
     */
    public SearchableTerm enableFuzzyMatching(float similarity) {
        checkSimilarity(similarity);
        this.fuzzySimilarity = similarity;
        return this;
    }

    /**
     * Enable the fuzzy matching. The fuzziness similarity setting
     * will get from the {@link Indexable#getFuzzinessSimilarityThreshold()}
     *
     * @return this
     */
     public SearchableTerm enableFuzzyMatching() {
        this.fuzzySimilarity = 1f;
        return this;
    }

    /**
     * Disables the fuzzy matching
     *
     * @return this
     */
    public SearchableTerm disableFuzzyMatching() {
        this.fuzzySimilarity = -1f;
        return this;
    }

    /**
     * To boost the importance of a given term.The boost value must be a
     * positive integer, and its default value is 1.
     *
     * @param boostValue the importance of the current term.
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
     * It specifies a field to search on. Otherwise the query is searched on all the
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
     * Create a composite term by <code>AND</code> operator
     *
     * @param rightTerm the right operand
     * @return a new CompositeTerm as a result of <code>this AND rightTerm</code>
     */
    public CompositeTerm AND(Term rightTerm) {
        return new CompositeTerm(BooleanOperator.AND, this, rightTerm);
    }

    /**
     * Create a composite term by <code>AND</code> operator, inverse match the rightTerm
     *
     * @param rightTerm the right operand
     * @return a new CompositeTerm as a result of <code>this AND NOT rightTerm</code>
     */
    public CompositeTerm AND_NOT(Term rightTerm) {
        return new CompositeTerm(BooleanOperator.AND, this, rightTerm.NOT());
    }

    /**
     * Create a composite term by <code>OR</code> operator
     *
     * @param rightTerm the right operand
     * @return a new CompositeTerm as a result of <code>this OR rightTerm</code>
     */
    public CompositeTerm OR(Term rightTerm) {
        return new CompositeTerm(BooleanOperator.OR, this, rightTerm);
    }
    /**
     * Create a composite term by <code>NOT</code> operator
     *
     * @return a new CompositeTerm as a result of <code>NOT this</code>
     */
    CompositeTerm NOT() {
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
        if (fuzzySimilarity != null) {
            restStr.append('~');
            if (fuzzySimilarity < 1 && fuzzySimilarity > 0) {
                restStr.append(fuzzySimilarity);
            }
        }
        return restStr.toString();
    }

    enum BooleanOperator {
        AND, OR, NOT,
    }

    /**
     * The CompositeTerm that enable the boolean selection on the query terms.
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
         * Create a composite term by <code>AND</code> operator
         *
         * @param rightTerm the right operand
         * @return a new CompositeTerm as a result of <code>this AND rightTerm</code>
         */
        @Override
        public Term AND(Term rightTerm) {
            return new CompositeTerm(BooleanOperator.AND, this, rightTerm);
        }

        /**
         * Create a composite term by <code>AND NOT</code> operator. The rightTerm will be inverse matching
         *
         * @param rightTerm the right operand
         * @return a new CompositeTerm as a result of <code>this AND NOT rightTerm</code>
         */
        @Override
        public Term AND_NOT(Term rightTerm) {
            return new CompositeTerm(BooleanOperator.AND, this, rightTerm.NOT());
        }
        /**
         * Create a composite term by <code>OR</code> operator
         *
         * @param rightTerm the right operand
         * @return a new CompositeTerm as a result of <code>this OR rightTerm</code>
         */
        @Override
        public Term OR(Term rightTerm) {
            return new CompositeTerm(BooleanOperator.OR, this, rightTerm);
        }

        /**
         * Create a composite term by <code>NOT</code> operator
         *
         * @return a new CompositeTerm as a result of <code>NOT this</code>
         */
        @Override
        Term NOT() {
            return new CompositeTerm(BooleanOperator.NOT, this, null);
        }
    }
}
