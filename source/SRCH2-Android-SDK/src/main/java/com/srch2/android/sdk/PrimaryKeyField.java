package com.srch2.android.sdk;

/**
 * Represents the primary key attribute of the index. The primary key has to be textual type
 * <code>PrimaryKeyField</code> is set to <code>searchable = false</code>, and <code>refining = true</code> by default.
 * <br><br>
 * <code>PrimaryKeyField</code> is a special field to construct a {@link Schema}.
 */
final public class PrimaryKeyField{
    Field primaryKey;

    PrimaryKeyField(Field field){
        this.primaryKey = field;
        this.primaryKey.required = true;
    }

}
