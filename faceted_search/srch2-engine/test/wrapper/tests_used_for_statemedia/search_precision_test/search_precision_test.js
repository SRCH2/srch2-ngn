
//a utility to calculute mean average precision for a freebase search

//change this to the test endpoint
// var host='http://10.182.2.214:8081/srch2'//sch-2
// var host='http://rack-s-sch-1:8081//srch2'//
//var host='http://rack-s-sch-2:8081//srch2'//
//var host='http://rack-p-sch-1:8081//srch2'//
//var host='http://rack-p-sch-2:8081//srch2'//
var host="http://localhost:8082"

//to run, type node searchtestfest.js...

var data=[{
    "name": "toronto",
    "id": "/m/0h7h6",
    "explanation": "flat-out search",
    "mongo" :   "4e342d158bbc402b7e000150"
}, {
    "name": "Maple syrup",
    "id": "/m/04_vv",
    "explanation": "flat-out search",
    "mongo" :   "f1a20b4ea5cdb070a60a6000"
}, {
    "name": "Maple syrup urine disease",
    "id": "/m/05xf_4",
    "explanation": "flat-out search"
}, {
    "name": "toronto maple leafs",
    "id": "/m/0j6tr",
    "explanation": "flat-out search",
    "mongo" :   "4efb28f68bbc4074a200004a"
}, {
    "name": "boston celtics",
    "id": "/m/0bwjj",
    "explanation": "flat-out search"
}, {
    "name": "canada",
    "id": "/m/0d060g",
    "explanation": "flat-out search",
    "mongo" :   "4e410adc8bbc406d6e000018"
}, {
    "name": "draft dodger",
    "id": "/m/02dmnj",
    "explanation": "flat-out search"
}, {
    "name": "detroit",
    "id": "/m/02dtg",
    "explanation": "flat-out search"
}, {
    "name": "Lymphoma",
    "id": "/m/04psf",
    "explanation": "flat-out search"
}, {
    "name": "canadian tire",
    "id": "/m/0184gy",
    "explanation": "flat-out search"
}, {
    "name": "walt whitman",
    "id": "/m/085gk",
    "explanation": "flat-out search"
}, {
    "name": "peter mansbridge",
    "id": "/m/01lghn",
    "explanation": "flat-out search",
    "mongo" :   "4f24640491464e5ca5000088"
}, {
    "name": "geneva",
    "id": "/m/03902",
    "explanation": "flat-out search"
}, {
    "name": "new hampshire",
    "id": "/m/059f4",
    "explanation": "flat-out search"
}, {
    "name": "radiohead",
    "id": "/m/09jm8",
    "explanation": "flat-out search",
    "mongo" :   "4e3402168bbc4021160001b9"
}, {
    "name": "thom yorke",
    "id": "/m/01p0w_",
    "explanation": "flat-out search",
    "mongo" :   "4e3a79e48bbc400a49000139"
}, {
    "name": "badminton",
    "id": "/m/0194d",
    "explanation": "flat-out search",
    "mongo" :   "4ee61939615fca2859000062"
}, {
    "name": "yugoslovia",
    "id": "/m/087vz",
    "explanation": "flat-out search"
}, {
    "name": "edmonton",
    "id": "/m/0nlh7",
    "explanation": "flat-out search"
}, {
    "name": "newt gingrich",
    "id": "/m/018fzs",
    "explanation": "flat-out search",
    "mongo" :   "4ed7eeeb615fca15c100001f"
}, {
    "name": "london",
    "id": "/m/04jpl",
    "explanation": "flat-out search",
    "mongo" :   "4e3426be8bbc402b7e0000af"
}, {
    "name": "miranda july",
    "id": "/m/06s5pl",
    "explanation": "flat-out search",
    "mongo" :   "4ea92a4a8bbc40178200005e"
}, {
    "name": "zooey deschanel",
    "id": "/m/03rl84",
    "explanation": "flat-out search",
    "mongo" :   "4f15ac3a47aa8a64e20000b7"
}, {
    "name": "Lauren Laverne",
    "id": "/m/01vt06h",
    "explanation": "flat-out search",
    "mongo" :   "4f1461c147aa8a3da700000f"
}, {
    "name": "bacteria",
    "id": "/m/017q2",
    "explanation": "flat-out search"
}, {
    "name": "Floyd Landis doping case",
    "id": "/m/051wc_f",
    "explanation": "flat-out search"
}, {
    "name": "doug gilmour",
    "id": "/m/01dg75",
    "explanation": "flat-out search",
    "mongo" :   "4f1829be47aa8a64e200017b"
}, {
    "name": "wikipedia",
    "id": "/m/0d07ph",
    "explanation": "flat-out search",
    "mongo" :   "4eb80fb5615fca5269000028"
}, {
    "name": "Mario Kart",
    "id": "/m/04zfy",
    "explanation": "flat-out search"
}, {
    "name": "Flubber",
    "id": "/m/04vcfx",
    "explanation": "flat-out search"
}, {
    "name": "lee harvey oswald",
    "id": "/m/04gwy",
    "explanation": "flat-out search"
}, {
    "name": "hmv",
    "id": "/m/012bcw",
    "explanation": "flat-out search"
}, {
    "name": "Ravi Shankar",
    "id": "/m/0pj8m",
    "explanation": "flat-out search"
}, {
    "name": "WrestleMania VI",
    "id": "/m/0526vm",
    "explanation": "flat-out search"
}, {
    "name": "steve reich",
    "id": "/m/06nkq",
    "explanation": "flat-out search"
}, {
    "name": "skydome",
    "id": "/m/0170x4",
    "explanation": "alias match"
}, {
    "name": "BBc",
    "id": "/m/0g5lhl7",
    "explanation": "alias match",
    "mongo" :   "4e3582cb8bbc401ec0000003"
}, {
    "name": "bbc",
    "id": "/m/0g5lhl7",
    "explanation": "alias match",
    "mongo" :   "4e3582cb8bbc401ec0000003"
}, {
    "name": "David Robert Jones",
    "id": "/m/01vsy7t",
    "explanation": "alias match",
    "mongo" :   "4e33ff1d8bbc402116000108"
}, {
    "name": "David Robert Hayward-Jones",
    "id": "/m/01vsy7t",
    "explanation": "alias match",
    "mongo" :   "4e33ff1d8bbc402116000108"
}, {
    "name": "t dot",
    "id": "/m/0h7h6",
    "explanation": "alias match",
    "mongo" :   "4e342d158bbc402b7e000150"
}, {
    "name": "Beyoncé",
    "id": "/m/01mpq7s",
    "explanation": "alias match",
    "mongo" :   "4e412c3e8bbc407204000114"
}, {
    "name": "farming",
    "id": "/m/0hkf",
    "explanation": "alias match",
    "mongo" :   "4f2bec5391464e734b0001dd"
}, {
    "name": "alghazali",
    "id": "/m/012prf",
    "explanation": "search fuzziness"
}, {
    "name": "golden doodle",
    "id": "/m/02fvym",
    "explanation": "search fuzziness",
    "mongo" :   "4e6f5fb98bbc401b9700001f"
}, {
    "name": "golden doodles",
    "id": "/m/02fvym",
    "explanation": "search fuzziness",
    "mongo" :   "4e6f5fb98bbc401b9700001f"
}, {
    "name": "takethat",
    "id": "/m/020gw1",
    "explanation": "search fuzziness"
}, {
    "name": "joseph and the technicolor dreamcoat",
    "id": "/m/01hs5bh",
    "explanation": "search fuzziness",
    "mongo" :   "4f182e6647aa8a64df000151"
}, {
    "name": "joseph and the technicolour dreamcoat",
    "id": "/m/01hs5bh",
    "explanation": "search fuzziness",
    "mongo" :   "4f182e6647aa8a64df000151"
}, {
    "name": "   waking  life  ",
    "id": "/m/01kg44",
    "explanation": "search fuzziness"
}, {
    "name": "waking life,",
    "id": "/m/01kg44",
    "explanation": "search fuzziness"
}, {
    "name": "Bunkyō",
    "id": "/m/01xhdy",
    "explanation": "search fuzziness"
}, {
    "name": "Bunkyo uni",
    "id": "/m/03lq95",
    "explanation": "search fuzziness"
}, {
    "name": "firefighter",
    "id": "/m/012n4x",
    "explanation": "search fuzziness"
}, {
    "name": "chinese room",
    "id": "/m/01vjl",
    "explanation": "search fuzziness"
}, {
    "name": "cuba",
    "id": "/m/0d04z6",
    "explanation": "search fuzziness"
}, {
    "name": "Miranda,  Richardson",
    "id": "/m/0525b",
    "explanation": "search fuzziness"
}, {
    "name": " Richardson, Miranda",
    "id": "/m/0525b",
    "explanation": "search fuzziness"
}, {
    "name": "dr. phil season 3",
    "id": "/m/0gg12mt",
    "explanation": "fuzzy & punctuation"
}, {
    "name": "24 (season 4)",
    "id": "/m/0b1ltn",
    "explanation": "fuzzy & punctuation"
}, {
    "name": "24 season 4",
    "id": "/m/0b1ltn",
    "explanation": "fuzzy & punctuation"
}, {
    "name": "September 11 attack",
    "id": "/m/0d0vp3",
    "explanation": "loose alias match"
}, {
    "name": "world war 2",
    "id": "/m/081pw",
    "explanation": "loose alias match",
    "mongo" :   "4f27d50847aa8a1131000023"
}, {
    "name": "Second World War",
    "id": "/m/081pw",
    "explanation": "loose alias match",
    "mongo" :   "4f27d50847aa8a1131000023"
}, {
    "name": "steven reich",
    "id": "/m/06nkq",
    "explanation": "fuzzy match"
}, {
    "name": "rogers center",
    "id": "/m/0170x4",
    "explanation": "fuzzy match"
}, {
    "name": "beyonce",
    "id": "/m/01mpq7s",
    "explanation": "loose alias match",
    "mongo" :   "4e412c3e8bbc407204000114"
}, {
    "name": "lauren lavern",
    "id": "/m/01vt06h",
    "explanation": "loose alias match",
    "mongo" :   "4f1461c147aa8a3da700000f"
}, {
    "name": "LAUREN LAVERNE",
    "id": "/m/01vt06h",
    "explanation": "loose alias match",
    "mongo" :   "4f1461c147aa8a3da700000f"
}, {
    "name": "milk scandal",
    "id": "/m/04n6hxj",
    "explanation": "loose alias match"
}, {
    "name": "The Artist Formerly Known as Prince",
    "id": "/m/01vvycq",
    "explanation": "loose alias match",
    "mongo" :   "4e33ff778bbc40211600012c"
}, {
    "name": "Unpronouncable Symbol",
    "id": "/m/01vvycq",
    "explanation": "loose alias match",
    "mongo" :   "4e33ff778bbc40211600012c"
}, {
    "name": "dave walter",
    "id": "/m/05yf2n1",
    "explanation": "obscure topic match"
}, {
    "name": "wendy cobain",
    "id": "/m/059x_9n",
    "explanation": "obscure topic match"
}, {
    "name": "Väinö Raitio",
    "id": "/m/0c81sv",
    "explanation": "punctuation"
}, {
    "name": "vaino railtio",
    "id": "/m/0c81sv",
    "explanation": "punctuation"
}, {
    "name": "Jōmon Sugi",
    "id": "/m/04jhzqc",
    "explanation": "punctuation"
}, {
    "name": "People's Choice Awards",
    "id": "/m/02qsr3",
    "explanation": "punctuation"
}, {
    "name": "Peoples Choice Awards",
    "id": "/m/02qsr3",
    "explanation": "punctuation"
}, {
    "name": "Super Bowl",
    "id": "/m/06x5s",
    "explanation": "tight match"
}, {
    "name": "lymphoma",
    "id": "/m/0jdk0",
    "explanation": "tight match"
}, {
    "name": "walt disney",
    "id": "/m/081nh",
    "explanation": "tight match"
}, {
    "name": "united kingdom",
    "id": "/m/07ssc",
    "explanation": "tight match",
    "mongo" :   "4e345c128bbc402f9b000047"
}, {
    "name": "prince",
    "id": "/m/01vvycq",
    "explanation": "tight match",
    "mongo" :   "4e33ff778bbc40211600012c"
}, {
    "name": "nico",
    "id": "/m/01vsqvs",
    "explanation": "tight match"
}, {
    "name": "rain",
    "id": "/m/06mb1",
    "explanation": "busted relevance",
    "mongo" :   "caca0b4eceafb45b84310000"
}, {
    "name": "thunderstorm",
    "id": "/m/0jb2l",
    "explanation": "busted relevance"
}, {
    "name": "friday",
    "id": "/m/0f7z4",
    "explanation": "busted relevance"
}, {
    "name": "vacation",
    "id": "/m/02jwqh",
    "explanation": "busted relevance"
}, {
    "name": "pizza",
    "id": "/m/01z1c1t",
    "explanation": "busted relevance",
    "mongo" :   "4f295c6791464e73510000c5"
}, {
    "name": "airplane",
    "id": "/m/0cmf2",
    "explanation": "busted relevance"
}, {
    "name": "winter",
    "id": "/m/086mh",
    "explanation": "busted relevance"
}, {
    "name": "spring",
    "id": "/m/01txr2",
    "explanation": "busted relevance"
}, {
    "name": "car",
    "id": "/m/0k4j",
    "explanation": "busted relevance",
    "mongo" :   "4e78b77f8bbc40444700007c"
}, {
    "name": "sky",
    "id": "/m/01bqvp",
    "explanation": "busted relevance"
}, {
    "name": "book",
    "id": "/m/0bt_c3",
    "explanation": "busted relevance"
}, {
    "name": "sunset",
    "id": "/m/01b2w5",
    "explanation": "busted relevance"
}, {
    "name": "sunrise",
    "id": "/m/01b2q6",
    "explanation": "busted relevance"
}, {
    "name": "sunday",
    "id": "/m/0f6ld",
    "explanation": "busted relevance"
}, {
    "name": "night",
    "id": "/m/01d74z",
    "explanation": "busted relevance"
}, {
    "name": "DreamWorks Animation",
    "id": "/m/056ws9",
    "explanation": "straightforward",
}, {
    "name": "time warner",
    "id": "/m/0l8sx",
    "explanation": "straightforward",
}, {
    "name": "lions gate films",
    "id": " /m/03xsby",
    "explanation": "straightforward",
}, {
    "name": "Impeachment of Bill Clinton",
    "id": "/m/04qs58",
    "explanation": "straightforward",
    "mongo" :   "4ef3280b8bbc4026c400005f"
}, {
    "name": "monica lewinksy",
    "id": "/m/0509p",
    "explanation": "straightforward",
}, {
    "name": "brandy norwood",
    "id": "/m/03f3yfj",
    "explanation": "straightforward",
}, {
    "name": "brandy norwoodj",
    "id": "/m/03f3yfj",
    "explanation": "extra characters",
}, {
    "name": "captain's curse",
    "id": "/m/05f4hrt",
    "explanation": "straightforward",
}, {
    "name": "captain  curse",
    "id": "/m/05f4hrt",
    "explanation": "loose",
}, {
    "name": "vice",
    "id": "/m/034vfh",
    "explanation": "short match",
}, {
    "name": "old school",
    "id": "/m/03vv1f",
    "explanation": "short match",
    "mongo" :   "4f303ba147aa8a698300004f"
}, {
    "name": "tornado",
    "id": "/m/09c5_",
    "explanation": "short match",
}, {
    "name": "mardi gras",
    "id": "/m/09l8z",
    "explanation": "straightforward",
}, {
    "name": "tornado alley",
    "id": "/m/03msfz",
    "explanation": "straightforward",
}, {
    "name": "beaker street",
    "id": "/m/07dy7y",
    "explanation": "straightforward",
}, {
    "name": "beaker street with clyde",
    "id": "/m/07dy7y",
    "explanation": "loose match",
}, {
    "name": "Fantastic Mr. Fox",
    "id": "/m/08s6mr",
    "explanation": "straightforward",
    "mongo" :   "49cc0b4e3591b73c8f490000"
}, {
    "name": "lampoons christmas",
    "id": "/m/048f00",
    "explanation": "loose match",
    "mongo" :   "4ef1ba9f8bbc40181f0001c7"
}, {
    "name": "lampoons christmas movie",
    "id": "/m/048f00",
    "explanation": "extra characters",
    "mongo" :   "4ef1ba9f8bbc40181f0001c7"
}, {
    "name": "cinemark",
    "id": "/m/0624bc",
    "explanation": "straightforward",
}, {
    "name": "cc media",
    "id": "/m/018p5f",
    "explanation": "straightforward",
}, {
    "name": "clear channel",
    "id": " /m/018p5f",
    "explanation": "straightforward",
}, {
    "name": "Russell 'Rusty' Griswold",
    "id": "/m/0h6hhld",
    "explanation": "punctuation",
}, {
    "name": "Rusty Griswold",
    "id": "/m/0h6hhld",
    "explanation": "partial match",
}, {
    "name": "Russell Griswold",
    "id": "/m/0h6hhld",
    "explanation": "partial match",
}, {
    "name": "captain tanida",
    "id": "/m/0gkf3rp",
    "explanation": "straightforward",
}, {
    "name": "franz ferdinand",
    "id": "/m/02ft60",
    "explanation": "straightforward",
    "mongo" :   "4f4b8c6391464e17ac000121"
}, {
    "name": "fran ferdinad",
    "id": "/m/02ft60",
    "explanation": "partial match",
    "mongo" :   "4f4b8c6391464e17ac000121"
}, {
    "name": "University of Maryland School of Law",
    "id": "/m/0466h4",
    "explanation": "straightforward",
    "mongo" :   "4f4f8c6591464e263e0000fb"
}, {
    "name": "bush x",
    "id": "/m/01727v",
    "explanation": "straightforward",
}, {
    "name": "nazism",
    "id": "/m/05hyf",
    "explanation": "straightforward",
    "mongo" :   "87a20b4e7d297f7272006000"
}, {
    "name": "jon stewart",
    "id": "/m/01j7rd",
    "explanation": "straightforward",
    "mongo" :   "4e67c4218bbc407a7d00000f"
}, {
    "name": "Entourage - Season 2",
    "id": "/m/02211dx",
    "explanation": "punctuation",
}, {
    "name": "J.J. Cale ",
    "id": "/m/01wk5_r",
    "explanation": "extra punctuation",
    "mongo" :   "4f4d4f1d91464e2641000072"
}, {
    "name": "Jj Cale ",
    "id": "/m/01wk5_r",
    "explanation": "straightforward",
    "mongo" :   "4f4d4f1d91464e2641000072"
}, {
    "name": "Jj Cale musician",
    "id": "/m/01wk5_r",
    "explanation": "extra characters",
    "mongo" :   "4f4d4f1d91464e2641000072"
}, {
    "name": "baltimore",
    "id": "/m/094jv",
    "explanation": "straightforward",
}, {
    "name": "creationism",
    "id": "/m/01mm1",
    "explanation": "straightforward",
    "mongo" :   "4e3566f88bbc4018dc000137"
}, {
    "name": "ann deag ",
    "id": "/m/05w9z9m",
    "explanation": "partial match",
}, {
    "name": "There is no balm in Birmingham",
    "id": "/m/06lk0mm",
    "explanation": "straightforward",
}, {
    "name": "peta ",
    "id": "/m/0gj8r",
    "explanation": "alias",
    "mongo" :   "4f3b8d7847aa8a24be00009a"
}, {
    "name": "people of the ethical treatment",
    "id": "/m/0gj8r",
    "explanation": "straightforward",
    "mongo" :   "4f3b8d7847aa8a24be00009a"
}, {
    "name": "P.E.T.A. ",
    "id": "/m/0gj8r",
    "explanation": "extra punctuation",
    "mongo" :   "4f3b8d7847aa8a24be00009a"
}, {
    "name": "Quinta Dju ",
    "id": "/m/0bv_f5v",
    "explanation": "straightforward",
}, {
    "name": "Djurgården/Älvsjö",
    "id": "/m/047yn6",
    "explanation": "unicode",
}, {
    "name": "Djurgården Älvsjö",
    "id": "/m/047yn6",
    "explanation": "loose and punctuation",
}, {
    "name": "Djurgarden/alvsjo",
    "id": "/m/047yn6",
    "explanation": "swap unicode to ascii",
}, {
    "name": "DJ Tiësto",
    "id": "/m/01w334h",
    "explanation": "unicode",
}, {
    "name": "DJ Tiesto",
    "id": "/m/01w334h",
    "explanation": "swap unicode to ascii",
}, {
    "name": "farming",
    "id": "/m/0hkf",
    "explanation": "alias",
    "mongo" :   "4f2bec5391464e734b0001dd"
}, {
    "id": "/m/027hnc9",
    "name": "          beach house",
    "explanation": "perfect"
}, {
    "id": "/m/0n7zx",
    "name": " Leicester Square",
    "explanation": "perfect"
}, {
    "id": "/m/0n7zx",
    "name": "lester square",
    "explanation": "loose"
}, {
    "id": "/m/0n7zx",
    "name": " Leicester Square london",
    "explanation": "extra"
}, {
    "id": "/m/04kmf_w",
    "name": "ellerdale",
    "explanation": "perfect"
}, {
    "id": "/m/04kmf_w",
    "name": "ellderdale",
    "explanation": "loose"
}, {
    "id": "/m/04kmf_w",
    "name": "ellerdale website",
    "explanation": "extra"
}, {
    "id": "/m/01hkyyt",
    "name": "face to face",
    "explanation": "perfect"
}, {
    "id": "/m/0bgpcy",
    "name": "Trever Keith",
    "explanation": "perfect"
}, {
    "id": "/m/026xgrh",
    "name": "South Quay bombing",
    "explanation": "perfect"
}, {
    "id": "/m/026xgrh",
    "name": "canary warf ira",
    "explanation": "new alias"
}, {
    "id": "/m/019bwp",
    "name": "canary war",
    "explanation": "incomplete"
}, {
    "id": "/m/019bwp",
    "name": "london canary warf",
    "explanation": "loose alias"
}, {
    "id": "/m/019bwp",
    "name": "canary warf west end london",
    "explanation": "extra"
}, {
    "id": "/m/019bwp",
    "name": "canary warf business district",
    "explanation": "extra"
}, {
    "id": "/m/0nlg4",
    "name": "tower hamlets",
    "explanation": "alias"
}, {
    "id": "/m/0nlg4",
    "name": "Tower Hamlets in london!",
    "explanation": "loose"
}, {
    "id": "/m/027sqmg",
    "name": "barrie advance",
    "explanation": ""
}, {
    "id": "/m/027sqmg",
    "name": "barrie newspaper advance",
    "explanation": "extra"
}, {
    "id": "/m/027sqmg",
    "name": "barrie advance in ontario",
    "explanation": "extra"
}, {
    "id": "/m/018n0s",
    "name": "orillia",
    "explanation": "perfect"
}, {
    "id": "/m/018n0s",
    "name": "orillia ontario",
    "explanation": "alias"
}, {
    "id": "/m/03wg41",
    "name": "Lake Couchiching",
    "explanation": "perfect"
}, {
    "id": "/m/03wg41",
    "name": "couchiching lake",
    "explanation": "loose"
}, {
    "id": "/m/03wg41",
    "name": "couchaching lake",
    "explanation": "loose"
}, {
    "id": "/m/0hzycw2",
    "name": "big chief island",
    "explanation": "perfect"
}, {
    "id": "/m/04n7dt5",
    "name": "Anishinaabe language",
    "explanation": "wp redirect"
}, {
    "id": "/m/04n7dt5",
    "name": "Ojibwe language",
    "explanation": "perfect"
}, {
    "id": "/m/04n7dt5",
    "name": "ojibwe",
    "explanation": "partial"
}, {
    "id": "/m/04n7dt5",
    "name": "Chippewa speakers",
    "explanation": "extra"
}, {
    "id": "/m/0dq3t",
    "name": "Algonquian languages",
    "explanation": "perfect"
}, {
    "id": "/m/0dq3t",
    "name": "Algonquian languages of north america",
    "explanation": "extra"
}, {
    "id": "/m/04dm2p_",
    "name": "opencyc",
    "explanation": "perfect"
}, {
    "id": "/m/04dm2p_",
    "name": "open cyc",
    "explanation": "loose"
}, {
    "id": "/m/0gw0",
    "name": "anarchism",
    "explanation": "perfect"
}, {
    "id": "/m/0lyrt",
    "name": "Ejército Zapatista de Liberación Nacional",
    "explanation": "alias"
}, {
    "id": "/m/0lyrt",
    "name": "Zapatista Army",
    "explanation": "partial"
}, {
    "id": "/m/09qhs2",
    "name": "Himno Zapatista",
    "explanation": "perfect"
}, {
    "id": "/m/09qhs2",
    "name": "zapatista music",
    "explanation": "description match"
}, {
    "id": "/m/01qf_4",
    "name": "georgian bay",
    "explanation": "perfect"
}, {
    "id": "/m/09r57",
    "name": "Samuel de Champlain",
    "explanation": "perfect"
}, {
    "id": "/m/09r57",
    "name": "Champlain, Samuel",
    "explanation": "loose"
}, {
    "id": "/m/09r57",
    "name": "The Father of New France",
    "explanation": "description"
}, {
    "id": "/m/06829d",
    "name": "Saintonge, france",
    "explanation": "extra"
}, {
    "id": "/m/06829d",
    "name": "Saintonge",
    "explanation": "perfect"
}, {
    "id": "/m/0d1_ky",
    "name": "Saintonge War",
    "explanation": "perfect"
}, {
    "id": "/m/05py0",
    "name": "octopus",
    "explanation": "perfect"
}, {
    "id": "/m/0gsstt",
    "name": "Incirrina",
    "explanation": "perfect"
}, {
    "id": "/m/03fbc",
    "name": "gorillaz",
    "explanation": "perfect"
}, {
    "id": "/m/03fbc",
    "name": "the gorillaz",
    "explanation": "extra"
}, {
    "id": "/m/0phx4",
    "name": "Albarn",
    "explanation": "partial"
}, {
    "id": "/m/0ncx1",
    "name": "whitechapel",
    "explanation": "perfect"
}, {
    "id": "/m/07kxjj",
    "name": "Hit List",
    "explanation": "perfect"
}, {
    "id": "/m/0h3r1",
    "name": "formaldehyde",
    "explanation": "perfect"
}, {
    "id": "/m/0h3r1",
    "name": "CH2O",
    "explanation": "description"
}, {
    "id": "/m/0h3r1",
    "name": "formeldehide",
    "explanation": "loose-phonetic"
}, {
    "id": "/m/0d1_ky",
    "name": "saintoj war",
    "explanation": "loose-phonetic"
}, {
    "id": "/m/01mv4m",
    "name": "caucus",
    "explanation": "perfect"
}, {
    "id": "/m/05kskw",
    "name": "Ontario Highway 69",
    "explanation": "description"
}, {
    "id": "/m/0ftkx",
    "name": "taipai",
    "explanation": "loose-phonetic"
}, {
    "id": "/m/0ftkx",
    "name": "Taipei City",
    "explanation": "extra-description"
}, {
    "id": "/m/0ftkx",
    "name": "Táiběi Shì",
    "explanation": "punctuation"
}, {
    "id": "/m/0ftkx",
    "name": "Tai Peh",
    "explanation": "partial-alias"
}, {
    "id": "/m/03tj1c",
    "name": "fu jen",
    "explanation": "partial"
}, {
    "id": "/m/03tj1c",
    "name": "fu jen university",
    "explanation": "partial"
}, {
    "id": "/m/03tj1c",
    "name": "fu jen taipei",
    "explanation": "partial alias"
}, {
    "id": "/m/015f2x",
    "name": "singapore air",
    "explanation": "partial"
}, {
    "id": "/m/015f2x",
    "name": "Syarikat Penerbangan",
    "explanation": "description"
}, {
    "id": "/m/015f2x",
    "name": "新加坡航空公司",
    "explanation": "description-punctuation"
}, {
    "id": "/m/01gt7",
    "name": "747",
    "explanation": "partial"
}, {
    "id": "/m/01gt7",
    "name": "Jumbo Jet",
    "explanation": "alias"
}, {
    "id": "/m/05kr4",
    "name": "where art thou",
    "explanation": "partial"
}, {
    "id": "/m/05kr4",
    "name": "Art Thou?",
    "explanation": "partial"
}, {
    "id": "/m/05kr4",
    "name": "where art thou movie",
    "explanation": "partial-extra"
}, {
    "id": "/m/014zcr",
    "name": "clooney",
    "explanation": "partial"
}, {
    "id": "/m/014zcr",
    "name": "Timothy Clooney",
    "explanation": "partial alias"
}, {
    "id": "/m/014zcr",
    "name": "tim clooney",
    "explanation": "partial alias"
}, {
    "id": "/m/0gx9s4k",
    "name": "nina warren",
    "explanation": "obscure topic"
}, {
    "id": "/m/0gx9s4k",
    "name": "Nina Bruce",
    "explanation": "obscure alias"
}, {
    "id": "/m/0brz326",
    "name": "Monck's Landing",
    "explanation": "partial"
}, {
    "id": "/m/0brz326",
    "name": "monck's in Norland",
    "explanation": "description"
}, {
    "id": "/m/05kr_",
    "name": "ontari",
    "explanation": "partial"
}, {
    "id": "/m/05kr_",
    "name": "Province D' Ontario",
    "explanation": "alias"
}, {
    "id": "/m/018dmn",
    "name": "Cambridge, Ontario",
    "explanation": "alias"
}, {
    "id": "/m/018dmn",
    "name": "cambridge in canada",
    "explanation": "loose alias"
}, {
    "id": "/m/0ddc3xk",
    "name": "Capture of Cambridge",
    "explanation": "perfect"
}, {
    "id": "/m/02xmdg",
    "name": "oro",
    "explanation": "partial"
}, {
    "id": "/m/02xmdg",
    "name": "oro-medonte",
    "explanation": "punctuation"
}, {
    "id": "/m/02xmdg",
    "name": "oro ontario",
    "explanation": "alias"
}, {
    "id": "/m/02x51dw",
    "name": "life of Edward II of England",
    "explanation": "partial"
}, {
    "id": "/m/02x51dw",
    "name": "life of edward II play",
    "explanation": "extra"
}, {
    "id": "/m/02x51dw",
    "name": "Leben Eduards des Zweiten ",
    "explanation": "description"
}, {
    "id": "/m/02rp7f3",
    "name": "St. Louis Moonstone",
    "explanation": "partial"
}, {
    "id": "/m/032g4c",
    "name": "Norland Ontario",
    "explanation": "loose"
}, {
    "id": "/m/01d8wq",
    "name": "Southend-on-Sea",
    "explanation": "punctuation"
}, {
    "id": "/m/04ztj",
    "name": "marriage",
    "explanation": "perfect"
}, {
    "id": "/m/02679s",
    "name": "factory farming",
    "explanation": "perfect"
}, {
    "id": "/m/02qx33m",
    "name": "industrial farming",
    "explanation": "description"
}, {
    "id": "/m/015lz1",
    "name": "singing",
    "explanation": "perfect"
}, {
    "id": "/m/04bwjc",
    "name": "Speech pathology",
    "explanation": "perfect"
}, {
    "id": "/m/04h9z7z",
    "name": "Pat Hewitt",
    "explanation": "perfect"
}, {
    "id": "/m/04h9z7z",
    "name": "Patricia Shipp",
    "explanation": "alias"
}, {
    "id": "/m/041z1gp",
    "name": "wait don't tell me",
    "explanation": "partial"
}, {
    "id": "/m/02z4d2",
    "name": "Chicago Public Radio",
    "explanation": "perfect"
}, {
    "id": "/m/0gxz85q",
    "name": "Mnjikaning Fish Weirs",
    "explanation": "perfect"
}, {
    "id": "/m/0hk2p",
    "name": "Cardiopulmonary resuscitation",
    "explanation": "perfect"
}, {
    "id": "/m/02z4d2",
    "name": "wbez",
    "explanation": "description"
}, {
    "id": "/m/0hk2p",
    "name": "cpr",
    "explanation": "alias"
}, {
    "id": "/m/01hfbzz",
    "name": "honky chateau",
    "explanation": "perfect"
}, {
    "id": "/m/01hfbzz",
    "name": "Honky Château",
    "explanation": "punctuation alias"
}, {
    "id": "/m/0c0sl",
    "name": "npr",
    "explanation": "alias"
}, {
    "id": "/m/0c0sl",
    "name": "npr radio",
    "explanation": "alias-extra"
}, {
    "id": "/m/03gfw_m",
    "name": "WCAI/WNAN talk",
    "explanation": "partial"
}, {
    "id": "/m/07dn1",
    "name": "talk radio",
    "explanation": "perfect"
}, {
    "id": "/m/07d6dr",
    "name": "cfuv",
    "explanation": "partial"
}, {
    "id": "/m/07d6dr",
    "name": "cfuv campus radio",
    "explanation": "description"
}, {
    "id": "/m/017cy9",
    "name": "ubc",
    "explanation": "alias"
}, {
    "id": "/m/09jw2",
    "name": "glam rock",
    "explanation": "perfect"
}, {
    "id": "/m/06tml5",
    "name": "Nu-Pagadi",
    "explanation": "loose punctuation"
}, {
    "id": "/m/017cy9",
    "name": "ubc university",
    "explanation": "description"
}, {
    "id": "/m/04ck0n",
    "name": "Jesusland map",
    "explanation": "perfect"
}, {
    "id": "/m/01clyr",
    "name": "Polydor Records",
    "explanation": "perfect"
}, {
    "id": "/m/01243b",
    "name": "Indie rock",
    "explanation": "perfect"
}, {
    "id": "/m/01243b",
    "name": "indy rock",
    "explanation": "phonetic"
}, {
    "id": "/m/03tj1c",
    "name": "foo jen",
    "explanation": "phonetic"
}, {
    "id": "/m/03cssn9",
    "name": "Esther Shiner stadium",
    "explanation": "perfect"
}, {
    "id": "/m/0dplt5",
    "name": "esther shiner",
    "explanation": "perfect"
}, {
    "id": "/m/03cssn9",
    "name": "North York shiner stadium",
    "explanation": "loose alias"
}, {
    "id": "/m/03cssn9",
    "name": "ethershiner stadium",
    "explanation": "loose"
}, {
    "id": "/m/018ysx",
    "name": "Psychobilly",
    "explanation": "perfect"
}, {
    "id": "/m/025s_ht",
    "name": "heart transplant",
    "explanation": "loose"
}, {
    "id": "/m/025s_ht",
    "name": "cardiac transplant",
    "explanation": "loose alias"
}, {
    "id": "/m/018ysx",
    "name": "psycho-bily",
    "explanation": "loose"
}, {
    "id": "/m/0b_4l",
    "name": "organ donors",
    "explanation": "alias"
}, {
    "id": "/m/0grwj",
    "name": "oprah",
    "explanation": "alias"
}, {
    "id": "/m/0grwj",
    "name": "opra winfry",
    "explanation": "phonetic"
}, {
    "id": "/m/0xgb1",
    "name": "Kosciusko",
    "explanation": "perfect"
}, {
    "id": "/m/0xgb1",
    "name": "kosiusk",
    "explanation": "partial-loose"
}, {
    "id": "/m/0c2rh3",
    "name": "crazy on you",
    "explanation": "perfect"
}, {
    "id": "/m/0c2rh3",
    "name": "crazy on you song",
    "explanation": "extra"
}, {
    "id": "/m/011yhm",
    "name": "fargo movie",
    "explanation": "extra"
}, {
    "id": "/m/023kzp",
    "name": "William H. Macy",
    "explanation": "perfect"
}, {
    "id": "/m/04mn0r7",
    "name": "Carl Showalter",
    "explanation": "perfect"
}, {
    "id": "/m/0csm9",
    "name": "daylight savings",
    "explanation": "partial-with-noise"
}, {
    "id": "/m/035rfz",
    "name": "soviet space",
    "explanation": "partial"
}, {
    "id": "/m/02y2fn",
    "name": "emerica",
    "explanation": "perfect-with-phonetic noise"
}, {
    "id": "/m/06g7xj",
    "name": "Andrew Reynolds",
    "explanation": "perfect"
}, {
    "id": "/m/02j_4",
    "name": "easter",
    "explanation": "perfect"
}, {
    "id": "/m/0c2ng",
    "name": "Steven Pinker",
    "explanation": "perfect"
}, {
    "id": "/m/018gq3",
    "name": "101 Dalmatians",
    "explanation": "perfect"
}, {
    "id": "/m/02rjc05",
    "name": "Dalmatian",
    "explanation": "perfect"
}, {
    "id": "/m/02rjc05",
    "name": " Dalmatian dog",
    "explanation": "extra"
}, {
    "id": "/m/02rjc05",
    "name": "Dalmatians",
    "explanation": "plural"
}, {
    "id": "/m/01vq7_",
    "name": "iguanas",
    "explanation": "plural"
}, {
    "id": "/m/01jp58",
    "name": "plumbers",
    "explanation": "plural"
}, {
    "id": "/m/05bzrs",
    "name": "coffee beans",
    "explanation": "plural"
}, {
    "id": "/m/044gvx",
    "name": "beer bottles",
    "explanation": "plural"
}, {
    "id": "/m/9696",
    "name": "wheat",
    "explanation": "perfect"
}, {
    "id": "/m/02vmndb",
    "name": "gluten allergy",
    "explanation": "description"
}, {
    "id": "/m/02vmndb",
    "name": "gluten allergies",
    "explanation": "plural"
}, {
    "id": "/m/02vmndb",
    "name": "gluten intolerance",
    "explanation": "description"
}]





