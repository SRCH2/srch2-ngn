import json, sys

if __name__ == '__main__':

    i = 1
    counter = 0;
    inputF = sys.argv[1]  
    f_out = open("out3.json","a")
    f_in = open(inputF)
    num = ord('a')
    lastChar = ord('z')
    last = 1
    for line in f_in:
        temp = ""
        val=line.split('\n')
        jsonrec=val[0]
        if(jsonrec[0] == '['):
            continue;
        elif(jsonrec[0] == ']'):
            continue;
        if(jsonrec[-1] == ','):
            jsonrec = jsonrec[:-1]

#        print jsonrec
        data=json.loads(jsonrec)
#        print data["id"]
        data["id"] = str(counter + 1)
       
        if((counter%5 == 0) & (counter > 0) & (num <= 109 ) ):
             num = num +1
             lastChar = lastChar - 1
             last = last + 1
      
        print data["id"]
        if(num  <= 109):  
             data["body"] = chr(num)*6 +" "+ str(last)*6 +" "+ chr(lastChar)*6 + " " + data["body"]
             
        counter = counter + 1 
        data1=json.dumps(data)+"\n"   
        f_out.write(data1) 
    f_in.close()
    f_out.close()
         
