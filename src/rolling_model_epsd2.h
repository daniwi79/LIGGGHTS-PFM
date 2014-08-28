/* ----------------------------------------------------------------------
   LIGGGHTS - LAMMPS Improved for General Granular and Granular Heat
   Transfer Simulations

   LIGGGHTS is part of the CFDEMproject
   www.liggghts.com | www.cfdem.com

   Christoph Kloss, christoph.kloss@cfdem.com
   Copyright 2009-2012 JKU Linz
   Copyright 2012-     DCS Computing GmbH, Linz

   LIGGGHTS is based on LAMMPS
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   This software is distributed under the GNU General Public License.

   See the README file in the top-level directory.
------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------
   Contributing authors:
   Christoph Kloss (JKU Linz, DCS Computing GmbH, Linz)
   Andreas Aigner (JKU Linz)
------------------------------------------------------------------------- */
#ifdef ROLLING_MODEL
ROLLING_MODEL(ROLLING_EPSD2,epsd2,3)
#else
#ifndef ROLLING_MODEL_EPSD2_H_
#define ROLLING_MODEL_EPSD2_H_
#include "contact_models.h"
#include <algorithm>
#include "math.h"
#include "domain.h"
#include "math_extra_liggghts.h"

namespace LIGGGHTS {
namespace ContactModels
{
  using namespace LAMMPS_NS;

  template<>
  class RollingModel<ROLLING_EPSD2> : protected Pointers
  {
  public:
    static const int MASK = CM_CONNECT_TO_PROPERTIES | CM_COLLISION | CM_NO_COLLISION;

    RollingModel(class LAMMPS * lmp, IContactHistorySetup * hsetup) : Pointers(lmp), coeffRollFrict(NULL)
    {
      history_offset = hsetup->add_history_value("r_torquex_old", "1");
      hsetup->add_history_value("r_torquey_old", "1");
      hsetup->add_history_value("r_torquez_old", "1");
      /*NL*/ if(comm->me == 0) fprintf(screen, "EPSD2 loaded\n");
    }

    void registerSettings(Settings&) {}

    void connectToProperties(PropertyRegistry & registry) {
      registry.registerProperty("coeffRollFrict", &MODEL_PARAMS::createCoeffRollFrict);
      registry.connect("coeffRollFrict", coeffRollFrict,"rolling_model epsd2");

      //NP modified C.K.
      // error checks on coarsegraining
      if(force->cg_active())
        error->cg(FLERR,"rolling model epsd2");
    }

    void collision(CollisionData & cdata, ForceData & i_forces, ForceData & j_forces) //NP modified C.K.
    {
      double r_torque[3];
      vectorZeroize3D(r_torque);

      if(cdata.touch) *cdata.touch |= TOUCH_ROLLING_MODEL;

      if(cdata.is_wall) {
        const double wr1 = cdata.wr1;
        const double wr2 = cdata.wr2;
        const double wr3 = cdata.wr3;
        const double radius = cdata.radi;

        calcRollTorque(r_torque,cdata,radius,wr1,wr2,wr3);

      } else {
        double wr_roll[3];

        const int i = cdata.i;
        const int j = cdata.j;

        const double radi = cdata.radi;
        const double radj = cdata.radj;
        const double reff=radi*radj/(radi+radj);
        const double * const * const omega = atom->omega;

        // relative rotational velocity
        vectorSubtract3D(omega[i],omega[j],wr_roll);

        calcRollTorque(r_torque,cdata,reff,wr_roll[0],wr_roll[1],wr_roll[2]);

      }

      i_forces.delta_torque[0] -= r_torque[0];
      i_forces.delta_torque[1] -= r_torque[1];
      i_forces.delta_torque[2] -= r_torque[2];
      j_forces.delta_torque[0] += r_torque[0];
      j_forces.delta_torque[1] += r_torque[1];
      j_forces.delta_torque[2] += r_torque[2];
    }

    void noCollision(ContactData & cdata, ForceData&, ForceData&)
    {
      if(cdata.touch) *cdata.touch &= ~TOUCH_ROLLING_MODEL;
      double * const c_history = &cdata.contact_history[history_offset];
      c_history[0] = 0.0; // this is the r_torque_old
      c_history[1] = 0.0; // this is the r_torque_old
      c_history[2] = 0.0; // this is the r_torque_old
    }

    void beginPass(CollisionData&, ForceData&, ForceData&){}
    void endPass(CollisionData&, ForceData&, ForceData&){}

  private:
    double ** coeffRollFrict;
    int history_offset;

    inline void calcRollTorque(double (&r_torque)[3],const CollisionData & cdata,double reff,double wr1,double wr2,double wr3) {
      double wr_n[3],wr_t[3];

      const double enx = cdata.en[0];
      const double eny = cdata.en[1];
      const double enz = cdata.en[2];

      const double dt = update->dt; //NP TODO: any transformation of the timestep needed for other unit systems?

      double * const c_history = &cdata.contact_history[history_offset]; // requires Style::TANGENTIAL == TANGENTIAL_HISTORY
      const double rmu= coeffRollFrict[cdata.itype][cdata.jtype];

      // remove normal (torsion) part of relative rotation
      // use only tangential parts for rolling torque
      const double wr_dot_delta = wr1*enx+ wr2*eny + wr3*enz;
      wr_n[0] = enx * wr_dot_delta;
      wr_n[1] = eny * wr_dot_delta;
      wr_n[2] = enz * wr_dot_delta;
      wr_t[0] = wr1 - wr_n[0];
      wr_t[1] = wr2 - wr_n[1];
      wr_t[2] = wr3 - wr_n[2];

      // spring (reff depends on wall-particle or particle-particle contact)
      const double kr = cdata.kt*reff*reff; //NP modified A.A.;

      double dr_torque[3];
      vectorScalarMult3D(wr_t,dt*kr,dr_torque);

      r_torque[0] = c_history[0] + dr_torque[0];
      r_torque[1] = c_history[1] + dr_torque[1];
      r_torque[2] = c_history[2] + dr_torque[2];

      // limit max. torque
      const double r_torque_mag = vectorMag3D(r_torque);
      const double r_torque_max = fabs(cdata.Fn)*reff*rmu;
      if(r_torque_mag > r_torque_max)
      {
        //printf("[%d] %e > %e\n", update->ntimestep, r_torque_mag, r_torque_max);
        const double factor = r_torque_max / r_torque_mag;

        r_torque[0] *= factor;
        r_torque[1] *= factor;
        r_torque[2] *= factor;

      } else {
        // nothing to do
      }

      // save rolling torque due to spring
      c_history[0] = r_torque[0];
      c_history[1] = r_torque[1];
      c_history[2] = r_torque[2];

      // dashpot only for the original epsd model

    }
  };
}
}
#endif // ROLLING_MODEL_EPSD_H_
#endif
