****************************************************************************************************
*                                                                                                  *
*                        aflow - STEFANO CURTAROLO Duke University 2003-2021                       *
*                            High-Throughput ab-initio Computing Project                           *
*                                                                                                  *
****************************************************************************************************
One example of the high-throughput first-principle calculations for partial occupied
structure (mostly written by Kesong Yang).

1) Prepare your aflow.in input files:

[AFLOW] ************************************************************************************************************************** 
[AFLOW]SYSTEM=Ga0.25In0.75P
[AFLOW] ************************************************************************************************************************** 
[AFLOW] input file for aflow 
[AFLOW_MODE=VASP] 
[AFLOW] ************************************************************************************************************************** 
[AFLOW_MODE_ZIP=xz] 
[AFLOW_MODE_BINARY=vasp46s] 
[AFLOW] ************************************************************************************************************************** 
[AFLOW] ************************************************************************************************************************** 
#[AFLOW_MODE_MPI]
[AFLOW_MODE_MPI_MODE]NCPUS=MAX 
[AFLOW_MODE_MPI_MODE]COMMAND ="mpirun -np" 
[AFLOW_MODE_MPI_MODE]AUTOTUNE 
[AFLOW_MODE_MPI_MODE]BINARY="mpivasp46s" 
[AFLOW] ************************************************************************************************************************** 
[AFLOW_SYMMETRY]CALC 
#[AFLOW_SYMMETRY]SGROUP_WRITE 
#[AFLOW_SYMMETRY]SGROUP_RADIUS=7.77 
[AFLOW] ************************************************************************************************************************** 
#[AFLOW_NEIGHBOURS]CALC 
[AFLOW_NEIGHBOURS]RADIUS=7.7 
[AFLOW_NEIGHBOURS]DRADIUS=0.1 
[AFLOW] ************************************************************************************************************************** 
#[AFLOW_APL]CALC                                      // README_AFLOW_APL.TXT
[AFLOW_APL]ENGINE=DM                                  // README_AFLOW_APL.TXT
[AFLOW_APL]DMAG=0.015                                 // README_AFLOW_APL.TXT
[AFLOW_APL]MINATOMS=100                               // README_AFLOW_APL.TXT
#[AFLOW_APL]SUPERCELL=3x3x3                           // README_AFLOW_APL.TXT
[AFLOW_APL]DC=y                                       // README_AFLOW_APL.TXT
[AFLOW_APL]DPM=y                                      // README_AFLOW_APL.TXT
[AFLOW_APL]ZEROSTATE=y                                // README_AFLOW_APL.TXT
[AFLOW_APL]DOS=y                                      // README_AFLOW_APL.TXT
[AFLOW_APL]TP=y                                       // README_AFLOW_APL.TXT
[AFLOW_APL]TPT=0:2000:10                              // README_AFLOW_APL.TXT
[AFLOW] ************************************************************************************************************************** 
#[AFLOW_QHA]CALC                                      // README_AFLOW_QHA_SCQHA_QHA3P.TXT
[AFLOW_QHA]MODE=QHA3P                                 // README_AFLOW_QHA_SCQHA_QHA3P.TXT
[AFLOW_QHA]EOS=y                                      // README_AFLOW_QHA_SCQHA_QHA3P.TXT
[AFLOW] ************************************************************************************************************************** 
#[AFLOW_AAPL]CALC                                     // README_AFLOW_APL.TXT
[AFLOW_AAPL]TDMAG=0.015                               // README_AFLOW_APL.TXT
[AFLOW_AAPL]CUT_SHELL=4                               // README_AFLOW_APL.TXT
[AFLOW_AAPL]CUT_RAD=4.5                               // README_AFLOW_APL.TXT
[AFLOW_AAPL]SUMRULE=1E-5                              // README_AFLOW_APL.TXT
[AFLOW_AAPL]BTE=FULL                                  // README_AFLOW_APL.TXT
[AFLOW_AAPL]THERMALGRID=21x21x21                      // README_AFLOW_APL.TXT
[AFLOW_AAPL]ISOTOPE=y                                 // README_AFLOW_APL.TXT
[AFLOW_AAPL]CUMULATIVEK=y                             // README_AFLOW_APL.TXT
[AFLOW_AAPL]BOUNDARY=n                                // README_AFLOW_APL.TXT
[AFLOW_AAPL]NANO_SIZE=100                             // README_AFLOW_APL.TXT
[AFLOW_AAPL]TCT=200:700:20                            // README_AFLOW_APL.TXT
[AFLOW] ************************************************************************************************************************** 
[VASP_RUN]RELAX_STATIC_BANDS=2                        // GENERATE STATIC RELAX=N RELAX_STATIC=N STATIC_BANDS RELAX_STATIC_BANDS=N REPEAT_BANDS 
#[VASP_FORCE_OPTION]NEGLECT_NOMIX 
[VASP_FORCE_OPTION]CHGCAR=ON                         // ON | OFF (default ON)
[VASP_FORCE_OPTION]KEEPK 
#[VASP_FORCE_OPTION]KPOINTS=EVEN                      // EVEN | ODD (default none)
#[VASP_FORCE_OPTION]KPOINTS_KSHIFT_GAMMA=EVEN         // EVEN | ODD (default none)
#[VASP_FORCE_OPTION]KPOINTS_KSCHEME=MONKHORST_PACK    // MONKHORST_PACK | GAMMA (manual)
#[VASP_FORCE_OPTION]KPOINTS_GAMMA 
#[VASP_FORCE_OPTION]KPOINTS_IBZKPT 
[VASP_FORCE_OPTION]SYM=ON                             // ON | OFF  (default ON)
[VASP_FORCE_OPTION]AUTO_PSEUDOPOTENTIALS=potpaw_PBE   // pot_LDA | pot_GGA | potpaw_LDA | potpaw_GGA | potpaw_PBE | potpaw_LDA_KIN | potpaw_PBE_KIN  
[VASP_FORCE_OPTION]NBANDS                             // Estimate Bands (better than VASP)
[VASP_FORCE_OPTION]SPIN=OFF   // (ON | OFF  (default ON)), REMOVE_RELAX_1 | _2
[VASP_FORCE_OPTION]AUTO_MAGMOM=ON                    // ON | OFF (default OFF)
[VASP_FORCE_OPTION]RELAX_MODE=FORCES                  // ENERGY | FORCES | ENERGY_FORCES | FORCES_ENERGY (default ENERGY) 
[VASP_FORCE_OPTION]PREC=HIGH                          // (LOW | MEDIUM | NORMAL | HIGH | ACCURATE), PRESERVED (default=MEDIUM)
[VASP_FORCE_OPTION]ALGO=FAST                          // (NORMAL | VERYFAST | FAST | ALL | DAMPED), PRESERVED (default=NORMAL)
[VASP_FORCE_OPTION]RELAX 
#[VASP_FORCE_OPTION]NOTUNE 
[VASP_FORCE_OPTION]TYPE=INSULATOR                     // METAL | INSULATOR | SEMICONDUCTOR | DEFAULT (default DEFAULT) 
[VASP_FORCE_OPTION]CONVERT_UNIT_CELL=SPRIM       // SPRIM, SCONV, NIGGLI, MINK, INCELL, INCOMPACT, WS, CART, FRAC
#[VASP_FORCE_OPTION]VOLUME+=10.0 
#[VASP_FORCE_OPTION]VOLUME*=1.05 
[AFLOW] ************************************************************************************************************************** 
[AFLOW] ************************************************************************************************************************** 
[VASP_INCAR_MODE_EXPLICIT]START 
SYSTEM=Ga0.25In0.75P
#NELM = 120
NELMIN=2
LPLANE=.TRUE.
LREAL=.FALSE.
LSCALU=.FALSE.
PSTRESS=000                                           # for hand modification
#NBANDS=XX                                            # for hand modification
#IALGO=48                                             # for hand modification
[VASP_INCAR_MODE_EXPLICIT]STOP 
[AFLOW] ************************************************************************************************************************** 
[VASP_KPOINTS_MODE_IMPLICIT] 
[VASP_KPOINTS_FILE]KSCHEME=G 
[VASP_KPOINTS_FILE]KPPRA=1000
[VASP_KPOINTS_FILE]STATIC_KSCHEME=G
[VASP_KPOINTS_FILE]STATIC_KPPRA=3000
[VASP_KPOINTS_FILE]BANDS_LATTICE=AUTO
[VASP_KPOINTS_FILE]BANDS_GRID=20
[AFLOW] ************************************************************************************************************************** 
[AFLOW] ************************************************************************************************************************** 
[VASP_POTCAR_MODE_IMPLICIT] 
[VASP_POTCAR_FILE]Ga
[VASP_POTCAR_FILE]In
[VASP_POTCAR_FILE]P
[AFLOW] ************************************************************************************************************************** 
[AFLOW] ************************************************************************************************************************** 
[POCC_MODE_EXPLICIT]START.POCC_STRUCTURE
Ga0.25In0.75P 
1.224745  0.001
   0.00000000000000   2.24076438492221   2.24076438492221
   2.24076438492221   0.00000000000000   2.24076438492221
   2.24076438492221   2.24076438492221   0.00000000000000
