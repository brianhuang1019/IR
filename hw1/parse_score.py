import time
from time import gmtime, strftime
from datetime import datetime
import sys
reload(sys)
sys.setdefaultencoding('utf8')

from BeautifulSoup import BeautifulSoup
import urllib2
import re

print "ID:", sys.argv[1], '\n'

score = -1
upload_time = ""
number = -1

while True:

	soup = BeautifulSoup(urllib2.urlopen('http://wm.csie.ntu.edu.tw/wm2016hw1/'))
	
	tr = soup.findAll('tr')
	for row in tr:
		tds = row.findAll('td')
		if len(tds) >= 3 and str(tds[1]).find(sys.argv[1]) > -1:
			if upload_time != str(re.findall("[-\d]+[ ]+[:\d]+", str(tds[2]))[0]):
				score = re.findall("[-+]?\d+[\.]?\d*", str(tds[3]))[0]
				upload_time = str(re.findall("[-\d]+[ ]+[:\d]+", str(tds[2]))[0])
				number = re.findall("[-+]?\d+[\.]?\d*", str(tds[0]))[0]
				print >> sys.stderr, "\r",
				print "No.", number, "/", len(tr), "                      "
				print "Score:", score
				print "Upload Time:", upload_time + '\n'
	print >> sys.stderr, "\rCurrent Time:", datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
	time.sleep(3)
