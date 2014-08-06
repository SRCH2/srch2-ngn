package com.srch2.android.http.service;

import com.srch2.android.http.service.Filter.BooleanOperation;

import java.util.ArrayList;

public class Query {

    private static final String[] BOX_TAG = {"lblat=", "lblong=", "rtlat=", "rtlong="};
    private static final String[] CIRCLE_TAG = {"clat=", "clong=", "radius="};
    /**
     * this field will disable all the advanced settings
     */
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
     * Creates a query from a {@link SearchableTerm} or
     * {@link SearchableTerm.CompositeTerm};
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
     * Proximity Search The engine supports finding words that are within a
     * specified proximity. To use this feature, enablePositionIndex flag in
     * the configuration should be set to true.
     * <p/>
     * For example, the following query searches for records with the keywords
     * "saving" and "ryan" within 1 word
     * <p/>
     * <pre>
     * {@code Query("saving", "ryan", 1)}
     * </pre>
     * <p/>
     * (that is, either zero or one word separates them) of each other in a
     * document:
     * <p/>
     * Notice that a proximity search does not support edit-distance-based fuzzy
     * match, i.e., we do not allow typos in the keywords in the quotes.
     */
    public Query(String term1, String term2, int distance) {
        checkNoEmptyString("saving", "invalid query keyword");
        checkNoEmptyString(term2, "invalid query keyword");
        if (distance < 1) {
            throw new IllegalArgumentException("distance should >= 1");
        }
        this.proximitySentence = "\"" + "saving" + " " + term2 + "\"~"
                + Integer.toString(distance);
        this.term = null;
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
     * This method is used to specify a filter restricting the set of records
     * returned, <code>fields[fieldName].value == equalToValue</code>.
     *
     * @param fieldName    the specific field.
     * @param equalToValue the equalTo value.
     * @return this
     */
    public Query filterByFieldEqualsTo(String fieldName, String equalToValue) {
        checkNoEmptyString(fieldName, "fieldName is invalid");
        checkNoEmptyString(equalToValue, "equalToValue is invalid");
        filters.add(Filter.getFilterEqualTo(fieldName, equalToValue));
        setSearchType(SearchType.getAll);
        return this;
    }

    /**
     * It allows us to match records whose field value starts from
     * a specified lower bound.
     *
     * @param fieldName  the specific field.
     * @param startValue the startValue (inclusive).
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
     * It allows us to match records whose field value ends at a
     * specified upper bound.
     *
     * @param fieldName the specific field.
     * @param endValue  the endValue (inclusive).
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
     * It allows us to match records whose field value is between a
     * specified lower bound and upper bound (both inclusive)
     *
     * @param fieldName  the specific field.
     * @param startValue the startValue (inclusive).
     * @param endValue   the endValue (inclusive).
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
     *
     * @param booleanExpression
     * @return this
     */
    Query filterByBooleanExpression(String booleanExpression) {
        checkNoEmptyString(booleanExpression, "booleanExpression is invalid");
        filters.add(Filter.getFilterBooleanExpression(booleanExpression));
        setSearchType(SearchType.getAll);
        return this;
    }

    /**
     * Note: the engine supports only one kind of boolean operator (OR or AND)
     * between all the filter terms. This function is set to connect all the
     * filters using <code>AND</code> operator
     *
     * @return this
     */
    public Query setFilterConnectorAND() {
        filterConnector = Filter.BooleanOperation.AND;
        return this;
    }

    /**
     * Note: the engine supports only one kind of boolean operator (OR or AND)
     * between all the filter terms. This function is set to connect all the
     * filters using <code>OR</code> operator
     *
     * @return this
     */
    public Query setFilterConnectorOR() {
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
     * The engine's default behavior is to sort the results using a descending
     * order by the overall score of each record. The user can specify sorting
     * by other fields.
     * <p/>
     * Note the <code>SearchType</code> will be automatically set to
     * <code>getAll</code> method
     *
     * @param field1 the first field
     * @param restFields the rest of the fields
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
     * It specifies the order in which the result set should be sorted. This order is valid for all the
     * fields specified in the sort parameter.
     *
     * @return this
     */
    public Query orderByAscending() {
        orderbySentence = "orderby=asc";
        return this;
    }

    /**
     * It specifies the order in which the result set should be sorted. Engine by
     * default uses "desc" (descending) order. This order is valid for all the
     * fields specified in the sort parameter.
     *
     * @return this
     */
    public Query orderByDescending() {
        orderbySentence = "orderby=desc";
        return this;
    }

    /**
     * It is the offset in the complete result set of the query, where the set
     * of returned records should begin. The default value is 0
     *
     * @param startOffset specify the start offset of the result
     * @return this
     */
    public Query pagingStartFrom(int startOffset) {
        this.pagingStart = startOffset;
        return this;
    }

    /**
     * It indicates the number of records to return from the complete result
     * set.
     *
     * @param sizePerPage specify how many result within one search response
     * @return this
     */
    public Query pagingSize(int sizePerPage) {
        this.pagingRows = sizePerPage;
        return this;
    }

    /**
     * Get the GeoLocation search by specify a bounding box. <br>
     *
     * @param leftBottomLatitude
     * @param leftBottomLongitude
     * @param rightTopLatitude
     * @param rightTopLongitude
     * @return this
     */
    public Query insideBoxRegion(double leftBottomLatitude,
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
     * Get the GeoLocation search by specify a center point and a radius. <br>

     *
     * @param centerLatitude
     * @param centerLongitude
     * @param radius
     * @return this
     */
    public Query insideCircleRegion(double centerLatitude,
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
        // term[s]
        StringBuilder sb = new StringBuilder(term.toString());
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

        // Geo
        if (geoPosition.size() > 0) {
            if (geoPosition.size() == 4) {
                for (int i = 0; i < 4; ++i) {
                    sb.append('&').append(BOX_TAG[i])
                            .append(geoPosition.get(i));
                }

            } else if (geoPosition.size() == 3) {
                for (int i = 0; i < 3; ++i) {
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
     * @param fieldName
     * @return this
     */
    protected Query facetOn(String fieldName) {
        facetFields.add(FacetField.getFacetFieldOnCategory(fieldName));
        return this;
    }

    /**
     * Control how many rows to fetch
     *
     * @param fieldName
     * @param rows
     * @return this
     */
    protected Query facetOn(String fieldName, int rows) {
        facetFields.add(FacetField.getFacetFieldOnCategory(fieldName, rows));
        return this;
    }

    /**
     * TODO future release Note that all start, end, and gap values, if given in
     * the query, will override the corresponding values in the configuration
     * file.
     *
     * @param fieldName
     * @return this
     */
    protected Query facetOnRange(String fieldName, String start, String end,
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