1*0.25 1*0.75 1
Direct(2) P
   0.00000000000000   0.00000000000000   0.00000000000000  Ga    
   0.00000000000000   0.00000000000000   0.00000000000000  In    
   0.75000000000000   0.75000000000000   0.75000000000000  P     
[POCC_MODE_EXPLICIT]STOP.POCC_STRUCTURE
[AFLOW] ************************************************************************************************************************** 
[AFLOW_POCC]CALC



2) Once the appropriate aflow.in file is finished, you have to prepare the input with
aflow --pocc_input
This command will analyze your partially occupied structure from aflow.in file, which is between the lines:
[POCC_MODE_EXPLICIT]START.POCC_STRUCTURE
...
[POCC_MODE_EXPLICIT]STOP.POCC_STRUCTURE
In addition, the command will also generate a series of nonequivalent structures according to your partially occupied structure.
The log will be save in "LOG.POCC" file.

3) let your aflow daemon run your new aflow.in. The code will do the calculations for all the structures.
Once in a while you should check if there is an OUTCAR that has not been touched for a while. This indicates incomplete termination of VASP.
If the process gets stuck somewhere, delete the directory of the OUTCAR which is not been finished,
then delete the LOCK file of aflow, and restart aflow. The completed directories will not be modified.
The running step does not require an internet connection, so it is safe to run the displacements in a supercomputer with many nodes.

