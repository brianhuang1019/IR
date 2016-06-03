answer = open('ans.test', 'r').read().split('\n')
output = open('output', 'r').read().split('\n')

correct = wrong = 0

for i in range(0, len(answer)):
	if answer[i] == output[i]:
		correct += 1
		#print answer[i], output[i]
	else:
		wrong += 1

print correct, '/', correct+wrong
print "Accuracy:", float(correct) / float(correct+wrong)