package com.srch2.android.sdk;

import java.util.Locale;

/**
 * Represents an attribute of the index. An index is described by its schema, and the schema consists of fields. If
 * an index is compared to the table of an SQLite database, a field is comparable to the column of that table. Each
 * field must be associated with a type--whether it is textual, numerical or geographical--and is identified by
 * the value set when constructing the field as its <code>fieldName</code>. Fields can be searchable, refining,
 * searchable and refining, or geographical. Searchable fields are those whose data associated with the field is
 * matched against the keywords of the search input during a search--they are also textual in type; refining fields,
 * on the other hand, are not matched against the key words of the search input, but are used to do advanced search
 * functionality such as limiting the range of the search results or to do post-processing operations. Geographical
 * fields can be used to do geo-searches.
 * <br><br>
 * Fields are used when constructing an {@link Schema} in the
 * {@link Indexable} implementation
 * of the method {@link Indexable#getSchema()}. They should not be constructed or used otherwise.
 */
final public class Field {

    enum FacetType {
        CATEGORICAL, RANGE
    }

    /**
     * The type of the data that the field will be associated with.
     */
    public enum Type {
        TEXT, INTEGER, FLOAT, TIME
    }

    enum InternalType {
        TEXT, INTEGER, FLOAT, TIME, LOCATION_LONGITUDE, LOCATION_LATITUDE;
        static final InternalType[] values = InternalType.values();
    }

    /**
     * Defines the default boost value that searchable fields who boost values are not otherwise set will have.
     * <br><br>
     * Has the <bold>constant</bold> value of <code>1</code>.
     */
    static final public int DEFAULT_BOOST_VALUE = 1;
    final String name;
    final InternalType type;
    int boost = DEFAULT_BOOST_VALUE;
    boolean refining = false;
    boolean searchable = false;
    boolean facetEnabled = false;
    boolean required = false;
    boolean highlight = false;
    boolean isRecordBoostField = false;

    FacetType facetType = null;
    int facetStart = 0;
    int facetEnd = 0;
    int facetGap = 0;

    void setAsCategoricalFacet() {
        facetEnabled = true;
        facetType = FacetType.CATEGORICAL;
    }

    void setAsRangedFacet(int start, int end, int facetGap) {
        if (start > end) {
            throw new IllegalArgumentException(
                    "start value cannot be greater than end value, Illegal argument");
        }

        facetEnabled = true;
        facetType = FacetType.RANGE;
        facetStart = start;
        facetEnd = end;
        this.facetGap = facetGap;
    }

    Field(String name, InternalType ft, boolean searchable, boolean refining,
          int boost) {
        checkIfFieldNameIsValid(name);
        checkIfFieldTypeIsValid(ft);
        this.name = name;
        this.type = ft;
        this.searchable = searchable;
        this.refining = refining;
        this.boost = boost;
        this.facetEnabled = false;
    }

    /**
     * Static factory method for obtaining a refining PrimaryKey field.
     * PrimaryKey is always the textual in type. The data will be
     * returned with the search results whenever a search is performed, but it will not be
     * matched against the key words of the search input. Using the {@link Query} class
     * the data of this field can be used to filter or sort on search results.
     * <br><br>
     * This method will throw an exception if the value of
     * <code>fieldName</code> is null or has a length less than one.
     *
     * @param fieldName the name identifying the primary key field
     * @return the {@link PrimaryKeyField}
     */
    public static PrimaryKeyField createDefaultPrimaryKeyField(String fieldName) {
        return new PrimaryKeyField(createRefiningField(fieldName, Type.TEXT));
    }


    /**
     * Static factory method for obtaining a searchable and refining PrimaryKey field with
     * configurable boost value 'boost'.
     * Whenever a search on the index, the data of this primary key field will also be
     * returned with the search results.
     * <br><br>
     * The value of the <code>boost</code> argument will be used to calculate the score of the of search
     * results, making this field proportionally more or less relevant than other searchable fields. By
     * default this value is set to one.
     * <br><br>
     * This method will throw an exceptions if the value passed for
     * <code>boost</code> is less than one or greater than one hundred; or if the value of
     * <code>fieldName</code> is null or has a length less than one.
     *
     * @param fieldName the name identifying the primary key field.
     * @param boost the value to assign to the relevance of this field, relative to other searchable fields
     * @return the {@link PrimaryKeyField}
     */
    public static PrimaryKeyField createSearchablePrimaryKeyField(String fieldName, int boost) {
        return new PrimaryKeyField(createSearchableAndRefiningField(fieldName, boost));
    }

