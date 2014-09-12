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
