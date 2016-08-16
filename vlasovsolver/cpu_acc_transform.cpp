/*
 * This file is part of Vlasiator.
 * 
 * Copyright 2015 Finnish Meteorological Institute
 * 
 */

#include "../object_wrapper.h"
#include "cpu_moments.h"
#include "cpu_acc_transform.hpp"

using namespace std;
using namespace spatial_cell;
using namespace Eigen;



/*!
  Compute max timestep for vlasov acceleration for the particular population
  in one spatial cell.

 * @param spatial_cell Spatial cell containing the accelerated population.
 * @param popID ID of the accelerated particle species.
*/
void updateAccelerationMaxdt(
   SpatialCell* spatial_cell,
   const int& popID) 
{
   if (Parameters::propagatePotential == true) {
      #warning Electric acceleration works for Poisson only atm
      spatial_cell->set_max_v_dt(popID,numeric_limits<Real>::max());
   }
   else {
      const Real Bx = spatial_cell->parameters[CellParams::BGBXVOL]+spatial_cell->parameters[CellParams::PERBXVOL];
      const Real By = spatial_cell->parameters[CellParams::BGBYVOL]+spatial_cell->parameters[CellParams::PERBYVOL];
      const Real Bz = spatial_cell->parameters[CellParams::BGBZVOL]+spatial_cell->parameters[CellParams::PERBZVOL];
      const Eigen::Matrix<Real,3,1> B(Bx,By,Bz);
      const Real B_mag = B.norm() + 1e-30;      
      const Real gyro_period = 2 * M_PI * getObjectWrapper().particleSpecies[popID].mass
         / (getObjectWrapper().particleSpecies[popID].charge * B_mag);

      // Set maximum timestep limit for this cell, based on a maximum allowed rotation angle
      spatial_cell->set_max_v_dt(popID,fabs(gyro_period)*(P::maxSlAccelerationRotation/360.0));
   }
}


/*!
 Compute transform during on timestep, and update the bulk velocity of the
 cell
 * @param spatial_cell Spatial cell containing the accelerated population.
 * @param popID ID of the accelerated particle species.
 * @param dt Time step of one subcycle.
*/

