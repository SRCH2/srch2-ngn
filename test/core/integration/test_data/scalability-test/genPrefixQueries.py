import sys, random, string

prefix_len_limit = 3

def fuzzy(s):
    idx = random.randint(0, len(s)-1)
    l = list(s)
    l[idx] = random.choice(string.ascii_lowercase)
    return "".join(l)

f = open(sys.argv[1], 'r')

se = open('single_exact', 'w')
sf = open('single_fuzzy', 'w')
de = open('double_exact', 'w')
df = open('double_fuzzy', 'w')

isOddLine =True
first_exact = ''

for line in f.readlines():
    if isOddLine:
        first_exact = line.strip()
        for i in range(len(first_exact))[prefix_len_limit-1:]:
            se.write( first_exact[:i+1] + '\n' )
            sf.write( fuzzy(first_exact[:i+1]) + '\n' )
        isOddLine = False

    else:
        second_exact = line.strip()
        for i in range(len(second_exact))[prefix_len_limit-1:]:
            de.write( first_exact + '+' + second_exact[:i+1] + '\n' )
            df.write( fuzzy(first_exact) + '+' + second_exact[:i+1] + '\n' )
        isOddLine = True;
        
df.close()
de.close()
sf.close()
se.close()
f.close()
        
