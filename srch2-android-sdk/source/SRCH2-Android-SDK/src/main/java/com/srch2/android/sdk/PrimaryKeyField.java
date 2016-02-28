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
