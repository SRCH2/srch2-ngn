package com.srch2.android.http.service;

import org.json.JSONArray;
import org.json.JSONObject;

public abstract class Indexable {

    IndexInternal indexInternal;

    /**
     * User need to implement this method to define what inside this Index.
     * @return the Description of this Index
     */
    abstract public IndexDescription getIndexDescription();

    /**
     * Insert one record into the IndexInternal
     *
     * @param record Json format record
     */
    public void insert(JSONObject record) {
        if (indexInternal != null) {
            indexInternal.insert(record);
        }
    }

    /**
     * Insert a list of records into the IndexInternal in a batch way
     *
     * @param records A array of JSON records tobe inserted.
     */
    public void insert(JSONArray records) {
        if (indexInternal != null) {
            indexInternal.insert(records);
        }
    }

    /**
     * Update the record in the engine.
     *
     * @param record JSON recordto be inserted.
     */
    public void update(JSONObject record) {
        if (indexInternal != null) {
            indexInternal.update(record);
        }

    }

    /**
     * Update a list of records in the engine.
     *
     * @param records A array of JSON records tobe inserted.
     */
    public void update(JSONArray records) {
        if (indexInternal != null) {
            indexInternal.update(records);
        }
    }

    /**
     * Delete the record using the record id, or a list of ids.
     *
     * @param id the primary key of the record
     */
    public void delete(String id) {
        if (indexInternal != null) {
            indexInternal.delete(id);
        }
    }

    /**
     * Get the information from the Index
     */
    public void info() {
        if (indexInternal != null) {
            indexInternal.info();
        }
    }

    /**
     * Do a basic search on the current indexInternal. The queryString contains the
     * space will be split and searched using the multiple keywords. All query
     * keywords will be treated all as fuzzy matching, and the last keyword will
     * be treated as fuzzy and prefix matching. For the more specific setting on
     * the terms, please use {@link #advancedSearch(Query)} } method.
     *
     * @param searchInput the string input
     */
    public void search(String searchInput) {
        if (indexInternal != null) {
            indexInternal.search(searchInput);
        }
    }

    /**
     * Does a more powerful search using the {@link Query} object
     *
     * @param query query to be searched.
     */
    public void advancedSearch(Query query) {
        if (indexInternal != null) {
            indexInternal.advancedSearch(query);
        }
    }

    /**
     * Get the record by giving its primary key. The record will be sent to the
     * <code>searchResultsListener</code>
     *
     * @param id the primary key
     */
    public void getRecordbyID(String id) {
        if (indexInternal != null) {
            indexInternal.getRecordbyID(id);
        }
    }

}
