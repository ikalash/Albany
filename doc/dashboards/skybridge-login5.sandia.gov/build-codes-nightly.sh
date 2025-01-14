#!/bin/bash
## Do not put any commands or blank lines before the #SBATCH lines
export TEST_DIR=`pwd`
rm -rf *log 
rm -rf Albany
rm -rf Trilinos
git clone git@github.com:trilinos/Trilinos.git >& trilinos-clone.log
cd Trilinos
git checkout develop
cd ..
git clone git@github.com:SNLComputation/Albany.git >& albany-clone.log 
module use --append $TEST_DIR/Albany/doc/LandIce/modulefiles
module load serial-intel-release
#module load cde/dev/compiler/gcc/7.2.0
#module swap cmake/3.12.2 cde/dev/cmake/3.19.2
rm -rf trilinos-build-serial-intel-release
rm -rf trilinos-install-serial-intel-release
rm -rf albany-build-serial-intel-release
rm -rf slurm*
./clean-update-config-build-dash.sh trilinos 8
mv trilinos-serial-intel-release.log trilinos-serial-intel-release-build.log 
./test-dash.sh trilinos
./clean-update-config-build-dash.sh albany 8
mv albany-serial-intel-release.log albany-serial-intel-release-build.log 
sbatch batch.openmpi.bash
