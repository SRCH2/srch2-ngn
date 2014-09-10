package com.srch2.android.sdk;

public class RandomStringUtil {

    private final static String alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    private final static int alphabetLength = alphabet.length();

    public static String getRandomString(int length) {
        if (length < 1) {
            throw new IllegalArgumentException("Generated random string must be greater than 0 in length");
        }

        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < length; ++i) {
            sb.append(alphabet.charAt((int) (Math.random() * alphabetLength)));
        }

        return sb.toString();
    }
}
