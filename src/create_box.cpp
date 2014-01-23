/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#include "stdlib.h"
#include "string.h"
#include "create_box.h"
#include "atom.h"
#include "force.h"
#include "domain_wedge.h" //NP modified C.K.
#include "region.h"
#include "region_prism.h"
#include "region_wedge.h" //NP modified C.K.
#include "comm.h"
#include "update.h"
#include "error.h"

using namespace LAMMPS_NS;

/* ---------------------------------------------------------------------- */

CreateBox::CreateBox(LAMMPS *lmp) : Pointers(lmp) {}

/* ---------------------------------------------------------------------- */

void CreateBox::command(int narg, char **arg)
{
  if (narg < 2) error->all(FLERR,"Illegal create_box command"); //NP modified C.K.

  if (domain->box_exist)
    error->all(FLERR,"Cannot create_box after simulation box is defined");
  if (domain->dimension == 2 && domain->zperiodic == 0)
    error->all(FLERR,"Cannot run 2d simulation with nonperiodic Z dimension");

  domain->box_exist = 1;

  // region check

  int iregion = domain->find_region(arg[1]);
  if (iregion == -1) error->all(FLERR,"Create_box region ID does not exist");
  if (domain->regions[iregion]->bboxflag == 0)
    error->all(FLERR,"Create_box region does not support a bounding box");

  domain->regions[iregion]->init();

  // if region not prism:
  //   setup orthogonal domain
  //   set simulation domain from region extent
  // if region is prism:
  //   seutp triclinic domain
  //   set simulation domain params from prism params

  //NP modified C.K. can be wedge as well
  bool isBox = (strcmp(domain->regions[iregion]->style,"prism") != 0) &&
               (strcmp(domain->regions[iregion]->style,"wedge") != 0);

  if (isBox) { //NP modified C.K.
    domain->triclinic = 0;
    domain->boxlo[0] = domain->regions[iregion]->extent_xlo;
    domain->boxhi[0] = domain->regions[iregion]->extent_xhi;
    domain->boxlo[1] = domain->regions[iregion]->extent_ylo;
    domain->boxhi[1] = domain->regions[iregion]->extent_yhi;
    domain->boxlo[2] = domain->regions[iregion]->extent_zlo;
    domain->boxhi[2] = domain->regions[iregion]->extent_zhi;
  } else if (strcmp(domain->regions[iregion]->style,"wedge") == 0) {
    RegWedge *region = static_cast<RegWedge*>(domain->regions[iregion]);
    if(!dynamic_cast<DomainWedge*>(domain))
        error->all(FLERR,"Create_box with wedge region requires you to start "
                         "with the '-domain wedge' command line option");
    else
        dynamic_cast<DomainWedge*>(domain)->set_domain(region);
  } else {
    domain->triclinic = 1;
    RegPrism *region = (RegPrism *) domain->regions[iregion];
    domain->boxlo[0] = region->xlo;
    domain->boxhi[0] = region->xhi;
    domain->boxlo[1] = region->ylo;
    domain->boxhi[1] = region->yhi;
    domain->boxlo[2] = region->zlo;
    domain->boxhi[2] = region->zhi;
    domain->xy = region->xy;
    domain->xz = region->xz;
    domain->yz = region->yz;
  }

  // if molecular, zero out topology info

  if (atom->molecular) {
    //NP need to uncomment this since AtomVecBondGran sets this before
    //NP since needed for mem allocation
    //NP atom->bond_per_atom = 0;
    atom->angle_per_atom = 0;
    atom->dihedral_per_atom = 0;
    atom->improper_per_atom = 0;
    atom->nbonds = 0;
    atom->nangles = 0;
    atom->ndihedrals = 0;
    atom->nimpropers = 0;
  }

  // set atom and topology type quantities

  atom->ntypes = force->inumeric(FLERR,arg[0]);
  //NP need to uncomment this since AtomVecBondGran sets this before
  //NP since needed for mem allocation
  //NP atom->nbondtypes = 0;
  atom->nangletypes = 0;
  atom->ndihedraltypes = 0;
  atom->nimpropertypes = 0;

  // problem setup using info from header
  // no call to atom->grow since create_atoms or fixes will do it

  update->ntimestep = 0;

  atom->allocate_type_arrays();

  domain->print_box("Created ");
  domain->set_initial_box();
  domain->set_global_box();
  comm->set_proc_grid();
  domain->set_local_box();

  if(narg == 2) return;

  //NP modified C.K. bonds
  if(strcmp(arg[2],"bonds") == 0)
    error->all(FLERR,"Illegal create_box command, 'bonds' keyword moved to atom_style bond/gran command");
}
