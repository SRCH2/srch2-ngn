package com.srch2.android.http.service;

/**
 * Represents the primary key attribute of the index. The primary key has tobe textual type
 * PrimaryKey is set to <code>searchable = false</code>, and <code>refining = true </code>by default.
 * <br><br>
 * PrimaryKey Field is the special field to construct a {@link com.srch2.android.http.service.Schema}.
 */
final public class PrimaryKeyField{
    Field primaryKey;

    PrimaryKeyField(Field field){
        this.primaryKey = field;
        this.primaryKey.required = true;
    }

}
