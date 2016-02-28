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

/**
 * Represents the record boost field attribute of the index. This is the field whose data determines the ranking or
 * score relative of the record relative to other records: for instance, if in an index containing information
 * about contacts, if a given contact is starred the record boost field for that contact's record could be set 50,
 * and 1 otherwise. This will boost all starred contacts above other contacts in the search results.
 * <br><br>
 * {@code RecordBoostField} is of type REAL and can be integer. Values for this field should be within the range
 * of less than 100 and greater than or equal to one hundred.
 * <br><br>
 * A {@code RecordBoostField} instance can be obtained from the static factory method
 * {@link Field#createDefaultPrimaryKeyField(String)} and should only be created when constructing a schema in an
 * {@code Indexable} implementation of the method {@link com.srch2.android.sdk.Indexable#getSchema()}. This field
 * is optional for an index, but if present it is always the second argument to the static factory method when
 * obtaining a schema {@link Schema#createSchema(PrimaryKeyField, RecordBoostField, Field...)}.
 */
final public class RecordBoostField {
    Field recordBoost;

    RecordBoostField(Field field){
        this.recordBoost = field;
        this.recordBoost.required = false;
        this.recordBoost.highlight = false;
        this.recordBoost.refining = true;
        this.recordBoost.searchable = false;
        this.recordBoost.isRecordBoostField = true;
    }

}
