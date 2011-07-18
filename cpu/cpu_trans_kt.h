#ifndef CPU_TRANS_H
#define CPU_TRANS_H

#include <vector>
#include "../definitions.h"
#include "../common.h"
#include "../cell_spatial.h"
#include "../project.h"
#include "cpu_common.h"

template<typename T> T trIndex(const T& i,const T& j,const T& k) {return k*WID2+j*WID+i;}
template<typename T> T isBoundary(const T& STATE,const T& BND) {return STATE & BND;}

template<typename T> T spatDerivs1(const T& xl2,const T& xl1,const T& xcc,const T& xr1,const T& xr2) {
   return superbee<T>(xl1,xcc,xr1);
}

template<typename T> T spatDerivs2(const T& xl2,const T& xl1,const T& xcc,const T& xr1,const T& xr2) {
   return convert<T>(0.0);
}

template<typename REAL,typename UINT,typename CELL> void cpu_calcSpatDerivs(CELL& cell,const UINT& BLOCK,const std::vector<const CELL*>& nbrPtrs) {
   REAL* const d1x   = cell.cpu_d1x + BLOCK*SIZE_DERIV;
   REAL* const d2x   = cell.cpu_d2x + BLOCK*SIZE_DERIV;
   REAL* const d1y   = cell.cpu_d1y + BLOCK*SIZE_DERIV;
   REAL* const d2y   = cell.cpu_d2y + BLOCK*SIZE_DERIV;
   REAL* const d1z   = cell.cpu_d1z + BLOCK*SIZE_DERIV;
   REAL* const d2z   = cell.cpu_d2z + BLOCK*SIZE_DERIV;
   const REAL* const cellParams = cell.cpu_cellParams;
   const REAL* const blockParams = cell.cpu_blockParams + BLOCK*SIZE_BLOCKPARAMS;

   REAL xl2,xl1,xcc,xr1,xr2;
   xl2 = 0.0;
   xr2 = 0.0;
   // *************** X DERIVATIVES ******************
   for (UINT k=0; k<WID; ++k) for (UINT j=0; j<WID; ++j) for (UINT i=0; i<WID; ++i) {
      xcc = cell.cpu_avgs[BLOCK*SIZE_VELBLOCK + trIndex(i,j,k)];
      // Get vol.avg. from -x neighbour, or calculate it using a boundary function:
      if (nbrPtrs[0] == NULL) {
	 xl1 = calcBoundVolAvg(i,j,k,cellParams,blockParams,xcc,0,true);
      } else {
	 xl1 = nbrPtrs[0]->cpu_avgs[BLOCK*SIZE_VELBLOCK + trIndex(i,j,k)];
      }
      // Get vol.avg. from +x neighbour, or calculate it using a boundary function:
      if (nbrPtrs[1] == NULL) {
	 xr1 = calcBoundVolAvg(i,j,k,cellParams,blockParams,xcc,0,false);
      } else {
	 xr1 = nbrPtrs[1]->cpu_avgs[BLOCK*SIZE_VELBLOCK + trIndex(i,j,k)];
      }
      d1x[trIndex(i,j,k)] = spatDerivs1(xl2,xl1,xcc,xr1,xr2);
      d2x[trIndex(i,j,k)] = spatDerivs2(xl2,xl1,xcc,xr1,xr2);
   }
   // *************** Y DERIVATIVES ******************
   for (UINT k=0; k<WID; ++k) for (UINT j=0; j<WID; ++j) for (UINT i=0; i<WID; ++i) {
      xcc = cell.cpu_avgs[BLOCK*SIZE_VELBLOCK + trIndex(i,j,k)];
      // Get vol.avg. from -y neighbour, or calculate it using a boundary function:
      if (nbrPtrs[2] == NULL) {
	 xl1 = calcBoundVolAvg(i,j,k,cellParams,blockParams,xcc,1,true);
      } else {
	 xl1 = nbrPtrs[2]->cpu_avgs[BLOCK*SIZE_VELBLOCK + trIndex(i,j,k)];
      }
      // Get vol.avg. from +y neighbour, or calculate it using a boundary function:
      if (nbrPtrs[3] == NULL) {
	 xr1 = calcBoundVolAvg(i,j,k,cellParams,blockParams,xcc,1,false);
      } else {
	 xr1 = nbrPtrs[3]->cpu_avgs[BLOCK*SIZE_VELBLOCK + trIndex(i,j,k)];
      }
      d1y[trIndex(i,j,k)] = spatDerivs1(xl2,xl1,xcc,xr1,xr2);
      d2y[trIndex(i,j,k)] = spatDerivs2(xl2,xl1,xcc,xr1,xr2);
   }
   // *************** Z DERIVATIVES ******************
   for (UINT k=0; k<WID; ++k) for (UINT j=0; j<WID; ++j) for (UINT i=0; i<WID; ++i) {
      xcc = cell.cpu_avgs[BLOCK*SIZE_VELBLOCK + trIndex(i,j,k)];
      // Get vol.avg. from -z neighbour, or calculate it using a boundary function:
      if (nbrPtrs[4] == NULL) {
	 xl1 = calcBoundVolAvg(i,j,k,cellParams,blockParams,xcc,2,true);
      } else {
	 xl1 = nbrPtrs[4]->cpu_avgs[BLOCK*SIZE_VELBLOCK + trIndex(i,j,k)];
      }
      // Get vol.avg. from +z neighbour, or calculate it using a boundary function:
      if (nbrPtrs[5] == NULL) {
	 xr1 = calcBoundVolAvg(i,j,k,cellParams,blockParams,xcc,2,false);
      } else {
	 xr1 = nbrPtrs[5]->cpu_avgs[BLOCK*SIZE_VELBLOCK + trIndex(i,j,k)];
      }
      d1z[trIndex(i,j,k)] = spatDerivs1(xl2,xl1,xcc,xr1,xr2);
      d2z[trIndex(i,j,k)] = spatDerivs2(xl2,xl1,xcc,xr1,xr2);
   }
}