    /**
     * Static factory method for obtaining a searchable and refining PrimaryKey field with default boost value of one.
     * The data of this primary key field will be
     * returned with the search results whenever a search is performed.
     * <br><br>
     * This method will throw an exception if the value of
     * <code>fieldName</code> is null or has a length less than one.
     *
     * @param fieldName the name identifying the field
     * @return the {@link PrimaryKeyField}
     */
    public static PrimaryKeyField createSearchablePrimaryKeyField(String fieldName){
        return createSearchablePrimaryKeyField(fieldName, DEFAULT_BOOST_VALUE);
    }


    /**
     * Static factory method for obtaining a record boost field.
     * Record boost field is always the float in type. The value that this is set for each
     * record will be used in the scoring expression to compute each record's score. This
     * type of field can be used to assign relevance to each record: for instance, if creating
     * an index about contacts, all starred contacts could have the record boost field value
     * set to fifty, while all non-starred contacts could have the value set to one. The value
     * should never be set to zero, which will render the record unavailable to search results.
     * <br><br>
     * This method will throw an exception if the value of
     * <code>fieldName</code> is null or has a length less than one.
     *
     * @param fieldName the name identifying the record boost field
     * @return the {@link RecordBoostField}
     */
    public static RecordBoostField createRecordBoostField(String fieldName) {
        return new RecordBoostField(createRefiningField(fieldName, Type.FLOAT));
    }

    /**
     * Static factory method for obtaining a searchable field with configurable boost value 'boost'.
     * Data associated with this field must be textual in type and will be searchable whenever a search
     * on the index whose schema includes this field is searched. The data of this field will also be
     * returned with the search results whenever a search is performed.
     * <br><br>
     * The value of the <code>boost</code> argument will be used to calculate the score of the of search
     * results, making this field proportionally more or less relevant than other searchable fields. By
     * default this value is set to one.
     * <br><br>
     * This method will throw an exceptions if the value passed for
     * <code>boost</code> is less than one or greater than one hundred; or if the value of
     * <code>fieldName</code> is null or has a length less than one.
     *
     * @param fieldName the name identifying the field
     * @param boost the value to assign to the relevance of this field, relative to other searchable fields
     * @return the searchable field
     */
    public static Field createSearchableField(String fieldName, int boost) {
        if (boost < 1 || boost > 100) {
            throw new IllegalArgumentException(
                    "Boost value cannot be less than 1 or greater than 100.");
        }
        return new Field(fieldName, InternalType.values[Type.TEXT.ordinal()],
                true, false, boost);
    }

    /**
     * Static factory method for obtaining a searchable field with default boost value of one.
     * Data associated with this field must be textual in type and will be searchable whenever a search
     * on the index whose schema includes this field is searched. The data of this field will also be
     * returned with the search results whenever a search is performed.
     * <br><br>
     * This method will throw an exception if the value of
     * <code>fieldName</code> is null or has a length less than one.
     *
     * @param fieldName the name identifying the field
     * @return the searchable field
     */
    public static Field createSearchableField(String fieldName) {
        return createSearchableField(fieldName, DEFAULT_BOOST_VALUE);
    }

    /**
     * Static factory method for obtaining a refining field.
     * Data associated with this field must be have a type specified by the value of
     * <code>fieldType</code>. The data of this field will be
     * returned with the search results whenever a search is performed, but it will not be
     * matched against the key words of the search input. Using the <code>Query</code> class
     * the data of this field can be used to filter search results, however, such as in limiting
     * the range of values if the field type is numerical.
     * <br><br>
     * This method will throw an exception if the value of
     * <code>fieldName</code> is null or has a length less than one; or if the
     * value of <code>fieldType</code> is null.
     *
     * @param fieldName the name identifying the field
     * @param fieldType the type of the field (Field.FieldType.TEXT,
     *                  Field.FieldType.INTEGER, Field.FieldType.FLOAT,
     *                  Field.FieldType.TIME, Field.FieldType.LOCATION_LONGITUDE,
     *                  Field.FieldType.LOCATION_LATITUDE)
     * @return the refining field
     */
    public static Field createRefiningField(String fieldName, Type fieldType) {
        checkIfFieldTypeIsValid(fieldType);
        return new Field(fieldName, InternalType.values[fieldType.ordinal()],
                false, true, DEFAULT_BOOST_VALUE);
    }