var request = require("../node_modules/request");
var _ = require("../node_modules/underscore");


var pass=0;
var fail=0;
var inside=0;
var notin=0;
var fails={};
var scores=[];

//setup type-counts
var types={};
for(var i in data){
    if(types[data[i].explanation]){
        types[data[i].explanation]++;
    }else{
        types[data[i].explanation]=1;
    }
}


//talk to srch2's search interface
function srch2(query) {
  query=query.replace(/^\s+|\s+$/g, '')
  query=query.replace(/  /g, ' ')
  //console.log(query)
  var simboost='';
  var termtypes='';
  _(query.split(' ').length).times(function(){ simboost+='0.9+'; termtypes+='0+' });
  simboost=simboost.replace(/\+$/,'')
  termtypes=termtypes.replace(/\+$/,'')
  //var url=  host+'/search?q='+encodeURI( query.replace(/ /g,'+') )+'&fuzzy=1&type=0&start=0&limit=10&termtypes='+termtypes+'&simboost='+simboost;
  var url=  host+'/search?fuzzy=1&limit=10&q='+encodeURI( query.replace(/ /g,'+') );
// console.log(url)
  return url;
}


function callit(query, callback){
  var url=srch2(query);
  request({
    uri:url,
  }, function(error, response, body) {
       try{
         return callback(JSON.parse(body));
       }catch(e){
         console.log('-----srch2 kickback on '+query+'----'+e);
         return callback({results:[]})
       }

 });
}

