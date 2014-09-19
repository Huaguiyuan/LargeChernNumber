/*
 * edge.h
 *
 *  Created on: Sep 2, 2014
 *      Author: Lin Dong
 */
#ifndef EDGE_H_
#define EDGE_H_
#include "stdcpp.h"
class cChern {
private:
  int _argc;
  char** _argv;
  double _Eb, _h, _v;
  double _mu, _T;
  int _PMAX, pblock,pblock4, _MomentumSpaceCutoff, _NKX, _NKX2;
  VectorXd _bdg_E;
  MatrixXcd _bdg_V;
  MatrixXcd _bdg_H;
  complex<double> _chern;
  double kmax;
  double* gauss_k, *gauss_w_k;
public:
  cChern(const sPara& para, const sPhys& phys,  int argc, char** argv)
    :_argc(argc), _argv(argv),
    _Eb(para.t), _h(para.h), _v(para.v),
    _mu(phys.mu),_T(phys.T),
    // TODO: modify frequency cutoff
    _PMAX(4), // number of frequency cutoff for time expansion
    pblock(4),
    // TODO: modify mmtn space cutoff for the bulk system
    pblock4(4),
    _NKX(5000),
    _NKX2(_NKX*_NKX),
    _bdg_E(pblock4),
    _bdg_V(pblock4,pblock4),
    _bdg_H(pblock4,pblock4),_chern(1.0,0.0),
    kmax(10.0){}
    ~cChern(){
      delete []gauss_k;
      delete []gauss_w_k;}
    int compute_count(int,int);
    void distribution();
    void construction();
    void update(int);
    void update_kxky(double, double);
};
#endif /* EDGE_H_ */
