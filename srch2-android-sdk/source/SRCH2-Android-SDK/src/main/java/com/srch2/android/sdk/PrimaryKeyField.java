package com.srch2.android.sdk;

/**
 * Represents the primary key attribute of the index. The primary key will be textual in type;
 * however it is permissible to pass integer values, and they will be automatically converted.
 * <br><br>
 * The primary key of a record is the handle by which it can be retrieved and deleted. See
 * {@link Indexable#getRecordbyID(String)} and {@link Indexable#delete(String)} for more
 * information.
 * <br><br>
 * Every index must contain a primary key field, therefore every {@link Indexable} must override
 * {@link com.srch2.android.sdk.Indexable#getSchema()} and construct the schema with {@code PrimaryKeyField}.
 * {@code PrimaryKeyField} instances can be obtained by calling {@link Field#createDefaultPrimaryKeyField(String)},
 * {@link Field#createSearchablePrimaryKeyField(String)} or {@link Field#createSearchablePrimaryKeyField(String, int)}.
 * <br><br>
 * The {@code PrimaryKeyField} is the always the first argument when obtaining a schema from the factory
 * static methods such as {@link Schema#createSchema(PrimaryKeyField, Field...)}.
 */
final public class PrimaryKeyField{
    Field primaryKey;

    PrimaryKeyField(Field field){
        this.primaryKey = field;
        this.primaryKey.required = true;
    }

}