A simple trick. If you want to speed up the calculation, after aflow starts and creates its LOCK, you can rename the LOCK as LOCK1.
This  will not interfere with the running process since the code keeps a pointer to the physical file.

4) when all the directories have finished, run
4.1) aflow --pocc_dos=DIRECTORY,TEMPERATURE
Firstly, this command will analyze your aflow.in, scan through all the subdirectories, warn you if there are missing calculations.
If this is a complete calculation, then this command will generate the density of states (DOS) plot for the partially occupied structure at 
a specified temperature.

4.2) aflow --pocc_bandgap=DIRECTORY,TEMPERATURE
This command will produce the band gap of the partially occupied structure at the specified temperature. The band gap estimation comes from the 
density of states (DOS).

4.3) aflow --pocc_mag=DIRECTORY,TEMPERATURE
This command will produce the magnetic moment of the partially occupied structure at the specified temperature.

Good luck.
Kesong
July. 18th, 2013

5) Running APL with POCC

Using the APL module with POCC requires a completed POCC run.

5.1) Copy the aflow.in file for POCC into a new aflow.in file, e.g. aflow_apl.in. Inside this file,
uncomment the line "[AFLOW_APL]CALC"

5.2) Run aflow with the new aflow.in file. All subdirectories should now each contain an aflow_apl.in file.

5.3) Run APL inside the POCC subdirectories. For more information on APL, see the APL README. Running
the APL postprocessing step is not necessary.

5.4) Run aflow in the POCC directory with the aflow_apl.in file.

6) Sampling the supercells (CO20261006)

A parent with too many decorations to compute them all can be sampled. The keys go below [AFLOW_POCC]CALC; with
--aflow_proto, the options in brackets write them.

