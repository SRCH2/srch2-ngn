package com.srch2.android.http.service;

import java.util.HashSet;
import java.util.Iterator;

final class Schema {

    protected final String uniqueKey;
    protected HashSet<Field> fields;
    protected boolean facetEnabled = false;
    protected int indexType = 0;

    Schema(Field primaryKeyField, Field... remainingField) {
        if (primaryKeyField == null) {
            throw new IllegalArgumentException(
                    "Value of the field cannot be null");
        }

        primaryKeyField.setAsPrimaryKeyBySettingRequiredToTrue();

        uniqueKey = primaryKeyField.name;
        fields = new HashSet<Field>();
        fields.add(primaryKeyField);
        boolean searchableFieldExist = primaryKeyField.searchable;

        for (Field f : remainingField) {
            if (f == null) {
                throw new IllegalArgumentException(
                        "Value of the field cannot be null");
            }
            addToFields(f);
            if (!searchableFieldExist && f.searchable) {
                searchableFieldExist = true;
            }
        }
        if (!searchableFieldExist) {
            throw new IllegalArgumentException("No searchable field provided. The Engine need at least one searchable field to index on");
        }
    }

    Schema(Field primaryKeyField, String latitudeFieldName,
           String longitudeFieldName, Field... remainingField) {
        this(primaryKeyField, remainingField);
        addToFields(new Field(latitudeFieldName, Field.InternalType.LOCATION_LATITUDE,
                false, false, Field.DEFAULT_BOOST_VALUE));
        addToFields(new Field(longitudeFieldName,
                Field.InternalType.LOCATION_LONGITUDE, false, false,
                Field.DEFAULT_BOOST_VALUE));
        indexType = 1;
    }

    private void addToFields(Field f) {
        if (fields.contains(f)) {
            throw new IllegalArgumentException("duplicated field:" + f.name);
        }
        fields.add(f);
    }

    private String facetToXML() {

        StringBuilder facetNodeXML = new StringBuilder("		<facetEnabled>")
                .append(facetEnabled).append("</facetEnabled>\n");
        if (facetEnabled) {
            Iterator<Field> iter = fields.iterator();
            StringBuilder facetFieldsXML = new StringBuilder("");
            while (iter.hasNext()) {
                Field f = iter.next();
                if (f.facetEnabled) {
                    facetEnabled = true;
                    switch (f.facetType) {
                        case CATEGORICAL:
                            facetFieldsXML.append("			<facetField name=\"")
                                    .append(f.name)
                                    .append("\" facetType=\"categorical\"/>\n");
                            break;
                        case RANGE:
                        default:
                            facetFieldsXML.append("			<facetField name=\"")
                                    .append(f.name)
                                    .append("\" facetType=\"range\" facetStart=\"")
                                    .append(f.facetStart).append("\" facetEnd=\"")
                                    .append(f.facetEnd).append("\"")
                                    .append(" facetGap=\"").append(f.facetGap)
                                    .append("\"/>\n");
                    }
                }
            }
            facetNodeXML = facetNodeXML.append("		<facetFields>\n")
                    .append(facetFieldsXML).append("		</facetFields>\n");
        }

        return facetNodeXML.toString();
    }

    protected String schemaToXML() {

        StringBuilder schemaXML = new StringBuilder("	<schema>\n"
                + "		<fields>\n");
        Iterator<Field> iter = fields.iterator();
        while (iter.hasNext()) {
            schemaXML.append(Field.toXML(iter.next()));
        }
        schemaXML.append("		</fields>\n").append("		<uniqueKey>")
                .append(uniqueKey).append("</uniqueKey>\n");

        schemaXML
                .append(facetToXML())
                .append("		<types>\n")
                .append("		  <fieldType name=\"text_en\">\n")
                .append("			<analyzer>\n")
                .append("				<filter name=\"PorterStemFilter\" dictionary=\"\" />\n")
                .append("				<filter name=\"StopFilter\" words=\"stop-words.txt\" />\n")
                .append("			</analyzer>\n").append("		  </fieldType>\n")
                .append("		</types>\n").append("	</schema>\n");

        return schemaXML.toString();

    }
}