template<typename REAL,typename UINT,typename CELL> void cpu_calcSpatFluxesX(CELL& cell,const UINT& BLOCK,const std::vector<const CELL*>& nbrPtrs) {
   const REAL* const avgs = cell.cpu_avgs + BLOCK*SIZE_VELBLOCK;
   const REAL* const d1x  = cell.cpu_d1x  + BLOCK*SIZE_DERIV;
   const REAL* const d2x  = cell.cpu_d2x  + BLOCK*SIZE_DERIV;
   REAL* const fx         = cell.cpu_fx   + BLOCK*SIZE_FLUXS;
   const REAL* const cellParams  = cell.cpu_cellParams;
   const REAL* const blockParams = cell.cpu_blockParams + BLOCK*SIZE_BLOCKPARAMS;
   
   REAL avg_pos,d1_pos,d2_pos;
   REAL avg_neg,d1_neg,d2_neg;
   for (UINT k=0; k<WID; ++k) for (UINT j=0; j<WID; ++j) for (UINT i=0; i<WID; ++i) {
      // Reconstruct volume average at positive side of x-face:
      avg_pos = avgs[trIndex(i,j,k)];
      d1_pos  = d1x[trIndex(i,j,k)];
      d2_pos  = d2x[trIndex(i,j,k)];
      avg_pos = reconstruct_pos(avg_pos,d1_pos,d2_pos);
      // Reconstruct volume average at negative side of x-face.
      // Fetch vol.avg and derivatives from -x neighbour, or calculate them using a boundary func.:
      if (nbrPtrs[0] == NULL) {
	 avg_neg = calcBoundVolAvg(i,j,k,cellParams,blockParams,avg_pos,0,true);
	 d1_neg = 0.0;
	 d2_neg = 0.0;
      } else {
	 avg_neg = nbrPtrs[0]->cpu_avgs[BLOCK*SIZE_VELBLOCK + trIndex(i,j,k)];
	 d1_neg  = nbrPtrs[0]->cpu_d1x[BLOCK*SIZE_DERIV + trIndex(i,j,k)];
	 d2_neg  = nbrPtrs[0]->cpu_d2x[BLOCK*SIZE_DERIV + trIndex(i,j,k)];
      }
      avg_neg = reconstruct_neg(avg_neg,d1_neg,d2_neg);
      // Calculate x-flux:
      fx[trIndex(i,j,k)] = spatialFluxX(i,avg_neg,avg_pos,cell.cpu_blockParams+BLOCK*SIZE_BLOCKPARAMS);
   }
}

