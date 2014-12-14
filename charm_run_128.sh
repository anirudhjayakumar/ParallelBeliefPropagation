#!/bin/sh
# embedded options to qsub - start with #PBS
#PBS -q secondary
# -- job name --
#PBS -N superRes_128
# -- email me at the beginning (b) and end (e) of the execution --
#PBS -m be
# -- My email address --
#PBS -M ajayaku2@illinois.edu
# -- estimated wall clock time (execution time): hh:mm:ss --
#PBS -l walltime=04:00:00
# -- parallel environment requests --
#PBS -l nodes=16:ppn=8
# -- specify you want want DIFFERENT NODES --
#PBS -W x=nmatchpolicy=exactnode
# -- stdout and stderr in the same file --
#PBS -j oe

# -- end of PBS options --

# -- change to working directory --
cd $PBS_O_WORKDIR
# -- load compiler (if needed) and the OpenMPI module --
module load gcc/4.7.1 
module load mvapich2/2.0b-gcc-4.7.1
NODE_LIST=nodeList_128
/home/ajayaku2/gtc/src/lbl/scripts/test/gennodelist.sh > $NODE_LIST

./charmrun +p112 ++mpiexec ++remote-shell "mpiexec -ppn 1" ++nodelist $NODE_LIST ./superRes small_input/db small_input/inputblocks.txt small_input/outputblocks.txt ++ppn 7






