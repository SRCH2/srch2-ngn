package com.srch2.android.sdk;

import com.srch2.android.sdk.Filter.BooleanOperation;

import java.util.ArrayList;

/**
 * Formulates an advanced search. Users of the SRCH2 Android SDK can invoke this class
 * to create advanced searches that enable operations above and beyond the basic default
 * search performed.
 */
final public class Query {

    private static final String[] BOX_TAG = {"lblat=", "lblong=", "rtlat=", "rtlong="};
    private static final String[] CIRCLE_TAG = {"clat=", "clong=", "radius="};

    private final String proximitySentence;
    private final Term term;

    private final ArrayList<Filter> filters = new ArrayList<Filter>();
    private final ArrayList<FacetField> facetFields = new ArrayList<FacetField>();
    private final ArrayList<Double> geoPosition = new ArrayList<Double>();
    private BooleanOperation filterConnector = BooleanOperation.AND;
    private SearchType searchType;
    private String sortSentence;
    private String orderbySentence;
    private Integer pagingStart;
    private Integer pagingRows;


    /**
     * Creates a basic query initialized with a {@link SearchableTerm} or
     * {@link SearchableTerm.CompositeTerm}.
     *
     * @param term {@link SearchableTerm} or
     * {@link SearchableTerm.CompositeTerm}
     */
    public Query(Term term) {
        checkIsNotNull(term, "Input term should not be null");
        this.term = term;
        this.proximitySentence = null;
    }

    /**
     * Creates a query configured to do a proximity search. The SRCH2 search server
     * supports finding words that are within a
     * specified proximity.
     * <br><br>
     * For example, the following query searches for records with the keywords
     * "saving" and "ryan" within a single word distance:
     * <br>
     * &nbsp;&nbsp;&nbsp;&nbsp;{@code Query("saving", "ryan", 1) }
     * <br><br>
     * <b>Note</b> that a proximity search does not support edit-distance-based fuzzy
     * match: i.e. typos are not recognized in the term's keywords.
     * <br><br>
     * This method will throw exceptions if the term arguments passed have values that
     * are null or have a length less than one; or if the value of {@code distance}
     * is less than one.
     */
    public Query(String term1, String term2, int distance) {
        checkNoEmptyString(term1, "invalid query keyword");
        checkNoEmptyString(term2, "invalid query keyword");
        if (distance < 1) {
            throw new IllegalArgumentException("distance should >= 1");
        }
        this.proximitySentence = "\"" + term1 + " " + term2 + "\"~"
                + Integer.toString(distance);
        this.term = null;
    }

    /**
     * Creates a query configured to do a geo-search within an rectangular region.
     * <br><br>
     * Equivalent to calling {@link #insideRectangle(double, double, double, double)}, but the
     * search results will contain all results within the region without matching against
     * any keywords.
     * <br><br>
     * If the search also needs to be filtered by keyword, create
     * a normal search query and call the {@link #insideRectangle(double, double, double, double)}
     * method.
     *
     * @param leftBottomLatitude the left bottom point's latitude value
     * @param leftBottomLongitude the left bottom point's longitude value
     * @param rightTopLatitude  the right top point's latitude value
     * @param rightTopLongitude the right top point's longitude value
     */
    public Query(double leftBottomLatitude,
                 double leftBottomLongitude,
                 double rightTopLatitude,
                 double rightTopLongitude) {
        this.term = null;
        this.proximitySentence = null;
        this.geoPosition.clear();
        this.geoPosition.add(leftBottomLatitude);
        this.geoPosition.add(leftBottomLongitude);
        this.geoPosition.add(rightTopLatitude);
        this.geoPosition.add(rightTopLongitude);
    }

    /**
     * Creates a query configured to do a geo-search within a radius around a center point.
     * <br><br>
     * Equivalent to calling {@link #insideCircle(double, double, double)}, but the
     * search results will contain all results within the region without matching against
     * any keywords.
     * <br><br>
     * If the search also needs to be filtered by keyword, create
     * a normal search query and call the {@link #insideCircle(double, double, double)}
     * method.
     *
     * @param centerLatitude the center point's latitude value
     * @param centerLongitude the center point's longitude value
     * @param radius the radius value of the search area
     */
    public Query(double centerLatitude,
                 double centerLongitude, double radius) {
        this.term = null;
        this.proximitySentence = null;
        this.geoPosition.clear();
        this.geoPosition.add(centerLatitude);
        this.geoPosition.add(centerLongitude);
        this.geoPosition.add(radius);
    }

    private static void checkIsNotNull(Object term, String errMsg) {
        if (term == null) {
            throw new IllegalArgumentException(errMsg);
        }
    }