    /**
     * Static factory method for obtaining a field that is both searchable and
     * refining with the searchable component having a the default boost value
     * of one. The searchable field can only be textual type.
     * The data associated with this field will be matched against the
     * search input during a search, can also be used to support advanced
     * search features with the <code>Query</code> class such as limiting the
     * range of the search results, and can also be used to do post-processing
     * operations on the search results.
     * <br><br>
     * This method will throw an exception if the value of
     * <code>fieldName</code> is null or has a length less than one.
     *
     * @param fieldName the name identifying the field
     * @return the refining and searchable field
     */
    public static Field createSearchableAndRefiningField(String fieldName) {
        return new Field(fieldName, InternalType.TEXT,
                true, true, DEFAULT_BOOST_VALUE);
    }

    /**
     * Static factory method for obtaining a field that is both searchable and
     * refining with the searchable component having a the default boost value
     * of one. The searchable field can only be textual type.
     * The data associated with this field will be matched against the
     * search input during a search, can also be used to support advanced
     * search features with the <code>Query</code> class such as limiting the
     * range of the search results, and can also be used to do post-processing
     * operations on the search results.
     * <br><br>
     * The value of the <code>boost</code> argument will be used to calculate the score of the of search
     * results, making this field proportionally more or less relevant than other searchable fields. By
     * default this value is set to one.
     * <br><br>
     * This method will throw an exceptions if the value passed for
     * <code>boost</code> is less than one or greater than one hundred; or if the value of
     * <code>fieldName</code> is null or has a length less than one.
     *
     * @param fieldName the name identifying the field
     * @param boost     the value to assign to the relevance of this field, relative to other searchable fields
     * @return the refining and searchable field
     */
    public static Field createSearchableAndRefiningField(String fieldName,int boost) {
        if (boost < 1 || boost > 100) {
            throw new IllegalArgumentException(
                    "Boost value cannot be less than 1 or greater than 100.");
        }
        return new Field(fieldName, InternalType.TEXT,
                true, true, boost);
    }

    private void checkIfFieldNameIsValid(String name) {
        if (name == null || name.length() < 1) {
            throw new IllegalArgumentException("Value of name is not valid");
        }
    }

    private void checkIfFieldTypeIsValid(InternalType ft) {
        if (ft == null) {
            throw new IllegalArgumentException(
                    "Value for field type is not valid");
        }
    }

    private static void checkIfFieldTypeIsValid(Type fieldType) {
        if (fieldType == null) {
            throw new IllegalArgumentException(
                    "Value for field type is not valid");
        }
    }

    /**
     * Enables highlighting feature for the field. The SRCH2 search server
     * will generate a snippet for this field whenever keywords of the search
     * input match the data associated with this field: for instance, if the data
     * was 'All You Need is Love' and the search input was 'lo' the 'Lo' in 'Love'
     * would be surrounded by HTML tags colorizing this text.
     * <br><br>
     * This method returns the <code>Field</code> itself so that it can have
     * these calls cascaded.
     *
     * @return the corresponding field for cascading method calls
     */
    public Field enableHighlighting() {
        highlight = true;
        return this;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((name == null) ? 0 : name.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        Field other = (Field) obj;
        if (name == null) {
            if (other.name != null)
                return false;
        } else if (!name.equals(other.name))
            return false;
        return true;
    }

    static String toXML(Field field) {
        StringBuilder fieldXML = new StringBuilder();

        if (field.isRecordBoostField) {
            fieldXML.append("			<field name=\"").append(field.name)
                    .append("\" type=\"")
                    .append(field.type.name().toLowerCase(Locale.ENGLISH))
                    .append("\" default=\"").append(1)
                    .append("\" searchable=\"").append(field.searchable)
                    .append("\" refining=\"").append(field.refining)
                    .append("\"").append(" required=\"")
                    .append(field.required).append("\"/>\n");
        } else if (field.type == InternalType.LOCATION_LATITUDE
                || field.type == InternalType.LOCATION_LONGITUDE) {
            fieldXML.append("			<field name=\"").append(field.name)
                    .append("\" default=\"").append(0)
                    .append("\" required=\"").append("true")
                    .append("\" type=\"")
                    .append(field.type.name().toLowerCase(Locale.ENGLISH))
                    .append("\"/>\n");
        } else {
            fieldXML.append("			<field name=\"").append(field.name)
                    .append("\" type=\"")
                    .append(field.type.name().toLowerCase(Locale.ENGLISH))
                    .append("\" searchable=\"").append(field.searchable)
                    .append("\" refining=\"").append(field.refining)
                    .append("\"").append(" required=\"")
                    .append(field.required).append("\"/>\n");
        }
        return fieldXML.toString();
    }
}
