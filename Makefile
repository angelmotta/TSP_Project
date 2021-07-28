all:
	mpic++ main.cpp -o tsp.out
	mpirun -np 2 ./tsp < input/test1.txt
