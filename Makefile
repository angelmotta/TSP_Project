all:
	mpic++ main.cpp -o tsp.out
run:
	mpirun -np 2 ./tsp.out
