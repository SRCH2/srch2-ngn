import json

f = open('data', 'r')
out = open('data_simplified', 'w')
count = 0

for line in f.readlines():
    obj = json.loads(line)
    try:
        out.write(obj['index'] + '^' + obj['attr'] + '\n')
    except UnicodeEncodeError, e:
        pass

    count += 1

    if count % 100000 == 0:
        print count

out.close()
f.close()