    private static void checkNoEmptyString(String str, String errMsg) {
        if (str == null || str.trim().length() == 0) {
            throw new IllegalArgumentException(errMsg);
        }
    }

    /**
     * Creates a query configured to do a filtered search. To construct this kind of query, specify a filter
     * that will restrict the set of records returned by using the conditionality of
     * {@code fields[fieldName].value == equalToValue}.
     * <br><br>
     * This method will throw exceptions if the term arguments passed have values that
     * are null or have a length less than one; or if the value of {@code distance}
     * is less than one.
     * @param fieldName the specific field to evaluate
     * @param equalToValue the value to equate the {@code fieldName} to
     */
    public Query filterByFieldEqualsTo(String fieldName, String equalToValue) {
        checkNoEmptyString(fieldName, "fieldName is invalid");
        checkNoEmptyString(equalToValue, "equalToValue is invalid");
        filters.add(Filter.getFilterEqualTo(fieldName, equalToValue));
        setSearchType(SearchType.getAll);
        return this;
    }

    /**
     * Matches records whose field value starts from a specified lower bound.
     * @param fieldName the field to filter on
     * @param startValue the value (inclusive) to match the filter against
     * @return this
     */
    public Query filterByFieldStartsFrom(String fieldName, String startValue) {
        checkNoEmptyString(fieldName, "fieldName is invalid");
        checkNoEmptyString(startValue, "startValue is invalid");
        filters.add(Filter.getFilterStartFrom(fieldName, startValue));
        setSearchType(SearchType.getAll);
        return this;
    }

    /**
     * Matches records whose field value ends at a specified upper bound.
     * @param fieldName  the field to filter on
     * @param endValue  the value (inclusive) to match the filter against
     * @return this
     */
    public Query filterByFieldEndsTo(String fieldName, String endValue) {
        checkNoEmptyString(fieldName, "fieldName is invalid");
        checkNoEmptyString(endValue, "endValue is invalid");
        filters.add(Filter.getFilterEndTo(fieldName, endValue));
        setSearchType(SearchType.getAll);
        return this;
    }

    /**
     * Match records whose field value is between a
     * specified lower bound and upper bound (both inclusive).
     * @param fieldName  the specific field.
     * @param startValue the value (inclusive) to match the filter against
     * @param endValue   the value (inclusive) to match the filter against
     * @return this
     */
    public Query filterByFieldInRange(String fieldName, String startValue,
                                      String endValue) {
        checkNoEmptyString(fieldName, "fieldName is invalid");
        checkNoEmptyString(startValue, "startValue is invalid");
        checkNoEmptyString(endValue, "endValue is invalid");
        filters.add(Filter.getFilterInRange(fieldName, startValue, endValue));
        setSearchType(SearchType.getAll);
        return this;
    }

    /**
     * TODO disabled in the alpha version
     * The engine supports specifying boolean expressions as filters For
     * example, the user can submit the "log(year) + 5 > log(2003)" as a
     * expression to the engine using the boolean filter
     * @param booleanExpression expression to evaluate
     * @return this
     */
    Query filterByBooleanExpression(String booleanExpression) {
        checkNoEmptyString(booleanExpression, "booleanExpression is invalid");
        filters.add(Filter.getFilterBooleanExpression(booleanExpression));
        setSearchType(SearchType.getAll);
        return this;
    }

    /**
     * Compounds filters of a query by ANDing them.
     * <br><br>
     * Note: the engine supports only one kind of boolean operator (OR or AND)
     * between all the filter terms. This function is set to connect all the
     * filters using <code>AND</code> operator.
     * @return this
     */
    public Query setFilterRelationAND() {
        filterConnector = Filter.BooleanOperation.AND;
        return this;
    }

    /**
     * Compounds filters of a query by ORing them.
     * <br><br>
     * Note: the engine supports only one kind of boolean operator (OR or AND)
     * between all the filter terms. This function is set to connect all the
     * filters using <code>OR</code> operator.
     * @return this
     */
    public Query setFilterRelationOR() {
        filterConnector = Filter.BooleanOperation.OR;
        return this;
    }

    /**
     * Internal use, this is automatically set when filter or sort is turned on
     * Set the search type to get the topK or all result. By default only
     * topK result is returned.
     *
     * @param searchType {@link SearchType}
     * @return this
     */
    Query setSearchType(SearchType searchType) {
        checkIsNotNull(searchType, "SearchType can not be null");
        this.searchType = searchType;
        return this;
    }