template<typename REAL,typename UINT,typename CELL> void cpu_calcSpatFluxesY(CELL& cell,const UINT& BLOCK,const std::vector<const CELL*>& nbrPtrs) {
   const REAL* const avgs = cell.cpu_avgs + BLOCK*SIZE_VELBLOCK;
   const REAL* const d1y  = cell.cpu_d1y  + BLOCK*SIZE_DERIV;
   const REAL* const d2y  = cell.cpu_d2y  + BLOCK*SIZE_DERIV;
   REAL* const fy         = cell.cpu_fy   + BLOCK*SIZE_FLUXS;
   const REAL* const cellParams  = cell.cpu_cellParams;
   const REAL* const blockParams = cell.cpu_blockParams + BLOCK*SIZE_BLOCKPARAMS;
   
   REAL avg_pos,d1_pos,d2_pos;
   REAL avg_neg,d1_neg,d2_neg;
   for (UINT k=0; k<WID; ++k) for (UINT j=0; j<WID; ++j) for (UINT i=0; i<WID; ++i) {
      // Reconstruct volume average at positive side of y-face:
      avg_pos = avgs[trIndex(i,j,k)];
      d1_pos  = d1y[trIndex(i,j,k)];
      d2_pos  = d2y[trIndex(i,j,k)];
      avg_pos = reconstruct_pos(avg_pos,d1_pos,d2_pos);
      // Reconstruct volume average at negative side of y-face.
      // Fetch vol.avg and derivatives from -y neighbour, or calculate them using a boundary func.:
      if (nbrPtrs[2] == NULL) {
	 avg_neg = calcBoundVolAvg(i,j,k,cellParams,blockParams,avg_pos,1,true);
	 d1_neg = 0.0;
	 d2_neg = 0.0;
      } else {
	 avg_neg = nbrPtrs[2]->cpu_avgs[BLOCK*SIZE_VELBLOCK + trIndex(i,j,k)];
	 d1_neg  = nbrPtrs[2]->cpu_d1y[BLOCK*SIZE_DERIV + trIndex(i,j,k)];
	 d2_neg  = nbrPtrs[2]->cpu_d2y[BLOCK*SIZE_DERIV + trIndex(i,j,k)];
      }
      avg_neg = reconstruct_neg(avg_neg,d1_neg,d2_neg);
      // Calculate y-flux:
      fy[trIndex(i,j,k)] = spatialFluxY(j,avg_neg,avg_pos,cell.cpu_blockParams+BLOCK*SIZE_BLOCKPARAMS);
   }
}
      
