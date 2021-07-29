#include <mpi.h>
#include <iostream>
#include <vector>
#include <list>
#include <climits>

using namespace std;

int nVertices = 0;
vector <int> grafo;

int getProcLibre(vector<bool> &procOcupados, int totalProc, int &libres){
    for (int i = 0; i < totalProc; i++){
        if (procOcupados[i] == false) {
            procOcupados[i] = true;
            libres--;
            return i;
        }
    }
    return -1;
}

int minCost( int start, int idx){
    int minimo = INT_MAX;
    for(int i=0; i<nVertices; i++){
        if(i != idx && grafo[start + i] < minimo)
            minimo = grafo[start + i];
    }
    return minimo;
}

int getLowerBound(vector<int>& camino, int currCost) {
    for(int i=0; i<nVertices; i++){
        if(camino[i] == -1){
            currCost += minCost(i*nVertices, i);
        }
    }
    return currCost;
}

void getBranches(list< pair<vector<int>, vector<int> > >& caminos, vector<int>& camino, vector<int>& metadata) {
    bool encontrado = false;
    int ultimoNodoTomado = metadata[2];
    for (int i = 0; i < nVertices; i++) {
        if (camino[i] == -1 && i != ultimoNodoTomado) {
            encontrado = true;
            vector<int> nuevoCamino = camino;
            nuevoCamino[ultimoNodoTomado] = i;
            vector<int> nuevaMetadata = metadata;
            nuevaMetadata[1] += grafo[ultimoNodoTomado * nVertices + i];
            nuevaMetadata[2] = i; // ultimo nodo Tomado
            caminos.push_front(make_pair(nuevoCamino, nuevaMetadata));
        }
    }
    if (!encontrado) { // llegamos a un nivel hoja
        camino[ultimoNodoTomado] = 0;
        metadata[1] += grafo[ultimoNodoTomado * nVertices];
        metadata[2] = -1;
        metadata[3] = 1; // 1: nivel hoja
        caminos.push_front(make_pair(camino, metadata));
    }
}

