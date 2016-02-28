import sys, string

fname = sys.argv[1]

f_in = open(fname, 'r')

big_dict = {}

for line in f_in.readlines():
    attr = line.split('^')[1]
    attr = string.translate(attr, None, string.punctuation)
    word_list = attr.split()
    for word in word_list:
        if len(word) >= 3 and word not in big_dict:
            big_dict[word] = ''

f_in.close()

f_out = open('words.txt', 'w')

for key in big_dict.iterkeys():
    f_out.write(key+'\n');

f_out.close()

