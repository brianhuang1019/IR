import sys
import os
import re
from math import log

# global parameter
SMOOTHING = 0.01

# parse arguments

arguments = sys.argv
directory = ''
output = ''
labeled = None

if '-i' in arguments:
	directory = arguments[arguments.index('-i')+1]

if '-o' in arguments:
	output = arguments[arguments.index('-o')+1]

if '-n' in arguments:
	labeled = arguments[arguments.index('-n')+1]

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
	for filename in Train[topic]:
		content = open(filename, 'r').read()
		content = removeUselessContent(content)
		for term in content:
			if term not in wordCount[topic]['words']:
				wordCount[topic]['words'][term] = 1
			else:
				wordCount[topic]['words'][term] += 1
	#wordCount[topic]['words'] = removeUselessWord(wordCount[topic]['words'])
	wordCount[topic]['unique_length'] = len(wordCount[topic]['words'].keys())
	for key in wordCount[topic]['words']:
		if key not in Terms:
			Terms.append(key)
		wordCount[topic]['length'] += wordCount[topic]['words'][key]
	print topic, wordCount[topic]['length'], wordCount[topic]['unique_length'], len(Terms)


# naive Bayes

C_plus_D = float(len(Topic)+len(Label))
def topicPriorProbability(topic):
	topicDocs = len(Train[topic])
	return float(1+topicDocs) / C_plus_D

def wordProbability(word, topic):
	wordInTopic = 0
	if word in wordCount[topic]['words']:
		wordInTopic = wordCount[topic]['words'][word]
	topicLength = wordCount[topic]['length']
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

output_file = open(output, 'w')
maxData = {}

for filename in Test:
	content = open(filename, 'r').read()
	content = removeUselessContent(content)
	topicProbability = {}
	for topic in Topic:
		topicProbability[topic] = countTopicProbability(topic, content)
	maxTopic = findMax(topicProbability)
	maxData[int(filename.split('/')[2])] = maxTopic

sorted(maxData)

for key in maxData:
	output_file.write(str(key) + ' ' + maxData[key] + '\n')

output_file.close()