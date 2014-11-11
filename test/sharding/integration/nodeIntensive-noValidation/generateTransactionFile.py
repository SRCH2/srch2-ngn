#This python code generates a transaction file for 20 nodes (A-T) such that start and kill operation will be randomly selected for each of these nodes with the constraint that start will come before kill.

from sets import Set
import random, sys

if __name__ == '__main__':
  start = Set(['A||start', 'B||start', 'C||start', 'D||start', 'E||start', 'F||start', 'G||start', 'H||start', 'I||start', 'J||start'])
  inputFile = sys.argv[1]
  f_in = open(inputFile,'a')
  for i in start:
    if(sys.argv[2] == 'sleep'):
        f_in.write('-||sleep||70' + '\n')
    f_in.write(i + '\n')

  startKill = Set(['K||start','L||start','M||start','N||start','O||start','P||start','Q||start','R||start','S||start','T||start','A||kill','B||kill','C||kill','D||kill','E||kill','F||kill','G||kill','H||kill','I||kill','J||kill'])

  for j in xrange(0,30):
    pick = random.sample(startKill,1)
    if((pick[0] == 'K||start') or (pick[0] == 'L||start') or (pick[0] == 'M||start') or (pick[0] == 'N||start') or (pick[0] == 'O||start') or (pick[0] == 'P||start') or (pick[0] == 'Q||start') or (pick[0] == 'R||start') or (pick[0] == 'S||start') or (pick[0] == 'T||start')):
        temp = pick[0]
        newOp = temp[0:3]+"kill"
        startKill.remove(pick[0])
        startKill.add(newOp)
    else:
        startKill.remove(pick[0])
    f_in.write(pick[0] + '\n')
    if(sys.argv[2] == 'sleep'):
        f_in.write('-||sleep||70' + '\n')
    else:
        continue 
