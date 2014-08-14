package com.srch2.android.demo.helloworld;

import com.srch2.android.http.service.Field;
import com.srch2.android.http.service.Indexable;
import com.srch2.android.http.service.PrimaryKeyField;
import com.srch2.android.http.service.Schema;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class MovieIndex extends Indexable {

    // The name of the index. Used to identify the index when calling on the SRCH2Engine.
    public static final String INDEX_NAME = "movies";

    // The fields defining the schema of this index
    public static final String INDEX_FIELD_PRIMARY_KEY = "id";
    public static final String INDEX_FIELD_TITLE = "title";
    public static final String INDEX_FIELD_YEAR = "year";
    public static final String INDEX_FIELD_GENRE = "genre";

    @Override
    public String getIndexName() {
        // The name of the index this Indexable represents
        return INDEX_NAME;
    }

    @Override
    public Schema getSchema() {
        // Each index needs to be defined in terms of its schema: fields are comparable to the
        // the columns of an SQLite database table. A primary key field, whose value must be unique
        // for each record, is always required.
        PrimaryKeyField primaryKey = Field.createDefaultPrimaryKeyField(INDEX_FIELD_PRIMARY_KEY);
        Field title = Field.createSearchableField(INDEX_FIELD_TITLE, 3);
        Field year = Field.createRefiningField(INDEX_FIELD_YEAR, Field.Type.INTEGER);
        Field genre = Field.createSearchableField(INDEX_FIELD_GENRE);
        return Schema.createSchema(primaryKey, title, year, genre);
    }

    public static JSONArray getAFewRecordsToInsert() {
        // Records are inserted into an Indexable instance in the form of JSONObjects. For batch
        // insertion, insert the JSONObjects representing the records into a JSONArray.
        // Hopefully getting this record set will be retrieved from a server and not manually
        // entered by hand. Each JSONObject should contain as its keys only the fields
        // as defined in the schema and its values should match the type declared. A batch
        // insert performed with a JSONArray should only contain properly formed JSONObjects.

        JSONArray jsonRecordsToInsert = new JSONArray();
        try {
            JSONObject record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 1);
            record.put(INDEX_FIELD_TITLE, "The Good, the Bad And the Ugly");
            record.put(INDEX_FIELD_YEAR, 1966);
            record.put(INDEX_FIELD_GENRE, "Western Adventure");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 2);
            record.put(INDEX_FIELD_TITLE, "Citizen Kane");
            record.put(INDEX_FIELD_YEAR, 1941);
            record.put(INDEX_FIELD_GENRE, "Mystery Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 3);
            record.put(INDEX_FIELD_TITLE, "大红灯笼高高挂 (Raise the Red Lantern)");
            record.put(INDEX_FIELD_YEAR, 1991);
            record.put(INDEX_FIELD_GENRE, "Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 4);
            record.put(INDEX_FIELD_TITLE, "The Shawshank Redemption");
            record.put(INDEX_FIELD_YEAR, 1994);
            record.put(INDEX_FIELD_GENRE, "Crime Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 5);
            record.put(INDEX_FIELD_TITLE, "The Godfather");
            record.put(INDEX_FIELD_YEAR, 1972);
            record.put(INDEX_FIELD_GENRE, "Crime Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 6);
            record.put(INDEX_FIELD_TITLE, "The Dark Knight");
            record.put(INDEX_FIELD_YEAR, 2008);
            record.put(INDEX_FIELD_GENRE, "Action Drama Thriller");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 7);
            record.put(INDEX_FIELD_TITLE, "The Lord of the Rings: The Return of the King");
            record.put(INDEX_FIELD_YEAR, 2003);
            record.put(INDEX_FIELD_GENRE, "Action Adventure Fantasy");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 8);
            record.put(INDEX_FIELD_TITLE, "Fight Club");
            record.put(INDEX_FIELD_YEAR, 1999);
            record.put(INDEX_FIELD_GENRE, "Action Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 9);
            record.put(INDEX_FIELD_TITLE, "Inception");
            record.put(INDEX_FIELD_YEAR, 2010);
            record.put(INDEX_FIELD_GENRE, "Action Adventure Mystery");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 10);
            record.put(INDEX_FIELD_TITLE, "Forrest Gump");
            record.put(INDEX_FIELD_YEAR, 1994);
            record.put(INDEX_FIELD_GENRE, "Romance Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 11);
            record.put(INDEX_FIELD_TITLE, "One Flew Over the Cuckoo's Nest");
            record.put(INDEX_FIELD_YEAR, 1975);
            record.put(INDEX_FIELD_GENRE, "Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 12);
            record.put(INDEX_FIELD_TITLE, "The Lord of the Rings: The Two Towers ");
            record.put(INDEX_FIELD_YEAR, 2002);
            record.put(INDEX_FIELD_GENRE, "Action Adventure Fantasy");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 13);
            record.put(INDEX_FIELD_TITLE, "Goodfellas");
            record.put(INDEX_FIELD_YEAR, 1990);
            record.put(INDEX_FIELD_GENRE, "Biography Crime Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 14);
            record.put(INDEX_FIELD_TITLE, "The Matrix");
            record.put(INDEX_FIELD_YEAR, 1999);
            record.put(INDEX_FIELD_GENRE, "Science Fiction Action");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 15);
            record.put(INDEX_FIELD_TITLE, "七人の侍 (Seven Samurai: Shichinin no Samurai");
            record.put(INDEX_FIELD_YEAR, 1954);
            record.put(INDEX_FIELD_GENRE, "Action Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 16);
            record.put(INDEX_FIELD_TITLE, "Cidade de Deus (City of God)");
            record.put(INDEX_FIELD_YEAR, 2002);
            record.put(INDEX_FIELD_GENRE, "Crime Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 17);
            record.put(INDEX_FIELD_TITLE, "Casablanca");
            record.put(INDEX_FIELD_YEAR, 1942);
            record.put(INDEX_FIELD_GENRE, "War Romance Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 18);
            record.put(INDEX_FIELD_TITLE, "千と千尋の神隠し (Spirited Away: Sen to Chihiro no kamikakushi)");
            record.put(INDEX_FIELD_YEAR, 2001);
            record.put(INDEX_FIELD_GENRE, "Family Adventure Animation");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 19);
            record.put(INDEX_FIELD_TITLE, "Raiders of the Lost Ark");
            record.put(INDEX_FIELD_YEAR, 1981);
            record.put(INDEX_FIELD_GENRE, "Action Adventure");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 20);
            record.put(INDEX_FIELD_TITLE, "Memento");
            record.put(INDEX_FIELD_YEAR, 2000);
            record.put(INDEX_FIELD_GENRE, "Mystery Thriller");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 21);
            record.put(INDEX_FIELD_TITLE, "The Pianist");
            record.put(INDEX_FIELD_YEAR, 2002);
            record.put(INDEX_FIELD_GENRE, "Biography War Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 22);
            record.put(INDEX_FIELD_TITLE, "Apocalypse Now");
            record.put(INDEX_FIELD_YEAR, 1979);
            record.put(INDEX_FIELD_GENRE, "War Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 23);
            record.put(INDEX_FIELD_TITLE, "Back to the Future");
            record.put(INDEX_FIELD_YEAR, 1985);
            record.put(INDEX_FIELD_GENRE, "Science Fiction Adventure Comedy");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 24);
            record.put(INDEX_FIELD_TITLE, "Gladiator");
            record.put(INDEX_FIELD_YEAR, 2000);
            record.put(INDEX_FIELD_GENRE, "Action Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 25);
            record.put(INDEX_FIELD_TITLE, "Alien");
            record.put(INDEX_FIELD_YEAR, 1979);
            record.put(INDEX_FIELD_GENRE, "Science Fiction Horror");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 26);
            record.put(INDEX_FIELD_TITLE, "The Shining");
            record.put(INDEX_FIELD_YEAR, 1980);
            record.put(INDEX_FIELD_GENRE, "Horror");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 27);
            record.put(INDEX_FIELD_TITLE, "American Beauty");
            record.put(INDEX_FIELD_YEAR, 1999);
            record.put(INDEX_FIELD_GENRE, "Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 28);
            record.put(INDEX_FIELD_TITLE, "The Lion King");
            record.put(INDEX_FIELD_YEAR, 1994);
            record.put(INDEX_FIELD_GENRE, "Adventure Drama Animation");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 29);
            record.put(INDEX_FIELD_TITLE, "Le fabuleux destin d'Amélie Poulain (Amélie)");
            record.put(INDEX_FIELD_YEAR, 2001);
            record.put(INDEX_FIELD_GENRE, "Romance Comedy");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 30);
            record.put(INDEX_FIELD_TITLE, "WALL·E");
            record.put(INDEX_FIELD_YEAR, 2008);
            record.put(INDEX_FIELD_GENRE, "Family Adventure Animation");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 31);
            record.put(INDEX_FIELD_TITLE, "Das Boot");
            record.put(INDEX_FIELD_YEAR, 1981);
            record.put(INDEX_FIELD_GENRE, "War Adventure Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 32);
            record.put(INDEX_FIELD_TITLE, "A Clockwork Orange");
            record.put(INDEX_FIELD_YEAR, 1971);
            record.put(INDEX_FIELD_GENRE, "Crime Drama Science Fiction");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 33);
            record.put(INDEX_FIELD_TITLE, "Taxi Driver");
            record.put(INDEX_FIELD_YEAR, 1976);
            record.put(INDEX_FIELD_GENRE, "Crime Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 34);
            record.put(INDEX_FIELD_TITLE, "もののけ姫 (Princess Mononoke: Mononoke-hime)");
            record.put(INDEX_FIELD_YEAR, 1997);
            record.put(INDEX_FIELD_GENRE, "Fantasy Adventure Animation");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 35);
            record.put(INDEX_FIELD_TITLE, "Lawrence of Arabia");
            record.put(INDEX_FIELD_YEAR, 1962);
            record.put(INDEX_FIELD_GENRE, "Biography Adventure Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 36);
            record.put(INDEX_FIELD_TITLE, "Eternal Sunshine of the Spotless Mind");
            record.put(INDEX_FIELD_YEAR, 2004);
            record.put(INDEX_FIELD_GENRE, "Romance Drama Science Fiction");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 37);
            record.put(INDEX_FIELD_TITLE, "Singin' in the Rain");
            record.put(INDEX_FIELD_YEAR, 1952);
            record.put(INDEX_FIELD_GENRE, "Comedy Drama Musical");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 38);
            record.put(INDEX_FIELD_TITLE, "Full Metal Jacket");
            record.put(INDEX_FIELD_YEAR, 1987);
            record.put(INDEX_FIELD_GENRE, "War Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 39);
            record.put(INDEX_FIELD_TITLE, "Monty Python and the Holy Grail");
            record.put(INDEX_FIELD_YEAR, 1975);
            record.put(INDEX_FIELD_GENRE, "Fantasy Adventure Comedy");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 40);
            record.put(INDEX_FIELD_TITLE, "Amadeus");
            record.put(INDEX_FIELD_YEAR, 1984);
            record.put(INDEX_FIELD_GENRE, "Biography Music Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 41);
            record.put(INDEX_FIELD_TITLE, "Amadeus");
            record.put(INDEX_FIELD_YEAR, 1984);
            record.put(INDEX_FIELD_GENRE, "Biography Music Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 42);
            record.put(INDEX_FIELD_TITLE, "2001: A Space Odyssey");
            record.put(INDEX_FIELD_YEAR, 1968);
            record.put(INDEX_FIELD_GENRE, "Mystery Adventure Science Fiction");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 43);
            record.put(INDEX_FIELD_TITLE, "Batman Begins");
            record.put(INDEX_FIELD_YEAR, 2005);
            record.put(INDEX_FIELD_GENRE, "Action Thriller");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 44);
            record.put(INDEX_FIELD_TITLE, "用心棒 (Yôjinbô)");
            record.put(INDEX_FIELD_YEAR, 1961);
            record.put(INDEX_FIELD_GENRE, "Action Adventure Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 45);
            record.put(INDEX_FIELD_TITLE, "Metropolis");
            record.put(INDEX_FIELD_YEAR, 1927);
            record.put(INDEX_FIELD_GENRE, "Drama Science Fiction");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 46);
            record.put(INDEX_FIELD_TITLE, "Toy Story");
            record.put(INDEX_FIELD_YEAR, 1995);
            record.put(INDEX_FIELD_GENRE, "Adventure Comedy Animation");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 47);
            record.put(INDEX_FIELD_TITLE, "Chinatown");
            record.put(INDEX_FIELD_YEAR, 1974);
            record.put(INDEX_FIELD_GENRE, "Mystery Drama Thriller");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 48);
            record.put(INDEX_FIELD_TITLE, "Det sjunde inseglet (The Seventh Seal)");
            record.put(INDEX_FIELD_YEAR, 1957);
            record.put(INDEX_FIELD_GENRE, "Fantasy Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 49);
            record.put(INDEX_FIELD_TITLE, "Good Will Hunting");
            record.put(INDEX_FIELD_YEAR, 1997);
            record.put(INDEX_FIELD_GENRE, "Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 50);
            record.put(INDEX_FIELD_TITLE, "Blade Runner");
            record.put(INDEX_FIELD_YEAR, 1982);
            record.put(INDEX_FIELD_GENRE, "Science Fiction Drama Thriller");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 51);
            record.put(INDEX_FIELD_TITLE, "V for Vendetta");
            record.put(INDEX_FIELD_YEAR, 2005);
            record.put(INDEX_FIELD_GENRE, "Action Drama Speculative Fiction");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 52);
            record.put(INDEX_FIELD_TITLE, "Cool Hand Luke");
            record.put(INDEX_FIELD_YEAR, 1967);
            record.put(INDEX_FIELD_GENRE, "Crime Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 53);
            record.put(INDEX_FIELD_TITLE, "Fargo");
            record.put(INDEX_FIELD_YEAR, 1996);
            record.put(INDEX_FIELD_GENRE, "Crime Drama Thriller");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 54);
            record.put(INDEX_FIELD_TITLE, "How to Train Your Dragon");
            record.put(INDEX_FIELD_YEAR, 2010);
            record.put(INDEX_FIELD_GENRE, "Family Adventure Animation");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 55);
            record.put(INDEX_FIELD_TITLE, "Into the Wild");
            record.put(INDEX_FIELD_YEAR, 2007);
            record.put(INDEX_FIELD_GENRE, "Biography Adventure Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 56);
            record.put(INDEX_FIELD_TITLE, "No Country for Old Men");
            record.put(INDEX_FIELD_YEAR, 2007);
            record.put(INDEX_FIELD_GENRE, "Crime Drama Thriller");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 57);
            record.put(INDEX_FIELD_TITLE, "Annie Hall");
            record.put(INDEX_FIELD_YEAR, 1997);
            record.put(INDEX_FIELD_GENRE, "Romance Comedy Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 58);
            record.put(INDEX_FIELD_TITLE, "Finding Nemo");
            record.put(INDEX_FIELD_YEAR, 2003);
            record.put(INDEX_FIELD_GENRE, "Adventure Comedy Animation");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 59);
            record.put(INDEX_FIELD_TITLE, "Jaws");
            record.put(INDEX_FIELD_YEAR, 1975);
            record.put(INDEX_FIELD_GENRE, "Adventure Thriller");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 60);
            record.put(INDEX_FIELD_TITLE, "La strada (The Road)");
            record.put(INDEX_FIELD_YEAR, 1954);
            record.put(INDEX_FIELD_GENRE, "Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 61);
            record.put(INDEX_FIELD_TITLE, "La battaglia di Algeri (The Battle of Algiers)");
            record.put(INDEX_FIELD_YEAR, 1966);
            record.put(INDEX_FIELD_GENRE, "History Crime Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 62);
            record.put(INDEX_FIELD_TITLE, "The Graduate");
            record.put(INDEX_FIELD_YEAR, 1967);
            record.put(INDEX_FIELD_GENRE, "Romance Comedy Drama");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 63);
            record.put(INDEX_FIELD_TITLE, "Jurassic Park");
            record.put(INDEX_FIELD_YEAR, 1993);
            record.put(INDEX_FIELD_GENRE, "Adventure Science Fiction Thriller");
            jsonRecordsToInsert.put(record);

            record = new JSONObject();
            record.put(INDEX_FIELD_PRIMARY_KEY, 64);
            record.put(INDEX_FIELD_TITLE, "Beauty And the Beast");
            record.put(INDEX_FIELD_YEAR, 1991);
            record.put(INDEX_FIELD_GENRE, "Family Fantasy Animation");
            jsonRecordsToInsert.put(record);

        } catch (JSONException oops) {
            // We know there are no errors
        }
        return jsonRecordsToInsert;
    }
}