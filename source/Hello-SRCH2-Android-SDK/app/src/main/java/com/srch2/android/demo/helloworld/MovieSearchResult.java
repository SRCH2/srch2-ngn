package com.srch2.android.demo.helloworld;

public class MovieSearchResult {

    // This class is a model for the SearchResultsAdapter data set.
    // Its field represents the visible data of a MovieIndex record.

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
