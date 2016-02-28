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
