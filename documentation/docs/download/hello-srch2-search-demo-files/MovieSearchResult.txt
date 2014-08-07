package com.srch2.android.demo.hellosrch2;

public class MovieSearchResult {

    private String mTitle;
    private String mGenre;
    private int mYear;

    public String getTitle() {
        return mTitle;
    }

    public String getGenre() {
        return mGenre;
    }

    public int getYear() {
        return mYear;
    }

    public MovieSearchResult(String title, String genre, int year) {
        mTitle = title;
        mGenre = genre;
        mYear = year;
    }
}