import random
import sys
##########################
# Configuration parameters
dataSize = 100 # data size default
outputFileName = 'data.json'
##########################


class Field:

   # Field Name
   fieldName = "field"

   # TypeCode values:
   # numerical
   # realtext
   # synthtext
   # datetime
   # stopwords
   typeCode = 'numerical'
   
   # Value Type is different for different typeCodes
   # For Numerical:
   ## 'integer', 'float'
   # For real text
   ## 'single-word','short','medium','long'
   valueType = 'integer'

   # Order defines the order of values in the records 
   # which are written in the file.
   # Only valid for Numerical and Date/Time
   ## values are 'asc','desc','random' 
   order = 'asc'

   # Minimum value
   # Only valid for Numerical
   valueMin = 0

   # Maximum value
   # Only valid for Numerical
   valueMax = 1000

   # Gap value
   # Only valid for Numerical
   valueGap = 1

   # Value set
   # Only valid for real text, single-word
   valueSet = list()

   # Format Code or Date/Time
   # Values : 1,2,3
   # 1 : mm/DD/YYYY
   # 2 : HH:MM:SS
   # 3 : mm/DD/YYYY HH:MM:SS
   formatCode = 1

   # Number of records in which this field will be included
   # Values:
   ## A Number which indicates this value
   recordNumber = 1000

   # Offset of records from which this field will be included
   # Values:
   ## A number which indicates this offset
   ## 'end' : recordNumber records from the end will contain this field
   recordOffset = 0

   # Initializes a Numerical field
   def initNumerical(self, valueType, order, valueMin, valueMax, valueGap, recordOffset, recordNumber):
      self.typeCode = 'numerical'
      self.valueType = valueType
      self.order = order
      if self.valueType == 'integer':
         self.valueMin = int(valueMin)
         self.valueMax = int(valueMax)
         self.valueGap = int(valueGap)
      elif self.valueType == 'float':
         self.valueMin = float(valueMin)
         self.valueMax = float(valueMax)
         self.valueGap = float(valueGap)
      self.recordNumber = recordNumber
      self.recordOffset = recordOffset

   # Initializes a Real Text field
   def initRealText(self, valueType, valueSet, recordOffset, recordNumber):
      self.typeCode = 'realtext'
      self.valueType = valueType
      self.valueSet = valueSet
      self.recordNumber = recordNumber
      self.recordOffset = recordOffset

   # Initializes a synthetic text term field
   def initSynthText(self, recordOffset, recordNumber):
      self.typeCode = 'synthtext'
      self.recordNumber = recordNumber
      self.recordOffset = recordOffset

   # Initializes a Date/Time field
   def initDateTime(self, formatCode, order, recordOffset, recordNumber):
      self.typeCode = 'datetime'
      self.formatCode = formatCode
      self.order = order
      self.recordNumber = recordNumber
      self.recordOffset = recordOffset

   # Initializes an empty field
   def __init__(self):
      self.typeCode = self.typeCode

   # Generates a list of values based on the properties of this field
   def generateValues(self):
      if self.typeCode == 'numerical': # Numerical
         result = list()
         currentValue = self.valueMin
         for i in range(0,self.recordNumber):
            result.append(currentValue)
            currentValue += self.valueGap
            if currentValue > self.valueMax:
                currentValue = self.valueMin
         if self.order == 'desc': # no need to do anything for ascending
            result.reverse()
         elif self.order == 'random':
            random.shuffle(result)
         return result
      elif self.typeCode == 'realtext': # Real text
         result = list()
         valuesToUse = list()
         if self.valueType == 'singleterm':
            valuesToUse = self.valueSet
         else:
            f = open('text_' + self.valueType)
            for line in f:
               valuesToUse.append(line.strip())
         for i in range(0,self.recordNumber):
             result.append(valuesToUse[i % len(valuesToUse)])
         return result
      elif self.typeCode == 'synthtext': # Synthetic text
         result = list()
         for i in range(0,self.recordNumber):
            result.append('srch2_zzzzzzzzzz')
         return result
      elif self.typeCode == 'datetime': # Date/Time
         result = list()
         valuesToUse = list()
         f = open('datetime_' + self.formatCode)
         for line in f:
            valuesToUse.append(line.strip())
         for i in range(0,self.recordNumber):
            result.append(valuesToUse[i % len(valuesToUse)])
         if self.order == 'desc':
            result.reverse()
         return result
      elif self.typeCode == 'stopwords': # Stop Words
         result = list()
         valuesToUse = list()
         f = open('stopwords')
         for line in f:
            valuesToUse.append(line.strip())
         for i in range(0,self.recordNumber):
            result.append(valuesToUse[i % len(valuesToUse)])         
         return result
      else: # Unknown type, returning empty list
         return list()