template<typename REAL,typename UINT,typename CELL> void cpu_calcSpatFluxesZ(CELL& cell,const UINT& BLOCK,const std::vector<const CELL*>& nbrPtrs) {
   const REAL* const avgs = cell.cpu_avgs + BLOCK*SIZE_VELBLOCK;
   const REAL* const d1z  = cell.cpu_d1z  + BLOCK*SIZE_DERIV;
   const REAL* const d2z  = cell.cpu_d2z  + BLOCK*SIZE_DERIV;
   REAL* const fz         = cell.cpu_fz   + BLOCK*SIZE_FLUXS;
   const REAL* const cellParams  = cell.cpu_cellParams;
   const REAL* const blockParams = cell.cpu_blockParams + BLOCK*SIZE_BLOCKPARAMS;
   
   REAL avg_pos,d1_pos,d2_pos;
   REAL avg_neg,d1_neg,d2_neg;
   for (UINT k=0; k<WID; ++k) for (UINT j=0; j<WID; ++j) for (UINT i=0; i<WID; ++i) {
      // Reconstruct volume average at positive side of z-face:
      avg_pos = avgs[trIndex(i,j,k)];
      d1_pos  = d1z[trIndex(i,j,k)];
      d2_pos  = d2z[trIndex(i,j,k)];
      avg_pos = reconstruct_pos(avg_pos,d1_pos,d2_pos);
      // Reconstruct volume average at negative side of z-face.
      // Fetch vol.avg and derivatives from -z neighbour, or calculate them using a boundary func.:
      if (nbrPtrs[4] == NULL) {
	 avg_neg = calcBoundVolAvg(i,j,k,cellParams,blockParams,avg_pos,2,true);
	 d1_neg = 0.0;
	 d2_neg = 0.0;
      } else {
	 avg_neg = nbrPtrs[4]->cpu_avgs[BLOCK*SIZE_VELBLOCK + trIndex(i,j,k)];
	 d1_neg  = nbrPtrs[4]->cpu_d1z[BLOCK*SIZE_DERIV + trIndex(i,j,k)];
	 d2_neg  = nbrPtrs[4]->cpu_d2z[BLOCK*SIZE_DERIV + trIndex(i,j,k)];
      }
      avg_neg = reconstruct_neg(avg_neg,d1_neg,d2_neg);
      // Calculate z-flux:
      fz[trIndex(i,j,k)] = spatialFluxZ(k,avg_neg,avg_pos,cell.cpu_blockParams+BLOCK*SIZE_BLOCKPARAMS);
   }
}