Eigen::Transform<Real,3,Eigen::Affine> compute_acceleration_transformation(
        SpatialCell* spatial_cell,
        const int& popID,
        const Real& dt) {
   // total field
   const Real Bx = spatial_cell->parameters[CellParams::BGBXVOL]+spatial_cell->parameters[CellParams::PERBXVOL];
   const Real By = spatial_cell->parameters[CellParams::BGBYVOL]+spatial_cell->parameters[CellParams::PERBYVOL];
   const Real Bz = spatial_cell->parameters[CellParams::BGBZVOL]+spatial_cell->parameters[CellParams::PERBZVOL];

   // perturbed field
   //const Real perBx = spatial_cell->parameters[CellParams::PERBXVOL];
   //const Real perBy = spatial_cell->parameters[CellParams::PERBYVOL];
   //const Real perBz = spatial_cell->parameters[CellParams::PERBZVOL];   

   // read in derivatives need for curl of B (only perturbed, curl of background field is always 0!)
   const Real dBXdy = spatial_cell->derivativesBVOL[bvolderivatives::dPERBXVOLdy]/spatial_cell->parameters[CellParams::DY];
   const Real dBXdz = spatial_cell->derivativesBVOL[bvolderivatives::dPERBXVOLdz]/spatial_cell->parameters[CellParams::DZ];
   const Real dBYdx = spatial_cell->derivativesBVOL[bvolderivatives::dPERBYVOLdx]/spatial_cell->parameters[CellParams::DX];

   const Real dBYdz = spatial_cell->derivativesBVOL[bvolderivatives::dPERBYVOLdz]/spatial_cell->parameters[CellParams::DZ];
   const Real dBZdx = spatial_cell->derivativesBVOL[bvolderivatives::dPERBZVOLdx]/spatial_cell->parameters[CellParams::DX];
   const Real dBZdy = spatial_cell->derivativesBVOL[bvolderivatives::dPERBZVOLdy]/spatial_cell->parameters[CellParams::DY];

   const Eigen::Matrix<Real,3,1> B(Bx,By,Bz);
   Eigen::Matrix<Real,3,1> unit_B(B.normalized());

   // If B equals zero then gyro_period and unit_B are NAN.
   // Guard against that by adding epsilons:
   const Real B_mag = B.norm() + 1e-30;
   if (B_mag < 1e-28) {
      unit_B(0,0) = 0; unit_B(1,0) = 0; unit_B(2,0) = 1;
   }

   const Real gyro_period 
     = 2 * M_PI * getObjectWrapper().particleSpecies[popID].mass
     / (getObjectWrapper().particleSpecies[popID].charge * B_mag);

   // scale rho for hall term, if user requests
   const Real EPSILON = 1e10 * numeric_limits<Real>::min();
   const Real rho = spatial_cell->parameters[CellParams::RHO_V] + EPSILON;
   const Real hallRho =  (rho <= Parameters::hallMinimumRho ) ? Parameters::hallMinimumRho : rho ;
   
   #warning This should be electron charge density!
   const Real hallPrefactor = 1.0 / (physicalconstants::MU_0 * hallRho * physicalconstants::CHARGE );

   Eigen::Matrix<Real,3,1> bulk_velocity(spatial_cell->parameters[CellParams::RHOVX_V]/rho,
                                         spatial_cell->parameters[CellParams::RHOVY_V]/rho,
                                         spatial_cell->parameters[CellParams::RHOVZ_V]/rho);

   // compute total transformation
   Transform<Real,3,Affine> total_transform(Matrix<Real, 4, 4>::Identity()); //CONTINUE

   if (Parameters::propagatePotential == true) {
   #warning Electric acceleration works for Poisson only atm
      Real* E = &(spatial_cell->parameters[CellParams::EXVOL]);

      const Real q_per_m = getObjectWrapper().particleSpecies[popID].charge 
                         / getObjectWrapper().particleSpecies[popID].mass;
      const Real CONST = q_per_m * dt;
      total_transform(0,3) = CONST * E[0];
      total_transform(1,3) = CONST * E[1];
      total_transform(2,3) = CONST * E[2];
      return total_transform;
   } // if (Parameters::propagatePotential == true) 


   unsigned int bulk_velocity_substeps; // in this many substeps we iterate forward bulk velocity when the complete transformation is computed (0.1 deg per substep).
   bulk_velocity_substeps = fabs(dt) / (fabs(gyro_period)*(0.1/360.0)); 
   if (bulk_velocity_substeps < 1) bulk_velocity_substeps=1;

   // note, we assume q is positive (pretty good assumption though)
   const Real substeps_radians = -(2.0*M_PI*dt/fabs(gyro_period))/bulk_velocity_substeps; // how many radians each substep is.
   for (uint i=0; i<bulk_velocity_substeps; ++i) {
      // rotation origin is the point through which we place our rotation axis (direction of which is unitB).
      // first add bulk velocity (using the total transform computed this far.
      Eigen::Matrix<Real,3,1> rotation_pivot(total_transform*bulk_velocity);
      
      //include lorentzHallTerm (we should include, always)      
      rotation_pivot[0]-= hallPrefactor*(dBZdy - dBYdz);
      rotation_pivot[1]-= hallPrefactor*(dBXdz - dBZdx);
      rotation_pivot[2]-= hallPrefactor*(dBYdx - dBXdy);

#warning Is particle charge sign taken correctly into account here?
      
      // add to transform matrix the small rotation around  pivot
      // when added like this, and not using *= operator, the transformations
      // are in the correct order
      total_transform = Translation<Real,3>(-rotation_pivot)*total_transform;
      total_transform = AngleAxis<Real>(substeps_radians,unit_B)*total_transform;
      total_transform = Translation<Real,3>(rotation_pivot)*total_transform;
   }

   return total_transform;
}