# Returns a list of list of fields. Each list will be concated by ' ' to produce one field value
def parseFieldList(inputString):
   result = list()
   fieldStrings = inputString.split(',')
   for fieldString in fieldStrings:
      fieldName = fieldString.split(':')[0]
      fieldString = fieldString.split(':')[1]
      fieldPartsString = fieldString.split('+')
      result.append(list())
      for fieldPartString in fieldPartsString:
         fieldParts = fieldPartString.split('-')
         if fieldParts[0] == 'numerical':#################################
            valueType = fieldParts[1]
            order = fieldParts[2]
            valueRange = fieldParts[3][5:-1]
            # needing to parse min,max,gap
            minValue = valueRange.split('*')[0]
            gapValue = valueRange.split('*')[1]
            maxValue = valueRange.split('*')[2]
            if len(fieldParts) > 4:
               recordNumber =  int(fieldParts[4])
            else:
               recordNumber = dataSize
            if len(fieldParts) > 5:
               recordOffset =  int(fieldParts[5])
            else:
               recordOffset = 0
            field = Field()
            field.initNumerical(valueType, order, minValue, maxValue, gapValue, recordOffset, recordNumber)
            field.fieldName = fieldName
            result[len(result)-1].append(field)
         elif fieldParts[0] == 'realtext':##################################
            valueType = fieldParts[1]
            valueSet = list()
            recordNumber = dataSize
            recordOffset = 0
            if valueType == 'singleterm':###################################
               valueSet = fieldParts[2][1:-1].split('%')
               if len(fieldParts) > 3:
                  recordNumber =  int(fieldParts[3])
               else:
                  recordNumber = dataSize
               if len(fieldParts) > 4:
                  recordOffset =  int(fieldParts[4])
               else:
                  recordOffset = 0
            else:
               if len(fieldParts) > 2:
                  recordNumber =  int(fieldParts[2])
               else:
                  recordNumber = dataSize
               if len(fieldParts) > 3:
                  recordOffset =  int(fieldParts[3])
               else:
                  recordOffset = 0
            field = Field()
            field.initRealText(valueType, valueSet, recordOffset, recordNumber)
            field.fieldName = fieldName
            result[len(result)-1].append(field)
         elif fieldParts[0] == 'synthtext':#################################
            if len(fieldParts) > 1:
               recordNumber =  int(fieldParts[1])
            else:
               recordNumber = dataSize
            if len(fieldParts) > 2:
               recordOffset =  int(fieldParts[2])
            else:
               recordOffset = 0
            field = Field()
            field.initSynthText(recordOffset, recordNumber)
            field.fieldName = fieldName
            result[len(result)-1].append(field)
         elif fieldParts[0] == 'datetime':#####################################
            formatCode = fieldParts[1]
            order = fieldParts[2]
            if len(fieldParts) > 3:
               recordNumber =  int(fieldParts[3])
            else:
               recordNumber = dataSize
            if len(fieldParts) > 4:
               recordOffset =  int(fieldParts[4])
            else:
               recordOffset = 0
            field = Field()
            field.initDateTime(formatCode, order, recordOffset, recordNumber)
            field.fieldName = fieldName
            result[len(result)-1].append(field)
         elif fieldParts[0] == 'stopwords':#######################################
            field = Field()
            field.typeCode = 'stopwords'
            field.fieldName = fieldName
            result[len(result)-1].append(field)
         else:######################################
            print 'Unknown type of field.'
   return result

def prepareRecords(fieldList):
   records = list()
   for i in range(0,dataSize):
      records.append("{")
   for fieldIndex in range(0,len(fieldList)): # fields : a list of field objects which must be concated by ' '
      fields = fieldList[fieldIndex]
      values = list() # a list of lists of values parallel to fields
      for field in fields:
         values.append(field.generateValues())
      for i in range(0,dataSize):
         fieldValue = ""
         for valueListIndex in range(0,len(values)):
            valueList = values[valueListIndex]
            offset = fields[valueListIndex].recordOffset
            if i-offset >= 0 and i-offset < len(valueList):
               fieldValue += " " + str(valueList[i-offset])
         if fieldIndex > 0:
            records[i] += ', '
         records[i] += '"' + fields[0].fieldName + '":"' + fieldValue + '"'
   for i in range(0,dataSize):
      if i == dataSize - 1:
         records[i] += '}'
      else:
         records[i] += '},\n'
   return records

