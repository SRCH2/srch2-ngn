package com.srch2.android.demo.hellosrch2;

public class MovieSearchResult {

    private String title;
    private String genre;
    private int year;

    public String getTitle() {
        return title;
    }

    public void setTitle(String title) {
        this.title = title;
    }

    public String getGenre() {
        return genre;
    }

    public void setGenre(String genre) {
        this.genre = genre;
    }

    public int getYear() {
        return year;
    }

    public void setYear(int year) {
        this.year = year;
    }

    public MovieSearchResult(String title, String genre, int year) {

    }

}