int main (int argc, char *argv[]) {
    int rank, size;
    int master = 0;
    double start, end;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == master) {
        start = MPI_Wtime();
        // Leer matriz adyacencia
        cin >> nVertices;
        grafo.resize(nVertices * nVertices);
        for (int i = 0; i < nVertices; i++) {
            for (int j = 0; j < nVertices; j++) {
                cin >> grafo[i * nVertices + j];
            }
        }

        // Broadcast del número de vertices y la matriz
        MPI_Bcast(&nVertices, 1, MPI_INT, master, MPI_COMM_WORLD);
        MPI_Bcast(&grafo[0], nVertices * nVertices, MPI_INT, master, MPI_COMM_WORLD);

        // Setear estado de procesos (ocupado o libre)
        int procLibres = size - 1;
        vector <bool> procOcupados(size, false);
        procOcupados[master] = true;

        // Obtener el primer proceso libre
        int procDisponible = 1;
        procLibres--;
        procOcupados[procDisponible] = true;

        // Inicializar pathTSP y metada
        int costoOptimo = INT_MAX;
        vector <int> camino(nVertices, -1);

        // metadata[0] = costo optimo | metadata[1] = costo actual
        // metadata[2] = ultimo vertice del camino | metadata[3] = ha llegado a la hoja
        vector <int> metadata(4, 0);
        metadata[0] = costoOptimo;

        // Enviar camino y metadata
        MPI_Send(&camino[0], nVertices, MPI_INT, procDisponible, 1, MPI_COMM_WORLD);
        MPI_Send(&metadata[0], 4, MPI_INT, procDisponible, 1, MPI_COMM_WORLD);

        // Mientras haya algun proceso trabajando seguir computando el TSP
        //printf("Proc libres: %d\n", procLibres);
        while (procLibres != size-1) {
            MPI_Status status;
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int orden = status.MPI_TAG;
            int src = status.MPI_SOURCE;
            //printf("Src: %d \n\t orden: %d\n", src, orden);
            if (orden == 2) { // se halló un mejor camino con menor costo 
               //printf("\t LLego a la hoja\n");
               MPI_Recv(&camino[0], nVertices, MPI_INT, src, orden, MPI_COMM_WORLD, &status);
               MPI_Recv(&metadata[0], 4, MPI_INT, src, orden, MPI_COMM_WORLD, &status);
               costoOptimo = metadata[1];
            }
            if (orden == 3) { // Proceso en estado libre
                //printf("\t El proceso esta libre\n");
                MPI_Recv(&orden, 1, MPI_INT, src, orden, MPI_COMM_WORLD,  &status);
                procLibres++;
                procOcupados[status.MPI_SOURCE] = false;
            }
            if (orden == 4) {   // recibe mensaje para seguir descendiendo
                //printf("\t BnB, seguir descendiendo\n");
                vector<int> msg(2);
                MPI_Recv(&msg[0], 2, MPI_INT, src, orden, MPI_COMM_WORLD, &status);
                int currMinCost = msg[0];
                int caminosPorProcesar = msg[1];
                if (currMinCost < costoOptimo) {
                    int procAsignados = (caminosPorProcesar <= procLibres) ? caminosPorProcesar : procLibres;
                    //printf("\t se solicitan %d esclavos\n", caminosPorProcesar);
                    //printf("\t se aignan %d esclavos\n", procAsignados);
                    vector<int> slavesAsignados(procAsignados);
                    for (int i = 0; i < procAsignados; i++) {
                        slavesAsignados[i] = getProcLibre(procOcupados, size, procLibres);
                    }
                    MPI_Send(&costoOptimo, 1, MPI_INT, src, 4, MPI_COMM_WORLD);
                    MPI_Send(&slavesAsignados[0], procAsignados, MPI_INT, src, 4, MPI_COMM_WORLD);
                }
                else {
                    orden = 5;      // detener descenso
                    MPI_Send(&costoOptimo, 1, MPI_INT, src, orden, MPI_COMM_WORLD);
                }
            }
        }
        // Detener procesos esclavos
        int orden = 0;      // 0: terminar proceso
        for (int proc_i = 1; proc_i < size; proc_i++){
            MPI_Send(&orden, 1, MPI_INT, proc_i, orden, MPI_COMM_WORLD);
        }
        end = MPI_Wtime();
        printf("Tiempo: %g segundos\n", end - start);
        // Imprimir costo minimo
        cout << "Costo mínimo: " << costoOptimo << endl;
        // Imprimir ruta optima TSP
        cout << "Camino optimo: 0 ";
        int nodo = camino[0];
        while(nodo != 0) {
            cout << nodo << " ";
            nodo = camino[nodo];
        }
        cout << "0" << endl;

        // Finalizar MPI
        MPI_Finalize();
    }
    else { // Los otros procesos
        // Reciben el numero de vertices y creamos la matriz de costos
        MPI_Bcast(&nVertices, 1, MPI_INT, master, MPI_COMM_WORLD);
        grafo.resize(nVertices * nVertices);
        MPI_Bcast(&grafo[0], nVertices*nVertices, MPI_INT, master, MPI_COMM_WORLD);

        // creamos la lista de posibles caminos con su metadata
        list< pair< vector<int>, vector<int> > > caminos;

        // Mientras no recibemos un stop del maestro seguimos trabajando
        MPI_Status status;
        int costoOptimo;
        while(true) {
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            // Recibimos una orden del maestro
            int orden = status.MPI_TAG;
            int src = status.MPI_SOURCE;

            // Caso1: Terminar
            if (orden == 0) {
                MPI_Recv(&orden, 1, MPI_INT, src, orden, MPI_COMM_WORLD, &status);
                break;
            }
            // Caso2: Descender en el arbol
            if (orden == 1) {
                // posible camino
                vector<int> camino(nVertices);
                vector<int> metadata(4);
                MPI_Recv(&camino[0], nVertices, MPI_INT, src, orden, MPI_COMM_WORLD, &status);
                MPI_Recv(&metadata[0], 4, MPI_INT, src, orden, MPI_COMM_WORLD, &status);
                costoOptimo = metadata[0];
                caminos.push_front(make_pair(camino, metadata));

                // (while) Procesamos los vectores (caminos) de la lista
                while(!caminos.empty()){
                    vector<int> posibleCamino = caminos.front().first;
                    vector<int> posMetadata = caminos.front().second;
                    caminos.pop_front();
                    // se llega al nivel hoja
                    if (posMetadata[3] == 1) {
                        if (posMetadata[1] < costoOptimo) {    // si el costo actual es menor al optimo
                            costoOptimo = posMetadata[1];
                            MPI_Send(&posibleCamino[0], nVertices, MPI_INT, master, 2, MPI_COMM_WORLD);
                            MPI_Send(&posMetadata[0], 4, MPI_INT, master, 2, MPI_COMM_WORLD);
                        }
                        continue;
                    }

                    // Obtener el lower_bound (costo min hasta ese momento)
                    int currMinCost = getLowerBound(posibleCamino, posMetadata[1]);

                    // Actualizamos el min costo global de ser el caso
                    if (currMinCost < costoOptimo) {
                        vector<int> msg(2);
                        msg[0] = currMinCost;
                        msg[1] = caminos.size();
                        MPI_Send(&msg[0], 2, MPI_INT, master, 4, MPI_COMM_WORLD); // orden 4: seguir descendiendo

                        // Recibir el costo optimo global desde el master
                        MPI_Recv(&costoOptimo, 1, MPI_INT, master, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                        // Verificar la orden del master
                        if (status.MPI_TAG == 4) {
                            vector<int> slavesAsignados(caminos.size());
                            MPI_Recv(&slavesAsignados[0], caminos.size(), MPI_INT, master, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                            int totalAsignados;
                            MPI_Get_count(&status, MPI_INT, &totalAsignados);
                            // Branch: ramificar el problema
                            getBranches(caminos, posibleCamino, posMetadata);
                            // Repartir carga de trabajo entre esclavos disponibles
                            for (int i = 0; i < totalAsignados; i++) {
                                vector<int> nCamino = caminos.front().first;
                                vector<int> nMetadata = caminos.front().second;
                                caminos.pop_front();
                                nMetadata[0] = costoOptimo;
                                MPI_Send(&nCamino[0], nVertices, MPI_INT, slavesAsignados[i], 1, MPI_COMM_WORLD);
                                MPI_Send(&nMetadata[0], 4, MPI_INT, slavesAsignados[i], 1, MPI_COMM_WORLD);
                            }
                        }
                        else {
                            // orden 5: No seguir
                            continue;
                        }
                    }
                }
                // Indicar status libre del proceso y notificar al master
                orden = 3;
                MPI_Send(&orden, 1, MPI_INT, master, orden, MPI_COMM_WORLD);
            }
        }
        MPI_Finalize();
    }
    return 0;
}