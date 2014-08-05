package com.srch2.android.http.service;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.UnsupportedEncodingException;

public abstract class Indexable {

    IndexInternal indexInternal;

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
     * @param records
     */
    public void insert(JSONArray records) {
        if (indexInternal != null) {
            indexInternal.insert(records);
        }
    }

    /**
     * Update the record in the engine.
     *
     * @param record
     */
    public void update(JSONObject record) {
        if (indexInternal != null) {
            indexInternal.update(record);
        }

    }

    /**
     * Update a list of records in the engine.
     *
     * @param records
     */
    public void update(JSONArray records) {
        if (indexInternal != null) {
            indexInternal.update(records);
        }
    }

    /**
     * Delete the record using the record id, or a list of ids.
     *
     * @param id
     */
    public void delete(String id) {
        if (indexInternal != null) {
            indexInternal.delete(id);
        }
    }

    /**
     * Get the information from the indexInternal
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
     * @param searchInput
     * @throws UnsupportedEncodingException the searchInput is supposed to be UTF-8 encoded. Otherwise it will throw execption.
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
     * Get the record by giving its primary key The record will be sent to the
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