template<typename REAL,typename UINT,typename CELL> void cpu_propagateSpat(CELL& cell,const UINT& BLOCK,const std::vector<const CELL*>& nbrPtrs,const REAL& DT) {
   REAL* const avgs = cell.cpu_avgs + BLOCK*SIZE_VELBLOCK;
   const REAL* const fx = cell.cpu_fx + BLOCK*SIZE_FLUXS;
   const REAL* const fy = cell.cpu_fy + BLOCK*SIZE_FLUXS;
   const REAL* const fz = cell.cpu_fz + BLOCK*SIZE_FLUXS;
   const REAL* const d1x = cell.cpu_d1x + BLOCK*SIZE_DERIV;
   const REAL* const d1y = cell.cpu_d1y + BLOCK*SIZE_DERIV;
   const REAL* const d1z = cell.cpu_d1z + BLOCK*SIZE_DERIV;
   REAL* const cellParams =  cell.cpu_cellParams;
   const REAL* const blockParams = cell.cpu_blockParams + BLOCK*SIZE_BLOCKPARAMS;
   
   const REAL DTDX = DT / cellParams[CellParams::DX];
   const REAL DTDY = DT / cellParams[CellParams::DY];
   const REAL DTDZ = DT / cellParams[CellParams::DZ];
   const REAL DV3 = blockParams[BlockParams::DVX]*blockParams[BlockParams::DVY]*blockParams[BlockParams::DVZ];
   
   REAL avg,flux_neg,flux_pos;
   REAL sum_n   = 0.0;
   REAL sum_nvx = 0.0;
   REAL sum_nvy = 0.0;
   REAL sum_nvz = 0.0;
   for (uint k=0; k<WID; ++k) {
      const REAL VZ = blockParams[BlockParams::VZCRD] + (k+convert<REAL>(0.5))*blockParams[BlockParams::DVZ];
      for (uint j=0; j<WID; ++j) {
	 const REAL VY = blockParams[BlockParams::VYCRD] + (j+convert<REAL>(0.5))*blockParams[BlockParams::DVY];
	 for (uint i=0; i<WID; ++i) {
	    const REAL VX = blockParams[BlockParams::VXCRD] + (i+convert<REAL>(0.5))*blockParams[BlockParams::DVX];
	    // Calculate the contribution coming from x-fluxes:
	    flux_neg = fx[trIndex(i,j,k)];
	    // Get positive face flux from +x neighbour, or calculate it using a bound.func.:
	    if (nbrPtrs[1] == NULL) {	
	       flux_pos = spatialFluxX(i,reconstruct_neg(avgs[trIndex(i,j,k)],d1x[trIndex(i,j,k)],convert<REAL>(0.0)),
				       reconstruct_pos(calcBoundVolAvg(i,j,k,cellParams,blockParams,avgs[trIndex(i,j,k)],0,false),
						       convert<REAL>(0.0),convert<REAL>(0.0)),blockParams);
	    } else {
	       flux_pos = nbrPtrs[1]->cpu_fx[BLOCK*SIZE_FLUXS + trIndex(i,j,k)];
	    }
	    avg = (flux_neg - flux_pos)*DTDX;
	    // Calculate the contribution coming from y-fluxes:
	    flux_neg = fy[trIndex(i,j,k)];
	    // Get positive face flux from +y neighbour, or calculate it using a bound.func.:
	    if (nbrPtrs[3] == NULL) {
	       flux_pos = spatialFluxY(j,reconstruct_neg(avgs[trIndex(i,j,k)],d1y[trIndex(i,j,k)],convert<REAL>(0.0)),
				       reconstruct_pos(calcBoundVolAvg(i,j,k,cellParams,blockParams,avgs[trIndex(i,j,k)],1,false),
						       convert<REAL>(0.0),convert<REAL>(0.0)),blockParams);
	       
	    } else {
	       flux_pos = nbrPtrs[3]->cpu_fy[BLOCK*SIZE_FLUXS + trIndex(i,j,k)];
	    }
	    avg += (flux_neg - flux_pos)*DTDY;
	    // Calculate the contribution coming from z-fluxes:
	    flux_neg = fz[trIndex(i,j,k)];
	    // Get positive face flux from +z neighbour, or calculate it using a bound.func.:
	    if (nbrPtrs[5] == NULL) {
	       flux_pos = spatialFluxZ(k,reconstruct_neg(avgs[trIndex(i,j,k)],d1z[trIndex(i,j,k)],convert<REAL>(0.0)),
				       reconstruct_pos(calcBoundVolAvg(i,j,k,cellParams,blockParams,avgs[trIndex(i,j,k)],2,false),
						       convert<REAL>(0.0),convert<REAL>(0.0)),blockParams);
	    } else {
	       flux_pos = nbrPtrs[5]->cpu_fz[BLOCK*SIZE_FLUXS + trIndex(i,j,k)];
	    }
	    avg += (flux_neg - flux_pos)*DTDZ;
	    // Store new volume average:
	    avgs[trIndex(i,j,k)] += avg;
	    // Calculate contribution to spatial cell velocity moments:
	    sum_n   += avgs[trIndex(i,j,k)];
	    sum_nvx += avgs[trIndex(i,j,k)] * VX;
	    sum_nvy += avgs[trIndex(i,j,k)] * VY;
	    sum_nvz += avgs[trIndex(i,j,k)] * VZ;
	 }
      }
   }
   
   // Add contributions to spatial cell velocity moments. These updates need 
   // to be atomic (OpenMP):
   #pragma omp atomic
      cellParams[CellParams::RHO  ] += sum_n  *DV3;
   #pragma omp atomic
      cellParams[CellParams::RHOVX] += sum_nvx*DV3;
   #pragma omp atomic
      cellParams[CellParams::RHOVY] += sum_nvy*DV3;
   #pragma omp atomic
      cellParams[CellParams::RHOVZ] += sum_nvz*DV3;
}
      
#endif
      