    /**
     * Creates a query configured to sort the search results of that query
     * by the specified field(s). Equivalent to the SQLite parameter {@code ORDER BY}.
     * <br><br>
     * The SRCH2 search server's default behavior is to sort the results using descending
     * order by the overall score of each record. The user can specify sorting
     * by other fields.
     * @param field1 the first field to order by
     * @param restFields the rest of the fields to order by
     * @return this
     */
    public Query sortOnFields(String field1, String... restFields) {
        checkNoEmptyString(field1, "field name is empty");
        StringBuilder sb = new StringBuilder();
        sb.append("sort=").append(field1);
        if (restFields != null) {
            for (String f : restFields) {
                checkNoEmptyString(field1, "field name is empty");
                sb.append(',').append(f);
            }
        }
        sortSentence = sb.toString();
        setSearchType(SearchType.getAll);
        return this;
    }

    /**
     * Specifies the ordering to be ascending. This ordering is valid for all the
     * fields specified in the sort parameter. Equivalent to the SQLite parameter {@code ASC}.
     * @return this
     */
    public Query orderByAscending() {
        orderbySentence = "orderby=asc";
        return this;
    }

    /**
     * Specifies the ordering to be descending. This ordering is valid for all the
     * fields specified in the sort parameter. Equivalent to the SQLite parameter {@code DESC}.
     *
     * @return this
     */
    public Query orderByDescending() {
        orderbySentence = "orderby=desc";
        return this;
    }

    /**
     * Sets the offset of records returned in the complete search result set: ie, where the set
     * of returned records should begin. The default value is 0.
     *
     * @param startOffset the start offset of the result set
     * @return this
     */
    public Query pagingStartFrom(int startOffset) {
        this.pagingStart = startOffset;
        return this;
    }

    /**
     * Sets the number of records to return from the complete search result set.
     *
     * @param sizePerPage how many results to return per search
     * @return this
     */
    public Query pagingSize(int sizePerPage) {
        this.pagingRows = sizePerPage;
        return this;
    }

    /**
     * Configures the query to do a geo-search bounded by a rectangle region of
     * the specified coordinates.
     *
     * @param leftBottomLatitude the left bottom latitude value
     * @param leftBottomLongitude the left bottom longitude value
     * @param rightTopLatitude  the right top latitude value
     * @param rightTopLongitude the right top longitude value
     * @return this
     */
    public Query insideRectangle(double leftBottomLatitude,
                                 double leftBottomLongitude, double rightTopLatitude,
                                 double rightTopLongitude) {
        this.geoPosition.clear();
        this.geoPosition.add(leftBottomLatitude);
        this.geoPosition.add(leftBottomLongitude);
        this.geoPosition.add(rightTopLatitude);
        this.geoPosition.add(rightTopLongitude);

        return this;
    }

    /**
     * Configures the query to do a geo-search bounded by a circular region of
     * the specified radius and center coordinate.
     *
     * @param centerLatitude the center latitude value
     * @param centerLongitude the center longitude value
     * @param radius the radius to search within
     * @return this
     */
    public Query insideCircle(double centerLatitude,
                              double centerLongitude, double radius) {
        this.geoPosition.clear();
        this.geoPosition.add(centerLatitude);
        this.geoPosition.add(centerLongitude);
        this.geoPosition.add(radius);
        return this;
    }

    public String toString() {
        if (proximitySentence != null) {
            return proximitySentence;
        }
        StringBuilder sb = new StringBuilder("");
        if (term != null) { // normal query
            // term[s]
            sb.append("q=");
            sb.append(term.toString());
            // filter[s]
            if (filters.size() > 0) {
                sb.append('&').append(
                        (Filter.ConnectWithOperation(filters, filterConnector)));
            }

            // facet
            if (facetFields.size() > 0) {
                sb.append('&').append((FacetField.ConnectFacetFields(facetFields)));
            }

            // searchType
            if (searchType != null) {
                sb.append('&').append("searchType=").append(searchType.name());
            }
            // sort
            if (sortSentence != null) {
                sb.append('&').append((sortSentence));
                // orderby
                if (orderbySentence != null) {
                    sb.append('&').append((orderbySentence));
                }
            }

            // paging
            if (pagingStart != null) {
                sb.append('&').append("start=").append(pagingStart);
            }
            if (pagingRows != null) {
                sb.append('&').append("rows=").append(pagingRows);
            }

        }
        // Geo
        if (geoPosition.size() > 0) {
            if (term != null){ // geo search with keyword
                sb.append('&');
            } // else do need to add that '&'
            if (geoPosition.size() == 4) {
                sb.append(BOX_TAG[0]).append(geoPosition.get(0));
                for (int i = 1; i < 4; ++i) {
                    sb.append('&').append(BOX_TAG[i])
                            .append(geoPosition.get(i));
                }

            } else if (geoPosition.size() == 3) {
                sb.append(CIRCLE_TAG[0])
                        .append(geoPosition.get(0));
                for (int i = 1; i < 3; ++i) {
                    sb.append('&').append(CIRCLE_TAG[i])
                            .append(geoPosition.get(i));
                }
            } else {
                throw new IllegalStateException("should never happen");
            }

        }
        return sb.toString();
    }

