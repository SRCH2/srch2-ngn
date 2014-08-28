import signal, sys, os
sys.path.append(os.getcwd()+'/../../../thirdparty/pymongo/pymongo')
import pymongo

def signal_handler(signum,frame):
	return


class DBConn:
	conn = None
	
	def connect(self):
		print os.getcwd()
		signal.signal(signal.SIGALRM, signal_handler)
		signal.alarm(3)
		try:
			self.conn = pymongo.MongoClient('localhost:27017')
			return 0
		except :
			print 'Mongodb is not running!'
			return -1

	def close(self):
		return self.conn.disconnect()

	def getConn(self):
		return self.conn
