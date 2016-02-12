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

import org.junit.Assert;
import org.junit.Test;

public class FieldTest {

    @Test
    public void testDefaultValue() {
        Field f = Field.createSearchableField("f1");
        Assert.assertTrue(f != null);

        Assert.assertTrue(f.boost == Field.DEFAULT_BOOST_VALUE);
        Assert.assertTrue(!f.facetEnabled);
        Assert.assertTrue(f.name.equals("f1"));
        Assert.assertTrue(f.type == Field.InternalType.TEXT);

        Assert.assertTrue(f.searchable);
        Assert.assertFalse(f.refining);
        Assert.assertFalse(f.required);
        Assert.assertFalse(f.highlight);
        Assert.assertTrue(f.facetType == null);
        Assert.assertFalse(f.isRecordBoostField);

        f = Field.createRefiningField("f2", Field.Type.INTEGER);
        Assert.assertTrue(f != null);

        Assert.assertTrue(f.boost == Field.DEFAULT_BOOST_VALUE);
        Assert.assertTrue(!f.facetEnabled);
        Assert.assertTrue(f.name.equals("f2"));
        Assert.assertTrue(f.type == Field.InternalType.INTEGER);

        Assert.assertTrue(f.refining);
        Assert.assertFalse(f.searchable);
        Assert.assertFalse(f.required);
        Assert.assertFalse(f.highlight);
        Assert.assertTrue(f.facetType == null);
        Assert.assertFalse(f.isRecordBoostField);

        f = Field.createRefiningField("f2", Field.Type.TEXT);
        Assert.assertTrue(f != null);

        Assert.assertTrue(f.boost == Field.DEFAULT_BOOST_VALUE);
        Assert.assertTrue(!f.facetEnabled);
        Assert.assertTrue(f.name.equals("f2"));
        Assert.assertTrue(f.type == Field.InternalType.TEXT);

        Assert.assertTrue(f.refining);
        Assert.assertFalse(f.searchable);
        Assert.assertFalse(f.required);
        Assert.assertFalse(f.highlight);
        Assert.assertTrue(f.facetType == null);
        Assert.assertFalse(f.isRecordBoostField);

        f = Field.createRefiningField("f2", Field.Type.FLOAT);
        Assert.assertTrue(f.type == Field.InternalType.FLOAT);
        Assert.assertFalse(f.isRecordBoostField);

        f = Field.createRefiningField("f2", Field.Type.TIME);
        Assert.assertTrue(f.type == Field.InternalType.TIME);
        Assert.assertFalse(f.isRecordBoostField);
    }

    @Test
    public void testBoostedFields() {

        Field boostedField = Field.createSearchableField("f", 4);
        Assert.assertTrue(boostedField.boost == 4);
        Assert.assertTrue(boostedField.name.equals("f"));
        Assert.assertTrue(boostedField.type == Field.InternalType.TEXT);
        Assert.assertTrue(!boostedField.facetEnabled);

        Assert.assertTrue(boostedField.searchable);
        Assert.assertFalse(boostedField.refining);
        Assert.assertFalse(boostedField.required);
        Assert.assertFalse(boostedField.highlight);
        Assert.assertTrue(boostedField.facetType == null);
        Assert.assertFalse(boostedField.isRecordBoostField);
    }

    @Test
    public void testRecordBoostField() {

        RecordBoostField recordBoostField = Field.createRecordBoostField("boost");
        Assert.assertTrue(recordBoostField.recordBoost.name.equals("boost"));
        Assert.assertTrue(recordBoostField.recordBoost.type == Field.InternalType.FLOAT);
        Assert.assertTrue(!recordBoostField.recordBoost.facetEnabled);
        Assert.assertFalse(recordBoostField.recordBoost.searchable);
        Assert.assertTrue(recordBoostField.recordBoost.refining);
        Assert.assertFalse(recordBoostField.recordBoost.highlight);
        Assert.assertTrue(recordBoostField.recordBoost.isRecordBoostField);
        Assert.assertFalse(recordBoostField.recordBoost.required);
    }

    @Test
    public void testSearchableAndRefiningField() {

        Field srField = Field.createSearchableAndRefiningField("srField");
        Assert.assertTrue(srField.boost == Field.DEFAULT_BOOST_VALUE);
        Assert.assertTrue(srField.name.equals("srField"));
        Assert.assertTrue(srField.type == Field.InternalType.TEXT);
        Assert.assertTrue(!srField.facetEnabled);

        Assert.assertTrue(srField.searchable);
        Assert.assertTrue(srField.refining);
        Assert.assertFalse(srField.required);
        Assert.assertFalse(srField.highlight);
        Assert.assertTrue(srField.facetType == null);

        srField = Field.createSearchableAndRefiningField("srField");
        Assert.assertTrue(srField.type == Field.InternalType.TEXT);

        srField = Field.createSearchableAndRefiningField("srField");
        Assert.assertTrue(srField.type == Field.InternalType.TEXT);


    }

    @Test
    public void testBoostedSearchableAndRefiningField() {

        Field srField = Field.createSearchableAndRefiningField("srField",  4);
        Assert.assertTrue(srField.boost == 4);
        Assert.assertTrue(srField.name.equals("srField"));
        Assert.assertTrue(srField.type == Field.InternalType.TEXT);
        Assert.assertTrue(!srField.facetEnabled);

        Assert.assertTrue(srField.searchable);
        Assert.assertTrue(srField.refining);
        Assert.assertFalse(srField.required);
        Assert.assertFalse(srField.highlight);
        Assert.assertTrue(srField.facetType == null);

        srField = Field.createSearchableAndRefiningField("srField", 4);
        Assert.assertTrue(srField.boost == 4);
        Assert.assertTrue(srField.type == Field.InternalType.TEXT);

    }

    @Test
    public void testHighlightedField() {
        Field f = Field.createSearchableAndRefiningField("highlight");
        Assert.assertFalse(f.highlight);
        f.enableHighlighting();
        Assert.assertTrue(f.highlight);
        f = Field.createRefiningField("highlighter", Field.Type.FLOAT).enableHighlighting();
        Assert.assertFalse(f.highlight);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableField() {
        Field f = Field.createSearchableField(null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableField2() {
        Field f = Field.createSearchableField(null, 1);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableField3() {
        Field f = Field.createSearchableField("f", 0);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableField4() {
        Field f = Field.createSearchableField("f", 101);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableField5() {
        Field f = Field.createSearchableField(null, 101);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableField6() {
        Field f = Field.createSearchableField("");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInRefiningField() {
        Field f = Field.createRefiningField(null, Field.Type.FLOAT);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInRefiningField2() {
        Field f = Field.createRefiningField("f1", null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInRefiningField3() {
        Field f = Field.createRefiningField(null, null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInRefiningField4() {
        Field f = Field.createRefiningField("", Field.Type.FLOAT);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableAndRefiningField() {
        Field srField = Field.createSearchableAndRefiningField(null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableAndRefiningField5() {
        Field srField = Field.createSearchableAndRefiningField("srField", 101);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableAndRefiningField7() {
        Field srField = Field.createSearchableAndRefiningField(null, 1);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableAndRefiningField8() {
        Field srField = Field.createSearchableAndRefiningField("",  1);
    }

}
