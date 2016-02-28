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
package com.srch2.android.sdk;

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
