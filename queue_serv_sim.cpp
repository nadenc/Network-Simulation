#include <iostream>
#include <stdlib.h>
#include <math.h>

using namespace std;

double genRand(int factor){ 			// U(0,1) generator
	return -log( 1 - ( (long double) rand() / (long double) RAND_MAX ) ) / factor;
}

int queue_sim(double rho, double time, int QUEUE_MAX){
	
	//const double rho = 0.95;						// utilization of queue
	const double length = 12000;					// length of packet in bits
	const double C = 1000000;						// link rate in bps
	double lambda = rho*C/length;					// packets arriving per second
	const double alpha = 10*lambda;					// observation events per second
	//const double time = 10;							// simulation time in seconds
	const int PACKETS = lambda*time*1.2;			// approx number of packets + 20% headroom
	const int OBS = ((lambda+alpha)*time*1.2);		// approx number of observation events + 20% headroom
	
	double A[PACKETS];					// packet arrival times
	double D[PACKETS];					// packet departure times
	double O[OBS];						// observation events
	double ES[OBS+PACKETS*2][3];		// event scheduler
	
	double ET = 0; 						// average sojourn time
	double p_idle = 0;					// proportion of server idle time
	double p_loss = 0;					// proportion of dropped packets
	
	int i = 0;							// for counters
	
	int nA = 0, nO = 0, nD = 0;			// initialize number of arrivals, departures and observers
	
	//const int QUEUE_MAX = 9999;			// queue max size
	int queue = 0;						// packets in queue
	int queue_drop = 0;					// offset to account for dropped packets
	
	int serv_busy = 0;					// server busy flag
	
	srand(1);							// random number seed
	
	A[0] = genRand(lambda);				// set first packet arrival
	nA = 1;
	
	while (A[i] < time){ 					// packet arrival generator
		i++;		
		A[i] = A[i-1] + genRand(lambda);	// generate packet arrival if within sim time
		if (A[i] < time) nA++;				// increment number of arrivals if within sim time
	}
	
	for (i=0;i<=nA;i++){								// queue/server state machine
		
		if (serv_busy){									// check if server is still busy
			
			while ( D[i-queue_drop-1] == -1 ){			// if the packet was dropped skip it
				queue_drop++;
			}
														// update queue
			while ( A[i] > ( D[i-queue-queue_drop-1]) ) {
				if (queue == 0){
					serv_busy = 0;						// if packet being served is done clear flag
					break;								// break out of while loop
				}
				else{
					queue--;
				}
			}
			
			if (queue == QUEUE_MAX){					// queue full, drop packet
				D[i] = -1;								// -1 indicates packet dropped
			}
			else if (serv_busy){						// if stil serving
				queue++;								// add packet to queue
				D[i] = ( D[i-queue_drop-1] + ( length / C ) );		// set packet departure time
			}
			
			queue_drop = 0;								// reset dropped packet counter
			
		}
				
		if (!serv_busy){ 								// queue empty and server ready, serve packet immediately
			D[i] = ( A[i] + ( length / C ) );			// set packet departure time
			serv_busy = 1;								// set server busy
		}
		
		if (D[i] != -1){								// add up sojourn time of delivered packets
			ET += (D[i]-A[i]);
		}
		
		//cout << "Packet: " << i << "\tArr: " << A[i] << "\tDep: " << D[i] << "\tQueue: " << queue << endl;
	}
	
	O[0] = genRand(alpha);					// set first observer
	nO = 1;
	
	i = 0;									// reset counter
	
	while (O[i] < D[nA]){					// observation and packet arrival generator
		i++;
		O[i] = O[i-1] + genRand(alpha);		// generate observation times
		nO++;
	}
	
	int pA = 0, pD = 0, pO = 0; 			// track position of events for scheduling
	
	int num_events = (nO + 2*nA); 			// set max number of events
	
	i = 0;									// reset counter

	while (i<num_events){					// populate event scheduler
	
		if (pD == nA){						// all packets accounted for rest are observers
			ES[i][0] = O[pO];
			ES[i][1] = 0;
			ES[i][2] = pO;
			i++;
			pO++;
		}
		
		else if (pA == nA){					// packets in system still
			
			if (D[pD] == -1){				// check for dropped packets
				num_events--;				// reduce number of events
				pD++;
			}
											// packet delivered event
			else if (min(D[pD], O[pO]) == D[pD]){
				ES[i][0] = D[pD];
				ES[i][1] = 2;
				ES[i][2] = pD;
				nD++;
				pD++;
				i++;
			}
			
			else{							// observer event
				ES[i][0] = O[pO];
				ES[i][1] = 0;
				ES[i][2] = pO;
				if (pD==pA) p_idle++;		// if all packets have been passes through system is idle
				pO++;
				i++;
			}
		}
		
		else{								// waiting for everything
		
			if (D[pD] == -1){				// check for dropped packets
				num_events--;				// reduce number of events
				pD++;
			}
											// packet arrival event
			else if (min(min(A[pA],D[pD]),O[pO]) == A[pA] && pA < nA){
				ES[i][0] = A[pA];
				ES[i][1] = 1;
				ES[i][2] = pA;
				pA++;
				i++;
			}
											// observer event
			else if (min(min(A[pA],D[pD]),O[pO]) == O[pO] && pO < nO){
				ES[i][0] = O[pO];
				ES[i][1] = 0;
				ES[i][2] = pO;
				if (pD==pA) p_idle++;		// if all packets have been passes through system is idle
				pO++;
				i++;
			}
											// packet delivered event
			else if (min(min(A[pA],D[pD]),O[pO]) == D[pD]){
				ES[i][0] = D[pD];
				ES[i][1] = 2;
				ES[i][2] = pD;
				nD++;
				pD++;
				i++;
			}
		}

		//cout << "Event Type: " << ES[i-1][1] << "\tEvent Designator: " << ES[i-1][2] << "\tEvent Time: " << ES[i-1][0] << endl;
		
	}
	
	p_idle = 100 * (p_idle / nO);									// calculate percentage of time idle
	
	ET = (ET / nD);													// calculate average sojourn time of delivered packets
	
	p_loss = 100 * ( ((double) nA - (double) nD) / (double) nA);	// calculate percentage of lost packets
	
	cout << "############################################################" << endl;
	cout << "Rho: " << rho;
	cout << "\tQueue Size: " << QUEUE_MAX;
	cout << "\tSimulation Time: " << ES[num_events-1][0] << endl;
	
	cout << "Observations: " << nO << endl;
	cout << "Packet Arrivals: " << nA << endl;
	cout << "Packets Delivered : " << nD << endl;
	cout << "Packets Dropped: " << nA-nD << endl;
	
	cout << "Average sojourn time: " << ET << endl;
	cout << "Idle %: " << p_idle << endl;
	cout << "Loss %: " << p_loss << endl << endl;

}

int main(){
	double j = 0.25;			// rho
	int size_of_queue = 2;		// queue length
	double sim_time = 10;		// simulation time
	
	while (j <= 0.95){			// simulation loop
		queue_sim(j,sim_time,size_of_queue);
		j += 0.1;
	}
	
}	

/*
	//double mean = 0;					// initialize mean
	//double variance = 0;				// initialize variance
	
	for (i = 0; i < PACKETS; i++){				// calculate variance of array U
	
		variance += ( ( U[i] - mean ) * ( U[i] - mean ) / PACKETS );	// calculate U variance
		
	}
*/
	//cout << "Mean: " << mean << endl;
	//cout << "Variance: " << variance << endl;
