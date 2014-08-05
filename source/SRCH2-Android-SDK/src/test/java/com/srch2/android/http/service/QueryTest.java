package com.srch2.android.http.service;

import org.junit.Test;
import org.junit.Assert;

import java.io.UnsupportedEncodingException;

public class QueryTest {

    @Test
    public void testSampleQueriesOnDocWeb() throws UnsupportedEncodingException {
        // f1) curl -i "http://localhost:8081/search?q=terminator"
        final String title = "terminator";
        Query q = new Query(new SearchableTerm(title));
        Assert.assertEquals(title, q.toString());

        // f2) http://localhost:8081/search?q=title:terminator AND
        // director:cameron
        final String director = "cameron";
        SearchableTerm term1 = new SearchableTerm(title)
                .searchSpecificField("title");
        SearchableTerm term2 = new SearchableTerm(director)
                .searchSpecificField("director");
        q = new Query(term1.AND(term2));
        Assert.assertEquals("title:terminator AND director:cameron",
                q.toString());

        // f3) http://localhost:8081/search?q=title:terminator AND
        // director:cameron&fq=year:[1983 TO 1992]
        q.filterByFieldInRange("year", "1983", "1992");
        Assert.assertEquals(
                "title:terminator AND director:cameron&fq=year:[1983 TO 1992]&searchType=getAll",
                q.toString());

        Query q4 = new Query(term1.AND(term2));
        Assert.assertEquals("title:terminator AND director:cameron",
                q4.toString());
        // f4) "http://localhost:8081/search?q=title:terminator AND
        // director:cameron&fq=genre:action
        q4.filterByFieldEqualsTo("genre", "action");
        Assert.assertEquals(
                "title:terminator AND director:cameron&fq=genre:action&searchType=getAll",
                q4.toString());

        // f5) http://localhost:8081/search?q=title:terminator AND
        // director:cameron&fq=boolexp$ year>1982 $
        Query q5 = new Query(term1.AND(term2));
        Assert.assertEquals("title:terminator AND director:cameron",
                q5.toString());
        q5.filterByBooleanExpression("year>1982");
        Assert.assertEquals(
                "title:terminator AND director:cameron&fq=boolexp$ year>1982 $&searchType=getAll",
                q5.toString());

        // f6) curl -i
        // "http://localhost:8081/search?q=cameron&sort=year&orderby=asc"

        Query q6 = new Query(new SearchableTerm("cameron"));
        q6.sortOnFields("year").orderByAscending();
        Assert.assertEquals("cameron&searchType=getAll&sort=year&orderby=asc", q6.toString());

        // f7) curl -i
        // "http://localhost:8081/search?q=cameron&facet=true&facet.field=genre"

        Query q7 = new Query(new SearchableTerm("cameron"));
        q7.facetOn("genre");
        Assert.assertEquals("cameron&facet=true&facet.field=genre",
                q7.toString());
    }

    @Test
    public void testTerm() throws UnsupportedEncodingException {
        Assert.assertEquals("term*", new SearchableTerm("term")
                .setIsPrefixMatching(true).toString());
        Assert.assertEquals("spielburrg~0.8", new SearchableTerm("spielburrg")
                .enableFuzzyMatching(0.8f).toString());

        Assert.assertEquals(
                "sta*^4~0.6 AND wars~0.8",
                new SearchableTerm("sta")
                        .setIsPrefixMatching(true)
                        .setBoostValue(4)
                        .enableFuzzyMatching(0.6f)
                        .AND(new SearchableTerm("wars")
                                .enableFuzzyMatching(0.8f)).toString());

    }

    @Test
    public void testProximity() {
        Assert.assertEquals("\"saving ryan\"~1",
                new Query("saving", "ryan", 1).toString());
    }

    @Test
    public void testBooleanExp() throws UnsupportedEncodingException {
        Assert.assertEquals(
                "(\"star+wars\" AND \"episode+3\") OR (\"George+Lucas\" AND NOT \"Indiana+Jones\")",
                new SearchableTerm("star wars")
                        .AND(new SearchableTerm("episode 3"))
                        .OR(new SearchableTerm("George Lucas")
                                .AND(new SearchableTerm("Indiana Jones").NOT()))
                        .toString());

        Assert.assertEquals(
                "(big AND fish) AND (\"Tim+Burton\" OR MacGregor~0.5)",
                new SearchableTerm("big")
                        .AND(new SearchableTerm("fish"))
                        .AND(new SearchableTerm("Tim Burton")
                                .OR(new SearchableTerm("MacGregor")
                                        .enableFuzzyMatching(0.5f))).toString());
    }

    @Test
    public void testFilter() throws UnsupportedEncodingException {

        Assert.assertEquals(
                "term&fq=year:[2010 TO 2012]&searchType=getAll",
                new Query(new SearchableTerm("term")).filterByFieldInRange(
                        "year", "2010", "2012").toString());

        Assert.assertEquals(
                "term&fq=genre:[comedy TO drama]&searchType=getAll",
                new Query(new SearchableTerm("term")).filterByFieldInRange(
                        "genre", "comedy", "drama").toString());

        Assert.assertEquals(
                "term&fq=id:[1000 TO *] AND genre:drama AND year:[* TO 1975]&searchType=getAll",
                new Query(new SearchableTerm("term"))
                        .filterByFieldStartsFrom("id", "1000")
                        .filterByFieldEqualsTo("genre", "drama")
                        .filterByFieldEndsTo("year", "1975")
                        .setFilterConnectorAND().toString());

        Assert.assertEquals(
                "term&fq=id:[1000 TO *] OR genre:drama OR year:[* TO 1975]&searchType=getAll",
                new Query(new SearchableTerm("term"))
                        .filterByFieldStartsFrom("id", "1000")
                        .filterByFieldEqualsTo("genre", "drama")
                        .filterByFieldEndsTo("year", "1975")
                        .setFilterConnectorOR().toString());

    }

    @Test
    public void testFacet() throws UnsupportedEncodingException {
        Assert.assertEquals(
                "term&facet=true&facet.field=year&facet.field=genre",
                new Query(new SearchableTerm("term")).facetOn("year")
                        .facetOn("genre").toString());
    }

    @Test
    public void testSort() throws UnsupportedEncodingException {
        Assert.assertEquals(
                "term&searchType=getAll&sort=director,year,title&orderby=asc",
                new Query(new SearchableTerm("term"))
                        .sortOnFields("director", "year", "title")
                        .orderByAscending().toString());
    }

    @Test
    public void testPaging() throws UnsupportedEncodingException {
        Assert.assertEquals("term&start=10&rows=7", new Query(
                new SearchableTerm("term")).pagingSize(7).pagingStartFrom(10)
                .toString());
    }

    @Test
    public void testGeo() throws UnsupportedEncodingException {
        // girardelli&start=0&rows=20&lblat=61.20&lblong=-149.90&rtlat=61.22&rtlong=-149.70
        Assert.assertEquals(
                "girardelli&start=0&rows=20&lblat=61.2&lblong=-149.9&rtlat=61.22&rtlong=-149.7",
                new Query(new SearchableTerm("girardelli")).pagingStartFrom(0)
                        .pagingSize(20)
                        .insideBoxRegion(61.20, -149.90, 61.22, -149.70)
                        .toString());
    }

}
