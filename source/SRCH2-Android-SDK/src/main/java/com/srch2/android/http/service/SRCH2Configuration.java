package com.srch2.android.http.service;

import java.io.FileNotFoundException;
import java.io.PrintWriter;
import java.io.UnsupportedEncodingException;
import java.math.BigInteger;
import java.security.SecureRandom;
import java.util.HashMap;

@SuppressWarnings("StringEquality")
final class SRCH2Configuration {

    static final String LISTENING_PORT = "listeningPort";
    static final String LISTENING_HOSTNAME = "listeningHostname";
    static final String MAX_SEARCH_THREADS = "maxSearchThreads";
    static final String DEFAULT_CORE_NAME = "defaultCoreName";
    static final String SRCH2_HOME_FOLDER_DEFAULT_NAME = "srch2/";
    static final String HOSTNAME = "127.0.0.1";
    static final String AUTHORIZATION_KEY = "authorization-key";
    final HashMap<String, IndexInternal> indexesMap = new HashMap<String, IndexInternal>();
    private String fullPathOfSRCH2home = "srch2";
    private int maxSearchThreads = 2;
    private int port = 8081;
    private String authorizationKey = "";

    /**
     * The constructor to build a SRCH2Configuration. User can give multiple indexes to one configuration
     *
     * @param index1      the index
     * @param restIndexes the other more indexes
     */
    SRCH2Configuration(Indexable index1, Indexable... restIndexes) {
        index1.indexInternal = createIndex(index1.getIndexDescription());
        if (restIndexes != null) {
            for (Indexable idx : restIndexes) {
                if (idx == null) {
                    throw new NullPointerException("the given index is null");
                }
                idx.indexInternal = createIndex(idx.getIndexDescription());
            }
        }
    }

    /**
     * It returns configuration object in the form of an XML string.
     *
     * @return It returns an XML string for the corresponding Configuration
     * object
     */
    protected static String toXML(SRCH2Configuration conf) {

        if (conf.indexesMap.size() == 0) {
            throw new IllegalStateException("No index provided");
        }

        String defaultIndexName = conf.indexesMap.values().iterator().next().getIndexCoreName();

        StringBuilder configurationXML = new StringBuilder("<config>\n")
                .append("<srch2Home>")
                .append(conf.fullPathOfSRCH2home)
                .append("</srch2Home>\n")
                .append(conf.formAuthorizationKey())
                .append("    <listeningHostname>")
                .append(HOSTNAME)
                .append("</listeningHostname>\n")
                .append("    <listeningPort>")
                .append(conf.getPort())
                .append("</listeningPort>\n")
                .append("\n")
                .append("    <!-- moved from <query> which is now per core -->\n")
                .append("    <maxSearchThreads>")
                .append(conf.maxSearchThreads)
                .append("</maxSearchThreads>\n")
                .append("\n")
                .append("    <!-- Testing multiple cores here -->\n").append("    <cores defaultCoreName=\"").append(defaultIndexName).append("\">\n");

        for (IndexInternal indexInternal : conf.indexesMap.values()) {
            configurationXML.append(indexInternal.getConf()
                    .indexStructureToXML());
        }

        configurationXML.append("   </cores>\n" + "</config>\n");
        return configurationXML.toString();
    }

    String getUrlString() {
        return "http://" + HOSTNAME + ":" + getPort() + "/";
    }

    /**
     * This function generates the configuration XML file.
     *
     * @param pathOfXMLfile path where the configuration file has to be serialized
     * @throws FileNotFoundException
     * @throws UnsupportedEncodingException
     */
    void writeToSRCH2XML(String pathOfXMLfile) throws FileNotFoundException,
            UnsupportedEncodingException {
        PrintWriter writer = new PrintWriter(pathOfXMLfile, "UTF-8");
        writer.println(toXML(this));
        writer.close();
    }

    /**
     * This function adds IndexStructure objects into Configuration Object.
     *
     * @param indexDescription
     */
    IndexInternal createIndex(IndexDescription indexDescription) {
        IndexInternal indexInternal = new IndexInternal(indexDescription);

        checkIfIndexNameValidAndAreadyExistedThrowIfNot(indexDescription.getIndexName());
        indexesMap.put(indexInternal.getIndexCoreName(), indexInternal);
        return indexInternal;
    }

    /**
     * It returns the SRCH2Home, path where engine stores the serialized files.
     *
     * @return
     */
    protected String getSRCH2Home() {
        return fullPathOfSRCH2home;
    }

    /**
     * This value specifies the path of a folder where the engine stores.
     * serialized index files.
     */
    protected void setSRCH2Home(String fullPath) {
        fullPathOfSRCH2home = fullPath;
    }

    /**
     * It returns the port to which the engine is bound.
     *
     * @return
     */
    protected int getPort() {
        return port;
    }

    /**
     * It sets the port of the local machine to which the engine is bound.
     *
     * @param port
     */
    protected void setPort(int port) {
        this.port = port;
    }

    private String formAuthorizationKey() {
        StringBuilder auth = new StringBuilder("");
        if (this.authorizationKey == "") {
            this.authorizationKey = generateAuthorizationKey();
            auth = auth.append("	<authorization-key>").append(this.authorizationKey).append("</authorization-key>\n");
            return auth.toString();
        } else {
            auth = auth.append("	<authorization-key>").append(this.authorizationKey).append("</authorization-key>\n");
        }
        return auth.toString();
    }

    protected String toXML() {
        return SRCH2Configuration.toXML(this);
    }

    private String generateAuthorizationKey() {
        SecureRandom random = new SecureRandom();
        return new BigInteger(130, random).toString(32);
    }

    protected String getAuthorizationKey() {
        return this.authorizationKey;
    }

    /**
     * This parameter specifies an authorization key that is required in each HTTP request to the engine.
     * If this key is specified, each valid HTTP request needs to provide the following key-value pair in order to get the authorization.
     * OAuth=foobar
     * Example: curl -i "http://localhost:8081/search?q=terminator&OAuth=foobar"
     *
     * @param authorizationKey
     */
    void setAuthorizationKey(String authorizationKey) {
        if (authorizationKey == null) {
            throw new NullPointerException("Authorization key cannot be null");
        }
        this.authorizationKey = authorizationKey;
    }

    boolean checkIfIndexNameValidAndAreadyExistedThrowIfNot(String indexName) {
        if (indexName == null) {
            throw new NullPointerException("Cannot pass null indexName.");
        } else if (indexName != null && indexName.length() < 1) {
            throw new IllegalArgumentException("Cannot pass empty string as indexName.");
        } else if (indexesMap.containsKey(indexName)) {
            throw new IllegalArgumentException("The string indexName already existed.");
        }
        return true;
    }

    boolean checkIfIndexNameValidAndThrowIfNot(String indexName) {
        if (indexName == null) {
            throw new NullPointerException("Cannot pass null indexName.");
        } else if (indexName != null && indexName.length() < 1) {
            throw new IllegalArgumentException("Cannot pass empty string as indexName.");
        } else if (!indexesMap.containsKey(indexName)) {
            throw new IllegalArgumentException("The string indexName must correspond to the name of Indexable passed into SRCH2Configuration upon SRCH2Engine.initialization(...).");
        }
        return true;
    }

    IndexInternal getIndexAndThrowIfNotThere(String name) {
        checkIfIndexNameValidAndThrowIfNot(name);
        return indexesMap.get(name);
    }
}