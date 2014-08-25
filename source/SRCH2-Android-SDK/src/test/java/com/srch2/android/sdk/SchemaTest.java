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

    @Test
    public void testHighlightingPreAndPostScript() {

        Schema s = new Schema(Field.createDefaultPrimaryKeyField("id"), Field.createSearchableField("title").enableHighlighting());

        s.setHighlightedPreAndPostScript("FUZZYPRE", "FUZZYPOST", "EXACTPRE", "EXACTPOST");

        Assert.assertEquals(s.highlight_fuzzyPrefix, "FUZZYPRE");
        Assert.assertEquals(s.highlight_fuzzySuffix, "FUZZYPOST");
        Assert.assertEquals(s.highlight_exactPrefix, "EXACTPRE");
        Assert.assertEquals(s.highlight_exactSuffix, "EXACTPOST");

        s = new Schema(Field.createDefaultPrimaryKeyField("id"), Field.createSearchableField("title").enableHighlighting())
                    .formatExactTextMatchesHighlighting(true, true, "#FFFFFF")
                    .formatFuzzyTextMatchesHighlighting(true, true, "#0000AA");

        Assert.assertEquals(s.highlightBoldFuzzy, true);
        Assert.assertEquals(s.highlightBoldExact, true);
        Assert.assertEquals(s.highlightColorExact, "FFFFFF");
        Assert.assertEquals(s.highlightColorFuzzy, "0000AA");

        s = new Schema(Field.createDefaultPrimaryKeyField("id"), Field.createSearchableField("title").enableHighlighting())
                .formatExactTextMatchesHighlighting(false, true)
                .formatFuzzyTextMatchesHighlighting(false, true);

        Assert.assertEquals(s.highlightBoldFuzzy, false);
        Assert.assertEquals(s.highlightBoldExact, false);
        Assert.assertEquals(s.highlightItalicFuzzy, true);
        Assert.assertEquals(s.highlightItalicExact, true);
        Assert.assertNull(s.highlightColorExact);
        Assert.assertNull(s.highlightColorFuzzy);
    }
}