[AFLOW_POCC]SAMPLE_NUMBER=N (--pocc_sample_number=N)
The first round draws 100*N decorations at random from every site configuration of every superlattice, or takes all of
them when there are fewer. The seed is DEFAULT_POCC_SAMPLE_SEED in .aflow.rc, so a run gives the same draw every time.
The enumerator (section 7) groups the drawn decorations into unique supercells, each with the number of draws in it
as degeneracy.
The second round keeps N of the unique supercells at random, or all of them when there are no more than N.

[AFLOW_POCC]SAMPLE_HASH=HnCm (--pocc_sample_hash=HnCm)
Computes the one decoration that ARUN.POCC_*_HnCm names. With SAMPLE_NUMBER it joins the first round, and the second
round is skipped.

[AFLOW_POCC]SAMPLE_RATE=r1:r2 (--pocc_sample_rate=r1:r2)
The first round takes the fraction r1 of the site configurations, the same ones in every superlattice, from a full
enumeration; the second keeps the fraction r2 of the unique supercells. SAMPLE_NUMBER overrides it.

[AFLOW_POCC]SKIP_SITE_CONFIGURATIONS (--pocc_skip_site_configurations)
Does not write aflow.pocc.site_configurations.out, the list of all site configurations. Runs with SAMPLE_NUMBER or
SAMPLE_HASH never write it.

7) Enumerators

POCC lists every decoration of every superlattice of index n and groups the equivalent ones.
The group sizes are the degeneracies of the unique supercells.
Two enumerators do the grouping.

UFF: decorations of the same superlattice with equal UFF energies (within DEFAULT_UFF_ENERGY_TOLERANCE)
form one group. Two different supercells with the same UFF energy end up in one group.

ENUM (default): each group is the set of decorations that are the same structure, from the orbits of the permutation group
of the supercell (Phys. Rev. B 77, 224115 (2008) and Phys. Rev. B 80,
014120 (2009)).
The Smith normal form of the HNF matrix maps the sites onto Z_s1 x Z_s2 x Z_s3, so the translations act by addition.
The factor group operations of the parent that keep the superlattice permute the sites as well.
The parent symmetry comes from the PARTCAR with each site colored by the species counts it takes in the supercells
(sites with occupations 0.5/0.5 and 0.4/0.6 both give 2:2 at n=4 and are not told apart), keeping only the
operations that map the structure configurations onto each other.
Superlattices are reduced by the point group of those operations.
The list starts with the superlattices POCC reduces by the lattice point group, so their H indices stay the same,
and continues with any superlattice that only the smaller point group separates (for example, the two chiral
sqrt(5) x sqrt(5) cells of a parent with point group 4/m on a tetragonal lattice).
A superperiodic decoration (one that repeats with a smaller cell than its supercell) also appears under operations
that change the superlattice, on the same HNF matrix and on others; those decorations join its group, so a group can
hold decorations of several HNF matrices (the representative is the smallest H, then C, and the degeneracies still
add up to the decorations). The published algorithm drops superperiodic decorations; POCC keeps them.
Sampling (section 6) gives ENUM the decorations of the first round of UFF, and ENUM groups those that are the same
structure, so the two enumerators differ only in the grouping.
The degeneracy of a sampled group is the number of drawn decorations in it, as with UFF, so the weights remain a
sample of the ensemble: a large group is drawn more often already and is not weighted by its size a second time.
The second round and POCC_SAMPLE_HASH work as with UFF.
--pocc_count_unique counts the groups without UFF energies (--pocc_count_unique_fast applies to UFF only).

COMPARE: runs UFF and ENUM, lists the UFF groups that hold more than one supercell, the supercells UFF splits on
one HNF matrix, and the superperiodic supercells found on several HNF matrices (UFF groups each HNF matrix
separately), and keeps the ENUM result. It needs every decoration, so it refuses sampling.

Select the enumerator with
  aflow.in:      [AFLOW_POCC]ENUMERATOR=ENUM
  command line:  aflow --pocc_count_unique --pocc_enumerator=compare < PARTCAR
  .aflow.rc:     DEFAULT_POCC_ENUMERATOR="ENUM"
