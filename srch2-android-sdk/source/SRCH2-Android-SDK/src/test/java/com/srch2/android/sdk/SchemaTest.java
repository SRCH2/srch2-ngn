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

import junit.framework.Assert;
import org.junit.Test;

public class SchemaTest {

    @Test(expected = IllegalArgumentException.class)
    public void testNull(){
        Schema s = new Schema(null, (Field[]) null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testNoSearchableField(){
        Schema s = new Schema(Field.createDefaultPrimaryKeyField("id"));
        boolean noSearchable = true;
        for (Field f : s.fields) {
            if (f.searchable) {
                noSearchable = false;
            }
        }
        Assert.assertTrue(noSearchable);
    }

    @Test
    public void testNoRestField(){
        Schema s = new Schema(Field.createSearchablePrimaryKeyField("id"));
        Assert.assertTrue(s.fields.size() == 1);
        Assert.assertTrue(s.fields.contains(Field.createSearchableField("id")));
    }

    @Test
    public void testConstructor(){
        Schema s = new Schema(Field.createDefaultPrimaryKeyField("id"), Field.createSearchableField("text"));
        Assert.assertTrue(s.fields.size() == 2);
        Assert.assertTrue(s.fields.contains(Field.createRefiningField("id", Field.Type.TEXT)));
        Assert.assertTrue(s.fields.contains(Field.createSearchableField("text")));
        Assert.assertNull(s.recordBoostKey);


        String lat = "lat";
        String lon = "lon";

        s = new Schema(Field.createDefaultPrimaryKeyField("id"), lat, lon, Field.createSearchableField("title"));

        Assert.assertTrue(s.fields.size() == 4);
        Assert.assertTrue(s.fields.contains(Field.createRefiningField("id", Field.Type.TEXT)));
        Assert.assertTrue(s.fields.contains(Field.createRefiningField(lat, Field.Type.FLOAT)));
        Assert.assertTrue(s.fields.contains(Field.createRefiningField(lon, Field.Type.FLOAT)));
        Assert.assertTrue(s.fields.contains(Field.createSearchableField("title")));
        Assert.assertNull(s.recordBoostKey);


        s = new Schema(Field.createDefaultPrimaryKeyField("id"),
                    Field.createRecordBoostField("boost"),
                        Field.createSearchableField("title"));

        Assert.assertTrue(s.fields.size() == 3);
        Assert.assertTrue(s.fields.contains(Field.createRefiningField("id", Field.Type.TEXT)));
        Assert.assertTrue(s.fields.contains(Field.createRefiningField("boost", Field.Type.FLOAT)));
        Assert.assertTrue(s.fields.contains(Field.createSearchableField("title")));
        Assert.assertNotNull(s.recordBoostKey);

    }

}
