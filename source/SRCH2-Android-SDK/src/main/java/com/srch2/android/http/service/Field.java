package com.srch2.android.http.service;

import java.util.Locale;

/**
 * This is the data field object to describe the searchable attribute.
 * User can use the static method to get the <code>Field</code>
 */
final public class Field {

    enum FacetType {
        CATEGORICAL, RANGE
    }

    /**
     * The data type
     */
    public enum Type {
        TEXT, INTEGER, FLOAT, TIME
    }

    enum InternalType {
        TEXT, INTEGER, FLOAT, TIME, LOCATION_LONGITUDE, LOCATION_LATITUDE;
        static final InternalType[] values = InternalType.values();
    }

    static final public int DEFAULT_BOOST_VALUE = 1;
    final String name;
    final InternalType type;
    int boost = DEFAULT_BOOST_VALUE;
    boolean refining = false;
    boolean searchable = false;
    boolean facetEnabled = false;
    boolean required = false;
    boolean highlight = false;

    FacetType facetType = null;
    int facetStart = 0;
    int facetEnd = 0;
    int facetGap = 0;

    protected void setAsCategoricalFacet() {
        facetEnabled = true;
        facetType = FacetType.CATEGORICAL;
    }

    protected void setAsRangedFacet(int start, int end, int facetGap) {
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
     * It returns a searchable field with configurable boost value 'boost'.
     * Engine will include its values in the indices to support keyword search
     * within the field.
     *
     * @param fieldName Name of the field
     * @return It returns a searchable field
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
     * It returns a searchable field with boost value 1. Engine will include its
     * values in the indices to support keyword search within the field.
     *
     * @param fieldName Name of the field
     * @return It returns a searchable field
     */
    public static Field createSearchableField(String fieldName) {
        return createSearchableField(fieldName, DEFAULT_BOOST_VALUE);
    }

    /**
     * It returns a refining field with boost value 1. Engine will include its
     * values in the indices to support post-processing operations on query
     * results, such as additional filtering predicates (e.g., "price < 20"),
     * sort, and facet.
     *
     * @param fieldName Name of the field
     * @param fieldType Type of the field (Field.FieldType.TEXT,
     *                  Field.FieldType.INTEGER, Field.FieldType.FLOAT,
     *                  Field.FieldType.TIME, Field.FieldType.LOCATION_LONGITUDE,
     *                  Field.FieldType.LOCATION_LATITUDE
     * @return It returns a refining field
     */
    public static Field createRefiningField(String fieldName, Type fieldType) {
        checkIfFieldTypeIsValid(fieldType);
        return new Field(fieldName, InternalType.values[fieldType.ordinal()],
                false, true, DEFAULT_BOOST_VALUE);
    }

    /**
     * It returns a field that is both searchable and refining with boost value
     * 1. Engine will include its values in the indices to support keyword
     * search within the field and also support post-processing operations on
     * query results.
     *
     * @param fieldName Name of the field
     * @param fieldType Type of the field (Field.FieldType.TEXT,
     *                  Field.FieldType.INTEGER, Field.FieldType.FLOAT,
     *                  Field.FieldType.TIME, Field.FieldType.LOCATION_LONGITUDE,
     *                  Field.FieldType.LOCATION_LATITUDE
     * @return It returns a field that is both searchable and refining
     */
    public static Field createSearchableAndRefiningField(String fieldName,
                                                         Type fieldType) {
        checkIfFieldTypeIsValid(fieldType);
        return new Field(fieldName, InternalType.values[fieldType.ordinal()],
                true, true, DEFAULT_BOOST_VALUE);
    }

    /**
     * It returns a field that is both searchable and refining with configurable
     * boost value 'boost'. Engine will include its values in the indices to
     * support keyword search within the field and also support post-processing
     * operations on query results.
     *
     * @param fieldName Name of the field
     * @param fieldType Type of the field (Field.FieldType.TEXT,
     *                  Field.FieldType.INTEGER, Field.FieldType.FLOAT,
     *                  Field.FieldType.TIME, Field.FieldType.LOCATION_LONGITUDE,
     *                  Field.FieldType.LOCATION_LATITUDE
     * @return It returns a field that is both searchable and refining
     */
    public static Field createSearchableAndRefiningField(String fieldName,
                                                         Type fieldType, int boost) {
        if (boost < 1 || boost > 100) {
            throw new IllegalArgumentException(
                    "Boost value cannot be less than 1 or greater than 100.");
        }
        if(fieldType == null){
            throw new IllegalArgumentException("Field type cannot be null");
        }
        return new Field(fieldName, InternalType.values[fieldType.ordinal()],
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
     * It enables highlighting feature for the field. Engine will generate a
     * snippet for this field with matching keyword highlighted.
     *
     * @return It returns the corresponding field
     */
    public Field enableHighlighting() {
        highlight = true;
        return this;
    }

    void setAsPrimaryKeyBySettingRequiredToTrue() {
        required = true;
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

        if (field.type == InternalType.LOCATION_LATITUDE
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
