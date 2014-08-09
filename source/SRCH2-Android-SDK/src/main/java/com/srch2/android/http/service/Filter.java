package com.srch2.android.http.service;

import java.util.ArrayList;

/*the engine supports only one kind of boolean operator (OR or AND) between all the filter terms.*/
class Filter {

    enum BooleanOperation {
        AND, OR,
    }

    private final String fieldName;
    private final String equalTo;
    private final String start;
    private final String end;
    private final String booleanExpression;

    private Filter(String fieldName, String equalTo, String start, String end,
                   String booleanExpression) {
        this.fieldName = fieldName;
        this.equalTo = equalTo;
        this.start = start;
        this.end = end;
        this.booleanExpression = booleanExpression;
    }

    static Filter getFilterEqualTo(String fieldName, String equalTo) {
        return new Filter(fieldName, equalTo, null, null, null);
    }

    static Filter getFilterStartFrom(String fieldName, String startFrom) {
        return new Filter(fieldName, null, startFrom, "*", null);
    }

    static Filter getFilterEndTo(String fieldName, String endTo) {
        return new Filter(fieldName, null, "*", endTo, null);
    }

    static Filter getFilterInRange(String fieldName, String start, String end) {
        return new Filter(fieldName, null, start, end, null);
    }

    static Filter getFilterBooleanExpression(String booleanExpression) {
        return new Filter(null, null, null, null, booleanExpression);
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        if (fieldName != null) {
            sb.append(fieldName).append(':');
            if (equalTo != null) {
                sb.append(equalTo);
            } else {
                if (start != null && end != null) {
                    sb.append('[').append(start).append(" TO ").append(end)
                            .append(']');
                } else {
                    throw new IllegalStateException("should never happen!");
                }
            }
        } else if (booleanExpression != null) {
            sb.append("boolexp$ ").append(booleanExpression).append(" $");
        } else {
            throw new IllegalStateException("should never happen!");
        }
        return sb.toString();
    }

    static String ConnectWithOperation(ArrayList<Filter> filters,
                                       BooleanOperation op) {
        StringBuilder sb = new StringBuilder();
        sb.append("fq=");
        sb.append(filters.get(0).toString());
        if (filters.size() == 1) {
            return sb.toString();
        }
        for (int i = 1; i < filters.size(); ++i) {
            sb.append(' ').append(op.name()).append(' ')
                    .append(filters.get(i).toString());
        }
        return sb.toString();
    }
}
