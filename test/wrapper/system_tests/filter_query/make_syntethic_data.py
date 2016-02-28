import sys
import math
# This script uses an existing input data and appends synthetic fields which are proper for testing post processing
# filters like Faceted Search, Sort Filter and Filter Query. Three fields are appended. "price", "likes" and "model"
# which is of types float, unsigned and text.
def preparePrice():
   price = []
   for i in range(0,100):
      price.append(100-i+0.5)
   return price

def prepareLikes():
   likes = []
   for i in range(0,100):
      likes.append(i)
   return likes

def prepareExpiration():
   expiration = []
   for i in range(0,100):
      expiration.append('01/01/' + str(1910+i))
   return expiration


def prepareModel():
   model = []
   for i in range(0,5):
      model.append('BMW')
   for i in range(5,15):
      model.append('FORD')
   for i in range(15,30):
      model.append('HONDA')
   for i in range(30,50):
      model.append('JEEP')
   for i in range(50,75):
      model.append('MAZDA')
   for i in range(75,100):
      model.append('TOYOTA')
   return model

def prepareAdditionalRecords():
   model = prepareModel()
   price = preparePrice()
   likes = prepareLikes()
   expiration = prepareExpiration()
   data = []
   for i in range(0,100):
      data.append('"model": "'+model[i]+'","price":'+str(price[i])+',')
      data[i] = data[i] + '"likes":'+ str(likes[i])  + ',"expiration":"'+ expiration[i] + '"'
   
   return data


if __name__ == '__main__':
   inputDataFile = sys.argv[1]
   dataFile = open(inputDataFile , 'r')
   newData = prepareAdditionalRecords()
   lineNumber = 0
   for record in dataFile:
      if len(record) < 14:
         continue;
      commonKeyword = '"category": "verycommonword '
      if lineNumber % 2 == 0:
         commonKeyword = commonKeyword + 'lesscommonword '
      print('{' + newData[lineNumber] + ', ' + commonKeyword + record[14:]),
      lineNumber = lineNumber + 1