def printMan():
   print 'This program generates a dataset based on the description\n given by arguments. The output is printed in JSON format to a file.'
   print 'Arguments:'
   print '--size or -s : '
   print '\t The number of records generated. Default value is 100'
   print '--output or -o : '
   print '\t The output file in which the data is written.'
   print '--fieldlist or -f : '
   print '\tThe value of this argument is a comma-separated list of field-descriptions:'
   print '\tfield-description,field-description, ...,field-description'
   print '\t\tfield-description:'
   print '\t\t field-description is a list of componen-descriptions separated by +:'
   print '\t\t component-description+ ... +component-description'
   print '\t\t Values produces for each component will be appended together by " "'
   print '\t\t and generate the value of a field.'
   print '\t\tcomponent-description:'
   print '\t\t There are five different types if component.'
   print '\t\t\t numerical : Used to generate integer and float values.'
   print '\t\t\t realtext : Used to generate real textual values.'
   print '\t\t\t synthtext : Used to generate synthetic textual values.'
   print '\t\t\t datetime : Used to generate Date/Time values.'
   print '\t\t\t stopwords : Used to add stop words to field values.'
   print '\t\t In the following, the syntax of component-description for each type of'
   print '\t\t  component is explained.'
   print '\t\t numerical:'
   print '\t\t\t numerical-type-order-range-numberOfRecords-offset'
   print '\t\t\t   type can be integer or float'
   print '\t\t\t   order can be asc, desc or random'
   print '\t\t\t   range is in the format of from[start*gap*end]'
   print '\t\t\t   numberOfRecords is the number of records which contain this component.'
   print '\t\t\t   offset is the offset of the first record which contains this component.'
   print '\t\t\t   Example 1: numerical-integer-asc-from[1*2*20]-20-3'
   print '\t\t\t   Example 2: numerical-float-random-from[1.4*3.5*100]'
   print '\t\t\t   NOTE: numberOfRecords and offset are optional. Default values are'
   print '\t\t\t   100 and 0 respectively.'
   print '\t\t realtext:'
   print '\t\t\t realtext-type[-value set]-numberOfResults-offset'
   print '\t\t\t   type can be singleterm, short, medium and long'
   print '\t\t\t   if type is singleterm a value set must be provided in format of {TERM1%...%TERM2}'
   print '\t\t\t   numberOfRecords is the number of records which contain this component.'
   print '\t\t\t   offset is the offset of the first record which contains this component.'
   print '\t\t\t   Example 1: realtext-short-10-2'
   print '\t\t\t   Example 2: realtext-long'
   print '\t\t\t   Example 3: realtext-singleterm-{Comedy%Horror%Drama}'
   print '\t\t\t   NOTE: numberOfRecords and offset are optional. Default values are'
   print '\t\t\t   100 and 0 respectively.'
   print '\t\t synthtext:'
   print '\t\t\t synthtext-numberOfRecords-offset'
   print '\t\t\t   numberOfRecords is the number of records which contain this component.'
   print '\t\t\t   offset is the offset of the first record which contains this component.'
   print '\t\t\t   Example 1: synthtext-10-0'
   print '\t\t\t   NOTE: numberOfRecords and offset are optional. Default values are'
   print '\t\t\t   100 and 0 respectively.'
   print '\t\t datetime:'
   print '\t\t\t datetime-format-order-numberOfRecords-offset'
   print '\t\t\t   format is mmddyyyy or hhmmss or mmddyyyy_hhmmss'
   print '\t\t\t   order is asc or desc'
   print '\t\t\t   numberOfRecords is the number of records which contain this component.'
   print '\t\t\t   offset is the offset of the first record which contains this component.'
   print '\t\t\t   Example 1: datetime-mmddyyyy-asc-10-2'
   print '\t\t\t   NOTE: numberOfRecords and offset are optional. Default values are'
   print '\t\t\t   100 and 0 respectively.'
   print '\t\t stopwords:'
   print '\t\t\t stopwords-numberOfRecords-offset'
   print '\t\t\t   numberOfRecords is the number of records which contain this component.'
   print '\t\t\t   offset is the offset of the first record which contains this component.'
   print '\t\t\t   Example 1: stopwords-10-0'
   print '\t\t\t   NOTE: numberOfRecords and offset are optional. Default values are'
   print '\t\t\t   100 and 0 respectively.'



if __name__ == '__main__':
   i = 1
   fieldLists = ''
   while i < len(sys.argv):
      if sys.argv[i] == "--size" or sys.argv[i] == '-s':
         i = i+1
         dataSize = int(sys.argv[i])
         i = i+1
      elif sys.argv[i] == "--output" or sys.argv[i] == '-o':
         i = i+1
         outputFileName = sys.argv[i]
         i = i+1
      elif sys.argv[i] == "--fieldlist" or sys.argv[i] == '-f':
         i = i+1
         fieldLists = sys.argv[i]
         i = i+1
      else:
         printMan()
         quit(-1)
   if fieldLists == '':
      printMan()
      quit(-1)
   fieldLists = parseFieldList(fieldLists)
   records = prepareRecords(fieldLists)
   outputFile = open(outputFileName , 'w')
   for record in records:
      outputFile.write(record+'\n')
