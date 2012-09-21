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

#ifdef FIX_CLASS

FixStyle(mesh/surface/stress/servo,FixMeshSurfaceStressServo)

#else

#ifndef LMP_FIX_MESH_SURFACE_STRESS_SERVO_H
#define LMP_FIX_MESH_SURFACE_STRESS_SERVO_H

#include "fix.h"
#include "input.h"
#include "math.h"
#include "fix_mesh_surface_stress.h"

namespace LAMMPS_NS {

class FixMeshSurfaceStressServo : public FixMeshSurfaceStress {

    public:

      FixMeshSurfaceStressServo(class LAMMPS *, int, char **);
      virtual ~FixMeshSurfaceStressServo();

      virtual void post_create();

      void init();
      int setmask();

      void initial_integrate(int vflag);
      void final_integrate();

    private:

      void init_defaults();
      void error_checks();

      void limit_vel();
      void update_mass();
      void set_v_node();

      // properties of mesh

      VectorContainer<double,3> &xcm_;
      VectorContainer<double,3> &vcm_;
      VectorContainer<double,3> &xcm_orig_;

      // servo settings

      double vel_max_, acc_max_, mass_, f_servo_;
      double f_servo_vec_[3];

      // timesteps and flags for integration

      double dtf_,dtv_,dtfm_;
      VectorContainer<bool,3> &fflag_;

      // velocity for each node

      MultiVectorContainer<double,3,3> &v_;

}; //end class

}

#endif
#endif
