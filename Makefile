all:
	mpic++ main.cpp -o tsp.out
	mpirun -np 2 ./tsp.out < input/test1.txt
