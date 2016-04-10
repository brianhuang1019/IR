all:
	./compile.sh

run:
	./execute.sh -r -i query-test.xml -o ranked-list -m model -d CIRB010
	cat ranked-list | head -1907 > ranked-list4

diff:
	diff ranked-list ranked-list2

time:
	time ./execute.sh -r -i query-test.xml -o ranked-list2 -m model -d CIRB010
	cat ranked-list2 | head -1908 > ranked-list5

IR:
	gcc ./IR.c -lm -o vsm

	
