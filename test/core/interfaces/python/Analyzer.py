import bimaple_mapsearch_python as instant

analyzer = instant.Analyzer_create()
stringVector = instant.StringVector()

analyzer.tokenizeQuery("wake+me+up+when+this+compiles",stringVector,"+")

for item in stringVector:
	print item
