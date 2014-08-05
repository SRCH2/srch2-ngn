package com.srch2.android.http.service;

import org.junit.Assert;
import org.junit.Test;

public class FieldTest {

    @Test
    public void testDefaultValue() {
        Field f = Field.getSearchableField("f1");
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

        f = Field.getRefiningField("f2", Field.Type.INTEGER);
        Assert.assertTrue(f != null);

        Assert.assertTrue(f.boost == Field.DEFAULT_BOOST_VALUE);
        Assert.assertTrue(f.facetEnabled == false);
        Assert.assertTrue(f.name == "f2");
        Assert.assertTrue(f.type == Field.InternalType.INTEGER);

        Assert.assertTrue(f.refining);
        Assert.assertFalse(f.searchable);
        Assert.assertFalse(f.required);
        Assert.assertFalse(f.highlight);
        Assert.assertTrue(f.facetType == null);

        f = Field.getRefiningField("f2", Field.Type.TEXT);
        Assert.assertTrue(f != null);

        Assert.assertTrue(f.boost == Field.DEFAULT_BOOST_VALUE);
        Assert.assertTrue(f.facetEnabled == false);
        Assert.assertTrue(f.name == "f2");
        Assert.assertTrue(f.type == Field.InternalType.TEXT);

        Assert.assertTrue(f.refining);
        Assert.assertFalse(f.searchable);
        Assert.assertFalse(f.required);
        Assert.assertFalse(f.highlight);
        Assert.assertTrue(f.facetType == null);

        f = Field.getRefiningField("f2", Field.Type.FLOAT);
        Assert.assertTrue(f.type == Field.InternalType.FLOAT);

        f = Field.getRefiningField("f2", Field.Type.TIME);
        Assert.assertTrue(f.type == Field.InternalType.TIME);

    }

    @Test
    public void testBoostedFields() {

        Field boostedField = Field.getSearchableField("f", 4);
        Assert.assertTrue(boostedField.boost == 4);
        Assert.assertTrue(boostedField.name == "f");
        Assert.assertTrue(boostedField.type == Field.InternalType.TEXT);
        Assert.assertTrue(boostedField.facetEnabled == false);

        Assert.assertTrue(boostedField.searchable);
        Assert.assertFalse(boostedField.refining);
        Assert.assertFalse(boostedField.required);
        Assert.assertFalse(boostedField.highlight);
        Assert.assertTrue(boostedField.facetType == null);

    }

    @Test
    public void testSearchableAndRefiningField() {

        Field srField = Field.getSearchableAndRefiningField("srField", Field.Type.TEXT);
        Assert.assertTrue(srField.boost == Field.DEFAULT_BOOST_VALUE);
        Assert.assertTrue(srField.name == "srField");
        Assert.assertTrue(srField.type == Field.InternalType.TEXT);
        Assert.assertTrue(srField.facetEnabled == false);

        Assert.assertTrue(srField.searchable);
        Assert.assertTrue(srField.refining);
        Assert.assertFalse(srField.required);
        Assert.assertFalse(srField.highlight);
        Assert.assertTrue(srField.facetType == null);

        srField = Field.getSearchableAndRefiningField("srField", Field.Type.INTEGER);
        Assert.assertTrue(srField.type == Field.InternalType.INTEGER);

        srField = Field.getSearchableAndRefiningField("srField", Field.Type.TIME);
        Assert.assertTrue(srField.type == Field.InternalType.TIME);

        srField = Field.getSearchableAndRefiningField("srField", Field.Type.FLOAT);
        Assert.assertTrue(srField.type == Field.InternalType.FLOAT);

    }

    @Test
    public void testBoostedSearchableAndRefiningField() {

        Field srField = Field.getSearchableAndRefiningField("srField", Field.Type.TEXT, 4);
        Assert.assertTrue(srField.boost == 4);
        Assert.assertTrue(srField.name == "srField");
        Assert.assertTrue(srField.type == Field.InternalType.TEXT);
        Assert.assertTrue(srField.facetEnabled == false);

        Assert.assertTrue(srField.searchable);
        Assert.assertTrue(srField.refining);
        Assert.assertFalse(srField.required);
        Assert.assertFalse(srField.highlight);
        Assert.assertTrue(srField.facetType == null);

        srField = Field.getSearchableAndRefiningField("srField", Field.Type.INTEGER, 4);
        Assert.assertTrue(srField.boost == 4);
        Assert.assertTrue(srField.type == Field.InternalType.INTEGER);

        srField = Field.getSearchableAndRefiningField("srField", Field.Type.TIME, 4);
        Assert.assertTrue(srField.boost == 4);
        Assert.assertTrue(srField.type == Field.InternalType.TIME);

        srField = Field.getSearchableAndRefiningField("srField", Field.Type.FLOAT, 4);
        Assert.assertTrue(srField.boost == 4);
        Assert.assertTrue(srField.type == Field.InternalType.FLOAT);

    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableField() {
        Field f = Field.getSearchableField(null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableField2() {
        Field f = Field.getSearchableField(null, 1);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableField3() {
        Field f = Field.getSearchableField("f", 0);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableField4() {
        Field f = Field.getSearchableField("f", 101);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableField5() {
        Field f = Field.getSearchableField(null, 101);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableField6() {
        Field f = Field.getSearchableField("");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInRefiningField() {
        Field f = Field.getRefiningField(null, Field.Type.FLOAT);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInRefiningField2() {
        Field f = Field.getRefiningField("f1", null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInRefiningField3() {
        Field f = Field.getRefiningField(null, null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInRefiningField4() {
        Field f = Field.getRefiningField("", Field.Type.FLOAT);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableAndRefiningField() {
        Field srField = Field.getSearchableAndRefiningField(null, Field.Type.FLOAT);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableAndRefiningField2() {
        Field srField = Field.getSearchableAndRefiningField("srField", null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableAndRefiningField3() {
        Field srField = Field.getSearchableAndRefiningField(null, null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableAndRefiningField4() {
        Field srField = Field.getSearchableAndRefiningField("srField", null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableAndRefiningField5() {
        Field srField = Field.getSearchableAndRefiningField("srField", Field.Type.INTEGER, 101);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableAndRefiningField6() {
        Field srField = Field.getSearchableAndRefiningField("srField", null, 1);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableAndRefiningField7() {
        Field srField = Field.getSearchableAndRefiningField(null, Field.Type.INTEGER, 1);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testExceptionInSearchableAndRefiningField8() {
        Field srField = Field.getSearchableAndRefiningField("", Field.Type.INTEGER, 1);
    }

}
