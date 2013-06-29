import sys

group_size = int(sys.argv[1])

f = open('data_simplified', 'r')

line_dict = {}

for line in f.readlines():
    attr = line.split('^')[1]
    attr_list = attr.split(' ')
    list_len = len(attr_list)
    if list_len in line_dict:
        line_dict[list_len].append(line)
    else:
        line_dict[list_len] = [line]

word_list = []
word_idx = 0

for line_len in line_dict:
    out = open('record_with_' + str(line_len) + '_keywords', 'w')
    count = 0
    for line in line_dict[line_len]:
        if count < group_size:
            if line[-2] != '^':
                out.write(line)
                count += 1
        else:
            attr_list = line.split('^')
            word_list += attr_list[1].strip().split()


    while count < group_size:
        addition_line = "generated_id_" + str(count) + '^'
        for i in range(line_len):
            if word_idx == len(word_list):
                word_idx = 0
            addition_line += ' ' + word_list[word_idx]
            word_idx += 1
        addition_line += '\n'
        out.write(addition_line)
        count += 1
    out.close()

f.close()
