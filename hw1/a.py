f_list = open('./model/file-list', 'r')
file_count = 0
for line in f_list:
    file_count += 1
print "file_count:", file_count

v_list = open('./model/vocab.all', 'r')
vocab_count = 0
for line in v_list:
    vocab_count += 1
print "vocab_count:", vocab_count
