/*
 * edge.cpp
 *
 *  Created on: Sep 2, 2014
 *      Author: Lin Dong
 */
#include "chern.h"

void cChern::distribution(){
	int rank, size;
	  const int root = 0;
	  int *buffer = NULL;
	  Init(_argc, _argv);
	  rank = COMM_WORLD.Get_rank();
	  size = COMM_WORLD.Get_size();
	  int epp = _NKX * _NKX / size; // the amount of workload for each processor
	  int* recvB = new int[epp];
	  double* TotalEig = NULL;
	  if (rank == root){
	    buffer = new int[_NKX*_NKX];
	    for(int i = 0; i< _NKX*_NKX; ++i){
	      buffer[i] = i;
	    }
	  }
	  MPI_Scatter(buffer, epp, MPI_INT, recvB, epp, MPI_INT, root,COMM_WORLD); // evenly distributed
	  //MPI_Scatterv(buffer, epp, MPI_INT, recvB, epp, MPI_INT, root,COMM_WORLD); // unevenly distributed
	  double* localEig = new double[epp*4*pblock];
	  SelfAdjointEigenSolver<MatrixXcd> ces;
	  for (int i=0; i<epp; ++i) {
	    update(recvB[i]);
	    clock_t start = clock();
	    ces.compute(_bdg_H); // eigenvectors are also computed.
	    clock_t end = clock();
	    cout << double (end-start)/ (double) CLOCKS_PER_SEC  << endl;
	    for(int j = 0; j < 4*pblock; ++j){
	      localEig[j+i*4*pblock]=ces.eigenvalues()[j];
	    }
	    cout << "rank " << recvB[i] << " is finished out of epp "<< epp << endl;
	  }
	  if(rank == root){
	    TotalEig = new double[_NKX*_NKX*4*pblock];
	  }
	  MPI_Gather(localEig, epp*4*pblock, MPI_DOUBLE, TotalEig, epp*4*pblock, MPI_DOUBLE, root, COMM_WORLD);
	  //MPI_Gatherv(localEig, epp*4*pblock, MPI_DOUBLE, TotalEig, epp*4*pblock, MPI_DOUBLE, root, COMM_WORLD);
	  if (rank == root){
	    ofstream bdg_output;
	    bdg_output.open("spectrum_2109.OUT"); // TODO: modify output file name
	    assert(bdg_output.is_open());
	    for(int i = 0; i<size; ++i){
	    	for(int j = 0; j<epp; ++j){
	    		for(int q = 0; q<4*pblock; ++q){
	    			bdg_output << TotalEig[i*4*pblock*epp+j*4*pblock+q] << '\t';
	    		}
	    		bdg_output << endl;
	    	}
	    	bdg_output << endl;
	    }
	    bdg_output.close();
	  }
	  Finalize();
	  delete []localEig;
	  delete []TotalEig;
	  delete []buffer;
	  delete []recvB;
}

void cChern::construction(){
	update(-1); // arbitrary null construction.
}

void cChern::update(int nk){
  if (nk == -1) {
	  _bdg_H.setZero(); // This is done only once.
	  int p, q;
	  // The off-diagonal coupling introduced from time-dependent order parameter should be computed only here.
	  complex<double> Gamma2;
	  FILE *sf_inputR, *sf_inputI;
	  sf_inputR = fopen ("Rdata_2109.dat","r"); // TODO: modify input file name
	  sf_inputI = fopen ("Idata_2109.dat","r");
	  assert (sf_inputR != NULL);
	  assert (sf_inputI != NULL);
	  double dt = 0.0005;
	  double t;
	  VectorXcd Delta_t(100000);
	  Delta_t.setZero();
	  int count = 0;
	  double reD, imD;
	  while (fscanf(sf_inputR, "%lf", &reD) != EOF && fscanf(sf_inputI, "%lf", &imD) != EOF ){
		Delta_t(count) = complex<double>(reD,imD);
		//cout << Delta_t(count) << '\t' << count << endl;
	  	count++;
	  }
	  fclose (sf_inputR);
	  fclose (sf_inputI);
	  for (int i = 0; i < pblock; ++i) {
	  		p = i-_PMAX;
	  		for (int j = 0; j < pblock; ++j) {
	  			q = j-_PMAX;
	  			Gamma2 = complex<double> (0.0,0.0);
	  			t = 0.0;
	  			for (int ig = 0; ig < count; ++ig) {
					Gamma2 +=  conj(Delta_t(ig)) *
							complex<double> (cos(2*M_PI*(q-p)*t/_T),-sin(2*M_PI*(q-p)*t/_T));
					t += dt;
				}
	  			Gamma2 = Gamma2/_T*dt;
	  			//cout << Gamma2 << endl;
	  			_bdg_H(i+2*pblock,j+pblock) = Gamma2;
	  			_bdg_H(i+3*pblock,j) = -Gamma2;
	  		}
	  }
  } else {
	  // nk = nkx+ nky * NKX
	  int nkx = nk % _NKX;     // --> the modulo
	  int nky = int (nk/_NKX); // --> the floor
	  double kmax = 2.0; // TODO: modify momentum space cutoff value
	  double kx = -kmax + nkx * kmax *2.0 /(_NKX-1);
	  double ky = -kmax + nky * kmax *2.0 /(_NKX-1);
	  // This is to test the bulk spectrum periodicity.
	  update_kxky(kx,ky);

	  /* This is left for the Chern number calculation using Ming's method.
	  double dk = 0.5 * kmax * 2.0 /(_NKX-1);
	  double q1x, q2x, q1y, q2y; // This is to form a small square loop. Idea taken from Ming Gong's code.
	  for (int i = 0; i < 4; ++i) {
		switch (i) {
			case 1:
				q1x = kx - dk;
				q1y = ky - dk;
				q2x = kx + dk;
				q2y = ky - dk;
				break;
			case 2:
				q1x = kx + dk;
				q1y = ky - dk;
				q2x = kx + dk;
				q2y = ky + dk;
				break;
			case 3:
				q1x = kx + dk;
				q1y = ky + dk;
				q2x = kx - dk;
				q2y = ky + dk;
				break;
			case 4:
				q1x = kx - dk;
				q1y = ky + dk;
				q2x = kx - dk;
				q2y = ky - dk;
				break;
			default:
				break;
		}
	  }
	  update_kxky(q1x,q1y);

	  update_kxky(q2x,q2y);
	  */
  }
}

void cChern::update_kxky(double kx, double ky){
	double xi = kx*kx + ky*ky - _mu;
	int p;
	for (int i = 0; i < pblock; ++i) {
		p = i-_PMAX;
		// only lower left part of the matrix is needed for storage.
		_bdg_H(i,i) 	   = complex<double>(xi+_h+2*M_PI*p/_T,0.0);
		_bdg_H(i+pblock,i)   = complex<double>(_v*kx,_v*ky);
		_bdg_H(i+pblock,i+pblock)= complex<double>(xi-_h+2*M_PI*p/_T,0.0);
		_bdg_H(i+2*pblock,i+2*pblock) = complex<double>(-(xi+_h+2*M_PI*p/_T),0.0);
		_bdg_H(i+3*pblock,i+2*pblock) = complex<double>(_v*kx,-_v*ky);
		_bdg_H(i+3*pblock,i+3*pblock) = complex<double>(-(xi-_h+2*M_PI*p/_T),0.0);
	}
}
