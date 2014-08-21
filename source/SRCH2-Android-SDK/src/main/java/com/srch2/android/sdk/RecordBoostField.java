package com.srch2.android.sdk;

/**
 * Represents the record boost field attribute of the index. The record boost key has to be float type
 * <code>RecordBoostField</code> is set to <code>searchable = false</code>, and <code>refining = true</code>.
 * <br><br>
 * <code>RecordBoostField</code> is a special field to construct a {@link com.srch2.android.sdk.Schema}.
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
