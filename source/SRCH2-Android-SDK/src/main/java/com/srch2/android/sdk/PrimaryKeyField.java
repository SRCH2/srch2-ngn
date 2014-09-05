package com.srch2.android.sdk;

/**
 * Represents the primary key attribute of the index. The primary key will be textual in type;
 * however it is permissible to pass integer values, and they will be automatically converted.
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
