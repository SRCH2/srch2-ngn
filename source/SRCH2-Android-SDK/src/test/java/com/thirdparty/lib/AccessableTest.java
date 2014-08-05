package com.thirdparty.lib;

import org.junit.Test;

import com.srch2.android.http.service.Query;
import com.srch2.android.http.service.SearchableTerm;
import com.srch2.android.http.service.SearchableTerm.CompositeTerm;

import java.io.UnsupportedEncodingException;

public class AccessableTest {

    @Test
    public void testAccessable() throws UnsupportedEncodingException {
        CompositeTerm term = new SearchableTerm("abc").OR(new SearchableTerm(
                "bcd"));
        Query q = new Query(term.NOT());
        q.toString();
    }
}
