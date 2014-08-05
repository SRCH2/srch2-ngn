package com.srch2.android.http.service;

import junit.framework.Assert;
import org.junit.Test;

public class IndexDescriptionTest {

    @Test(expected = NullPointerException.class)
    public void testNull(){
        IndexDescription index = new IndexDescription(null,null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInvalidName(){
        IndexDescription index = new IndexDescription("",null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testNoSearchableField(){
        IndexDescription index = new IndexDescription("coreName", Field.getRefiningField("id", Field.Type.TEXT));
    }

    @Test
    public void testNoRestField(){

        IndexDescription index = new IndexDescription("coreName", Field.getSearchableField("id"));
        Assert.assertTrue(index.name.equals("coreName"));
        Assert.assertTrue(index.schema.fields.size() == 1);
        Assert.assertTrue(index.schema.fields.contains(Field.getSearchableField("id")));
    }

    @Test
    public void testConstructor(){
        IndexDescription index = new IndexDescription("core1", Field.getRefiningField("id", Field.Type.TEXT), Field.getSearchableField("text"));
        Assert.assertTrue(index.name.equals("core1"));
        Assert.assertTrue(index.schema.fields.size() == 2);
        Assert.assertTrue(index.schema.fields.contains(Field.getRefiningField("id", Field.Type.TEXT)));
        Assert.assertTrue(index.schema.fields.contains(Field.getSearchableField("text")));

        String lat = "lat";
        String lon = "lon";

        IndexDescription indexGeo = new IndexDescription("core2", Field.getRefiningField("id", Field.Type.FLOAT), lat, lon, Field.getSearchableField("title"));
        Assert.assertTrue(indexGeo.name.equals("core2"));
        Assert.assertTrue(indexGeo.schema.fields.size() == 4);
        Assert.assertTrue(indexGeo.schema.fields.contains(Field.getRefiningField("id", Field.Type.FLOAT)));
        Assert.assertTrue(indexGeo.schema.fields.contains(Field.getRefiningField(lat, Field.Type.FLOAT)));
        Assert.assertTrue(indexGeo.schema.fields.contains(Field.getRefiningField(lon, Field.Type.FLOAT)));
        Assert.assertTrue(indexGeo.schema.fields.contains(Field.getSearchableField("title")));





    }

    @Test
    public void miscTest(){
        IndexDescription index = new IndexDescription("core1", Field.getRefiningField("id", Field.Type.TEXT), Field.getSearchableField("text"));
        Assert.assertTrue(index.getIndexName().equals("core1"));

        index.setQueryTermSimilarityThreshold(0.5f);
        Assert.assertTrue(index.getQueryTermSimilarityThreshold() == 0.5f);

        index.setTopK(105);
        Assert.assertTrue(index.getTopK() == 105);

    }

    @Test (expected = IllegalArgumentException.class)
    public void invalidSimilarityTest() {
        IndexDescription index = new IndexDescription("core1", Field.getRefiningField("id", Field.Type.TEXT), Field.getSearchableField("text"));
        Assert.assertTrue(index.getIndexName().equals("core1"));

        index.setQueryTermSimilarityThreshold(-0.5f);
    }

    @Test (expected = IllegalArgumentException.class)
    public void invalidTopK() {
        IndexDescription index = new IndexDescription("core1", Field.getRefiningField("id", Field.Type.TEXT), Field.getSearchableField("text"));
        Assert.assertTrue(index.getIndexName().equals("core1"));

        index.setTopK(-105);
    }
}
