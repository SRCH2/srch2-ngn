package com.srch2.android.sdk;

import junit.framework.Assert;
import org.junit.Test;

public class HighlighterTest {

    @Test(expected = NullPointerException.class)
    public void testNull(){
        Highlighter h = new Highlighter(null, null, null, null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testIllegalCustomString(){
        Highlighter h = new Highlighter("", "", "", "");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testIllegalCustomStringContainingSingleQuote(){
        Highlighter h = new Highlighter("\'hello", "a", "b", "c");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testIllegalColorStringNonValid(){
        Highlighter.SimpleHighlighter h = new Highlighter.SimpleHighlighter().formatExactTextMatches(true, true, "#ZZFFEE");
    }


    @Test(expected = IllegalArgumentException.class)
    public void testIllegalColorStringNonValidLength(){
        Highlighter.SimpleHighlighter h = new Highlighter.SimpleHighlighter().formatExactTextMatches(true, true, "abab");
    }

    @Test(expected = NullPointerException.class)
    public void testIllegalColorStringNull(){
        Highlighter.SimpleHighlighter h = new Highlighter.SimpleHighlighter().formatExactTextMatches(true, true, null);
    }

    @Test
    public void testSimpleHighlighter(){
        Highlighter.SimpleHighlighter h = new Highlighter.SimpleHighlighter()
                                                .formatExactTextMatches(true, true, "#FF0000")
                                                .formatFuzzyTextMatches(true, true, "#FF00FF");
        Assert.assertEquals(h.highlightingHasBeenCustomSet, false);
        Assert.assertEquals(true, h.highlightBoldExact);
        Assert.assertEquals(true, h.highlightBoldFuzzy);
        Assert.assertEquals(true, h.highlightItalicExact);
        Assert.assertEquals(true, h.highlightItalicFuzzy);
        Assert.assertEquals(h.highlightColorExact, "FF0000");
        Assert.assertEquals(h.highlightColorFuzzy, "FF00FF");

        h = new Highlighter.SimpleHighlighter()
                .formatExactTextMatches(false, true)
                .formatFuzzyTextMatches(false, true);
        Assert.assertEquals(h.highlightingHasBeenCustomSet, false);
        Assert.assertEquals(false, h.highlightBoldExact);
        Assert.assertEquals(false, h.highlightBoldFuzzy);
        Assert.assertEquals(true, h.highlightItalicExact);
        Assert.assertEquals(true, h.highlightItalicFuzzy);
        Assert.assertNull(h.highlightColorExact);
        Assert.assertNull(h.highlightColorExact);
    }

    @Test
    public void testCustomHighlighter(){
        Highlighter h = new Highlighter("EXACTPRE", "EXACTPOST", "FUZZYPRE", "FUZZYPOST");
        Assert.assertEquals(h.highlightingHasBeenCustomSet, true);
        Assert.assertEquals(h.highlightExactPreTag, "EXACTPRE");
        Assert.assertEquals(h.highlightExactPostTag, "EXACTPOST");
        Assert.assertEquals(h.highlightFuzzyPreTag, "FUZZYPRE");
        Assert.assertEquals(h.highlightFuzzyPostTag, "FUZZYPOST");
    }
}