    /**
     * TODO future release Please note: facet must also be enabled from the
     * configuration file by setting the "facetEnabled" tag to true. FacetField function is protected as of now.
     *
     * @param fieldName name of field to facet on
     * @return this
     */
    Query facetOn(String fieldName) {
        facetFields.add(FacetField.getFacetFieldOnCategory(fieldName));
        return this;
    }

    /**
     * Control how many rows to fetch
     *
     * @param fieldName name of field to face on
     * @param rows the number of rows to return for the faceted field
     * @return this
     */
    Query facetOn(String fieldName, int rows) {
        facetFields.add(FacetField.getFacetFieldOnCategory(fieldName, rows));
        return this;
    }

    /**
     * TODO future release Note that all start, end, and gap values, if given in
     * the query, will override the corresponding values in the configuration
     * file.
     *
     * @param fieldName name of the field to facet on
     * @return this
     */
    Query facetOnRange(String fieldName, String start, String end,
                                 String gap) {
        facetFields.add(FacetField.getFacetFieldOnRange(fieldName, start, end,
                gap));
        return this;
    }

    /**
     * !!! Note this is internal use.
     * Set the searchType for the query <br>
     * <code>topK</code> : The results will be sorted descending by their score. This
     * score is calculated for each record. This approach has a high
     * performance, but does not support sort operation. <br>
     * <code>getAll</code>: Use this strategy if sort query parameters is
     * needed.
     */
    enum SearchType {
        /**
         * topK: The results will be sorted descending by their score. This
         * score is calculated for each record. This approach has a high
         * performance.
         */
        topK,
        /**
         * getAll: Use this strategy if sort query parameters is
         * needed.<br>
         * <b>Note</b>: If the searchType is '<code>getAll</code>' and if the
         * expected number of results is more than 10,000, the engine will
         * return the top 2000 results based on the ranking score
         */
        getAll,
    }

    static class FacetField {
        static final String FACET_CATEGORY = "facet.field=";
        static final String FACET_RANGE = "facet.range=";
        static final String FACET_RANGE_START = ".facet.start=";
        static final String FACET_RANGE_END = ".facet.end=";
        static final String FACET_RANGE_GAP = ".facet.gap=";
        static final String FACET_ENABLE = "facet=true";
        final String fieldName;
        final TYPE type;
        final Integer rows;
        final String start;
        final String end;
        final String gap;

        private FacetField(String fieldName, TYPE type, Integer rows,
                           String start2, String end2, String gap2) {
            this.fieldName = fieldName;
            this.rows = rows;
            this.start = start2;
            this.end = end2;
            this.gap = gap2;
            this.type = type;
        }

        static FacetField getFacetFieldOnCategory(String fieldName) {
            return new FacetField(fieldName, TYPE.CATEGORY, null, null, null,
                    null);
        }

        static FacetField getFacetFieldOnCategory(String fieldName, int topRows) {
            return new FacetField(fieldName, TYPE.CATEGORY, topRows, null,
                    null, null);
        }

        static FacetField getFacetFieldOnRange(String fieldName) {
            return new FacetField(fieldName, TYPE.RANGE, null, null, null, null);
        }

        static FacetField getFacetFieldOnRange(String fieldName, String start,
                                               String end, String gap) {
            return new FacetField(fieldName, TYPE.RANGE, null, start, end, gap);
        }

        static String ConnectFacetFields(ArrayList<FacetField> list) {
            StringBuilder sb = new StringBuilder();
            sb.append(FACET_ENABLE);
            for (FacetField field : list) {
                sb.append('&').append(field.toString());
            }
            return sb.toString();
        }

        public String toString() {
            StringBuilder sb = new StringBuilder();
            switch (type) {
                case CATEGORY:
                    sb.append(FACET_CATEGORY).append(fieldName);
                    if (rows != null) {
                        sb.append("&f.").append(fieldName).append(".rows=")
                                .append(rows);
                    }
                    break;
                case RANGE:
                    sb.append(FACET_RANGE).append(fieldName);
                    if (start != null && end != null && gap != null) {
                        sb.append("&f.").append(fieldName)
                                .append(FACET_RANGE_START).append(start);
                        sb.append("&f.").append(fieldName).append(FACET_RANGE_END)
                                .append(end);
                        sb.append("&f.").append(fieldName).append(FACET_RANGE_GAP)
                                .append(gap);
                    }
                    break;
            }
            return sb.toString();
        }

        enum TYPE {
            CATEGORY, RANGE,
        }

    }

}