The command line takes precedence over aflow.in, and aflow.in over .aflow.rc.
DEFAULT_POCC_ENUMERATOR replaces DEFAULT_POCC_STRUCTURE_GENERATION_ALGO, which is no longer read: every .aflow.rc
written since 2020 sets that key to UFF, and AFLOW now ignores it, so existing installations get ENUM without
editing their .aflow.rc. To keep UFF as the default, set DEFAULT_POCC_ENUMERATOR="UFF".

A new POCC aflow.in records the enumerator it is written with (--pocc_enumerator= with --aflow_proto, else the
.aflow.rc default) as [AFLOW_POCC]ENUMERATOR=, so the parent keeps its supercell list if the default changes.
An aflow.in without the key was written while UFF made every supercell list, and runs with UFF (CO20261009).
The two lists name their supercells alike (ARUN.POCC_<i>_H<h>C<c>) but number and group them differently, so a
parent should not change enumerator after its supercells are written.

8) Correlation functions (CO20261006)

Post-processing (aflow --lib2raw included) writes the cluster correlation functions of the supercells to
aflow.pocc.out, for either enumerator.
The clusters are the pairs of partially occupied sites out to DEFAULT_POCC_CORRELATION_PAIR_SHELLS distinct distances
(6) and the triplets whose edges all lie within DEFAULT_POCC_CORRELATION_TRIPLET_SHELLS distances (2), grouped by the
symmetry of the parent.
A site with M species carries the point functions of CALPHAD 33, 266 (2009): gamma_0=1, and
gamma_j(s)=-cos(2 pi ceil(j/2) s/M) for odd j, -sin(2 pi floor(j/2) s/M) for even j, with s=0..M-1 in the order of
the species (vacancy first, then the PARTCAR order).
The correlation p1(1,1) is the average of gamma_1 gamma_1 over every cluster p1 in the supercell; for a binary it is
the average product of the spins (-1 for the first species, +1 for the second).
Each supercell is located on the parent lattice through its HNF matrix and decoration (read from its POSCAR title),
so no distance tolerance enters.

Temperature-independent lines:
  enumerator=                                    UFF or ENUM
  correlation_site_<i>=                          species of site i in point-function order, with concentrations
  correlation_cluster_<name>=                    pair or triplet, its sites, distance shell, size and multiplicity
  correlation_functions=                         names of the functions, in the order of the values below
  correlation_random=                            the random alloy at those concentrations
  warren_cowley_pairs=                           names of the Warren-Cowley parameters, in the order of their values
  correlation_supercell_<NN>=                    each supercell
  correlation_deviation_supercell_<NN>=          mean absolute difference from the random alloy
  warren_cowley_supercell_<NN>=                  each supercell
  correlation_infinite_temperature=              degeneracy-weighted average
  correlation_deviation_infinite_temperature=
  warren_cowley_infinite_temperature=
At each temperature:
  correlation_ensemble=                          probability-weighted average
  correlation_deviation_ensemble=
  warren_cowley_ensemble=

Warren-Cowley parameters: alpha_st=1-P(s,t)/(c_s c_t) for each pair cluster, where P(s,t) is the probability of species s
on its first point and t on its second, and c_s, c_t are the marginals of P (the composition of the supercell or of the
ensemble). alpha is 0 in the random alloy, negative for pairs found more often than at random. When both points lie on one
site, or on sites the symmetry of the parent exchanges, P is symmetrized and each unordered pair is listed once
(p1(Mg-Ni)); otherwise s is on the first site of the cluster (as listed in correlation_cluster_<name>) and t on the
second, and every ordered pair is listed. A species missing from a supercell gives nan.

The infinite-temperature ensemble differs from the random alloy in a finite supercell: fixed counts make two sites
of a cell anticorrelated (-1/(n-1) at equal concentrations), and a neighbor that is a periodic image of the site
itself contributes +1. For rocksalt Mg0.5Ni0.5O at n=8, the fourth shell gives +0.23 instead of 0.

****************************************************************************************************
*                                                                                                  *
*                        aflow - STEFANO CURTAROLO Duke University 2003-2021                       *
*                            High-Throughput ab-initio Computing Project                           *
*                                                                                                  *
****************************************************************************************************