function testit(query, id, mongo, type, i){

  callit(query, function(result){


      //test if its the first result
        if(result.results[0] && (result.results[0].record._id==id || result.results[0].record._id==mongo ) ){
     //     console.log(query + '\t     pass')
          pass++;
          inside++;
          scores.push(1);
        }
        else{
         fail++;
                  //test if its within top 10
          var found=false;
          for(var o in result.results){
            if(result.results[o] && (result.results[o].record._id==id || result.results[o].record._id==mongo)) {
              found=true;
              inside++;
              var c=parseInt(o)+1;
      //        console.log(query + '\t   -  (result #'+c+')   ('+ type+")");
              var x=1/c;
              scores.push(x)
              break;
            }
          }
          if(!found){
               notin++;
     //          console.log(query + '\t  -    ****fail***      ('+ type+")        ******** ")
               if(fails[type]){
                fails[type]++;
               }else{
                fails[type]=1;
               }
               scores.push(0)
             }
        }

        //proceed
        i=parseInt(i);
        i++
        if(data[i]){
          testit(data[i].name ,  data[i].id, data[i].mongo,   data[i].explanation, i)
        }
        else{
          console.log('done')
          console.log('---------------------------------------------------')
          console.log('using first result = '+pass+'  pass,  '+fail+' fail.     '+parseInt((pass/data.length)*100)+' %  ');
          console.log('using all results = '+inside+'  pass,  '+notin+' fail.     '+parseInt((inside/data.length)*100)+' %  ');
          var meanprecision=mean_average_precision(scores);
          console.log('mean average precision - '+meanprecision)
          console.log('==============================')
          console.log('-------- Test Passes ---------')
          console.log('==============================')
          //show failure types
          fails=to_arr(fails);
          for(var o in fails){
     //       console.log(fails[o].key+" errors:   "+fails[o].count+"   (out of "+ types[fails[o].key]+")" )
          }
        }

      })

}


var i=0
 testit(data[i].name ,  data[i].id,  data[i].mongo,  data[i].explanation, i)


function mean_average_precision(scores){
    var sum=scores.reduce(function(a,b){return a+b})
    return sum/scores.length
}


function rank(fails){
    var arr=[];
    for(var i in fails){
        arr.push({})
    }
}

function to_arr(obj){
    var arr=[];
    for(var i in obj){
        arr.push({key:i, count:obj[i]})
    }
    return arr.sort(function(a,b){return b.count - a.count})
}
