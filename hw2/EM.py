import sys
import os
import re
from math import log

# global parameter
SMOOTHING = 0.027

# parse arguments

arguments = sys.argv
directory = ''
output = ''
labeled = 1

if '-i' in arguments:
	directory = arguments[arguments.index('-i')+1]

if '-o' in arguments:
	output = arguments[arguments.index('-o')+1]

if '-n' in arguments:
	labeled = int(arguments[arguments.index('-n')+1])

print "Arguments:", directory, output, labeled


# parse documents name

def legalFile(filename):
	if (filename == '.DS_Store'):
		return False
	return True

Test = []		# all Test docs
Unlabel = []	# all unlabeled docs
Label = []		# all labeled docs
Topic = []		# all topics
Train = {}		# all labeled docs, key = topic

for filename in os.listdir(directory + '/Test'):
	if (legalFile(filename)):
		Test.append(directory + '/Test/' + filename)

for topics in os.listdir(directory + '/Train'):
	if (legalFile(topics)):
		Topic.append(topics)
		Train[topics] = []
		for filename in os.listdir(directory + '/Train/' + topics):
			if (legalFile(filename)):
				Train[topics].append(directory + '/Train/' + topics + '/' + filename)
				Label.append(directory + '/Train/' + topics + '/' + filename)

for filename in os.listdir(directory + '/Unlabel'):
	if (legalFile(filename)):
		Unlabel.append(directory + '/Unlabel/' + filename)


# parse vocabularies

def removeUseless(str):
	# remove digits
	#str = ''.join([i for i in str if not i.isdigit()])

	# remove symbol
	#symbol = ['.', '<', '>', '@', ':', '(', ')', '"', ',', '_', '-', '/', '\\', '^', '?', '[', ']', ';', '*', '|', '{', '}', '+', '=', '&', '%', '$', '#', '!', '~', '`']
	
	# find all vocabularies
	vocab = re.findall('[a-zA-Z]+', str)
	str = ''.join([i for i in vocab])
	str = str.lower()
	return str

def removeUselessContent(content):
	content = content.split()
	for i in range(0, len(content)):
		vocab = re.findall('[a-zA-Z]+', content[i])
		content[i] = ''.join([k for k in vocab])
		content[i] = content[i].lower()
	if '' in content:
		content.remove('')
	# content = re.findall('[a-zA-Z]+', content)
	# for i in range(0, len(content)):
	# 	content[i] = content[i].lower()
	return content

def removeUselessWord(dictionary):
	useless = []
	for key in dictionary:
		if dictionary[key] < 1:
			useless.append(key)
	#print useless
	for key in useless:
		del dictionary[key]
	return dictionary

Terms = []
wordCount = {}

for topic in Topic:
	wordCount[topic] = {}
	wordCount[topic]['length'] = 0
	wordCount[topic]['unique_length'] = 0
	wordCount[topic]['words'] = {}
	topicCount = 0
	for filename in Train[topic]:
		content = open(filename, 'r').read()
		content = removeUselessContent(content)
		for term in content:
			if term not in wordCount[topic]['words']:
				wordCount[topic]['words'][term] = 1
			else:
				wordCount[topic]['words'][term] += 1
		topicCount += 1
		if topicCount >= labeled:
			break
	#wordCount[topic]['words'] = removeUselessWord(wordCount[topic]['words'])
	wordCount[topic]['unique_length'] = len(wordCount[topic]['words'].keys())
	for key in wordCount[topic]['words']:
		if key not in Terms:
			Terms.append(key)
		wordCount[topic]['length'] += wordCount[topic]['words'][key]
	print topic, wordCount[topic]['length'], wordCount[topic]['unique_length'], len(Terms)


# EM

answerData = open('ans.test', 'r').read().split('\n')
answer = {}
for ans in answerData:
	answer[int(ans.split(' ')[0])] = ans.split(' ')[1]
maxData = {}

wordCountPrime = {}
for topic in Topic:
	wordCountPrime[topic] = {}
	wordCountPrime[topic]['words'] = {}
	wordCountPrime[topic]['length'] = 0
wordCountFile = {}
for filename in Test:
	wordCountFile[filename] = {}
	wordCountFile[filename]['words'] = {}
	wordCountFile[filename]['content'] = []
	wordCountFile[filename]['topic'] = ''

C_plus_D = float(len(Topic)+len(Label))
def topicPriorProbability(topic):
	topicDocs = len(Train[topic])
	return float(1+topicDocs) / C_plus_D

