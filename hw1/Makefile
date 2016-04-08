all:
	./compile.sh

run:
	./execute.sh -r -i query-test.xml -o ranked-list -m model -d CIRB010

diff:
	diff ranked-list ranked-list3

time:
	time ./execute.sh -r -i query-test.xml -o ranked-list2 -m model -d CIRB010

IR:
	gcc ./IR.c -lm -o vsm

	