def wordProbability(word, topic):
	wordInTopic = 0
	if word in wordCount[topic]['words']:
		wordInTopic = wordCount[topic]['words'][word]
	if word in wordCountPrime[topic]['words']:
		wordInTopic += wordCountPrime[topic]['words'][word]
	topicLength = wordCount[topic]['length'] + wordCountPrime[topic]['length']
	#print word, float(0.2+wordInTopic), "/", float(0.2*len(Terms)+topicLength), float(0.2+wordInTopic) / float(0.2*len(Terms)+topicLength)
	return float(SMOOTHING+wordInTopic) / float(SMOOTHING*len(Terms)+topicLength)

def countTopicProbability(topic, content):
	topic_P = log(topicPriorProbability(topic))
	probability_pi = 0
	for word in content:
		probability_pi += log(wordProbability(word, topic))
	return topic_P + probability_pi

def findMax(dictionary):
	maxKey = ''
	maxValue = -float("inf")
	for key in dictionary:
		if dictionary[key] > maxValue:
			maxValue = dictionary[key]
			maxKey = key
	return maxKey

def addToModel(wordCountFile, wordCountPrime):
	# init wordCountPrime
	for topic in Topic:
		wordCountPrime[topic] = {}
		wordCountPrime[topic]['words'] = {}
		wordCountPrime[topic]['length'] = 0
	# count wordCountPrime
	for filename in Test:
		topic = wordCountFile[filename]['topic']
		wordCountPrime[topic]['length'] += len(wordCountFile[filename]['content'])
		for term in wordCountFile[filename]['words']:
			if term not in wordCountPrime[topic]['words']:
				wordCountPrime[topic]['words'][term] = wordCountFile[filename]['words'][term]
			else:
				wordCountPrime[topic]['words'][term] += wordCountFile[filename]['words'][term]
	return wordCountPrime

_round = 0
def countAccuracy(maxData):
	correct = 0
	wrong = 0
	for key in answer:
		if answer[key] == maxData[key]:
			correct += 1
		else:
			wrong += 1
	accuracy = float(correct) / float(correct+wrong)
	print "Round:", _round, "Accuracy:", accuracy
	return accuracy

def converge(accuracy_prev, accuracy_cur):
	if accuracy_cur == accuracy_prev:
		return True
	else:
		return False

# add unlabeled to topic
print "Parse Unlabeled."
for filename in Test:
	content = open(filename, 'r').read()
	wordCountFile[filename]['content'] = removeUselessContent(content)
	topicProbability = {}
	for topic in Topic:
		topicProbability[topic] = countTopicProbability(topic, wordCountFile[filename]['content'])
	maxTopic = findMax(topicProbability)
	_file = filename.split('/')
	maxData[int(_file[len(_file)-1])] = maxTopic
	wordCountFile[filename]['topic'] = maxTopic
	for term in wordCountFile[filename]['content']:
		# To slow, so skip it
		# if term not in Terms:
		# 	Terms.append(term)
		if term not in wordCountFile[filename]['words']:
			wordCountFile[filename]['words'][term] = 1
		else:
			wordCountFile[filename]['words'][term] += 1
	# To slow, so skip it
	# for term in wordCountFile[filename]['words']:
	# 	if term not in Terms:
	# 		Terms.append(term)
	print "Parse", filename, "finished."
print "Parse Unlabeled finished."

# EM algorithm
print "Start EM."
wordCountPrime = addToModel(wordCountFile, wordCountPrime)
accuracy_prev = countAccuracy(maxData)
while True:
	accuracy = 0
	_round += 1
	# E-step
	for filename in Test:
		topicProbability = {}
		for topic in Topic:
			topicProbability[topic] = countTopicProbability(topic, wordCountFile[filename]['content'])
		maxTopic = findMax(topicProbability)
		wordCountFile[filename]['topic'] = maxTopic
		_file = filename.split('/')
		maxData[int(_file[len(_file)-1])] = maxTopic
	accuracy_cur = countAccuracy(maxData)
	if converge(accuracy_prev, accuracy_cur):
		break
	else:
		accuracy_prev = accuracy_cur
		# M-step
		wordCountPrime = addToModel(wordCountFile, wordCountPrime)
		
sorted(maxData)

output_file = open(output, 'w')
for key in maxData:
	output_file.write(str(key) + ' ' + maxData[key] + '\n')
output_file.close()

with open(output, 'rb+') as filehandle:
    filehandle.seek(-1, os.SEEK_END)
    filehandle.truncate()