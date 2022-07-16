# ==================================
# run scip to get trjs for training
# ==================================

# 使用示例：bash ./scripts/02_muti_run_oracle.sh -d facilities/train_200_100_5 -s ./sets/allfullstrong_bfs.set -x .lp -e 0624_scip3_afsb_oracle_11_multi -t 36000 -n 90 -u ./bin/scipdagger-0622

#!/bin/bash
set -e

data=setcover
data=${data%/}
suffix=.lp

dat=/home/xuliming/daggerSpace/training_files/scip-dagger/dat
training_files_base="/home/xuliming/daggerSpace/training_files/scip-dagger"

resultDir=$training_files_base/clip-scratch/result
solutionDir=$training_files_base/bfs_solution
oracleSolutionDir=$training_files_base/solution_true_oracle
bPBSolutionDir=$training_files_base/bPB_solution

usage() {
  echo "Usage: $0 -d <data_path_under_dat> -s <set> -x <suffix> -e <experiment> -t <timelimit> -n <thread_num> -u <executable>"
}

thread_num=20
suffix=".lp.gz"
set=./sets/allfullstrong_bfs.set
executable=./bin/time_debug_random10_20d

while getopts ":hd:e:x:t:n:u:s:" arg; do
  case $arg in
    h)
      usage
      exit 0
      ;;
    d)
      data=${OPTARG%/}
      echo "training data: dat/$data"
      ;;
    s)
      set=${OPTARG%/}
      echo "set file: $set"
      ;;
    e)
      experiment=${OPTARG}
      echo "experiment name: $experiment"
      ;;
    x)
      suffix=${OPTARG}
      echo "data suffix: $suffix"
      ;;
    t)
      timelimit=${OPTARG}
      echo "timelimit: $timelimit"
      ;;
    n)
      thread_num=${OPTARG}
      echo "thread_num: $thread_num"
      ;;
    u)
      executable=${OPTARG}
      echo "executable: $executable"
      ;;
    :)
      echo "ERROR: -${OPTARG} requires an argument"
      usage
      exit 1
      ;;
    ?)
      echo "ERROR: unknown option -${OPTARG}"
      usage
      exit 1
      ;;
  esac
done

# echo $experiment
# time=$(date "+%Y%m%d%H%M%S")
# experiment=${experiment}_${time}
# exit

datDir="$dat/$data"
solDir="$solutionDir/$data"
scratch="$training_files_base/clip-scratch"
trjDir=$scratch/training/trj/$data/$experiment

if [ -z $experiment ]; then experiment=c${svmc}w${svmw}; fi
if ! [ -d $trjDir ]; then mkdir -p $trjDir; fi

searchTrj=$trjDir/"search.trj"
killTrj=$trjDir/"kill.trj"

# We need to append to these trj
if [ -e $searchTrj ]; then rm $searchTrj; echo "rm $searchTrj"; fi
if [ -e $killTrj ]; then rm $killTrj; echo "rm $killTrj"; fi
if [ -e $searchTrj.weight ]; then rm $searchTrj.weight; fi
if [ -e $killTrj.weight ]; then rm $killTrj.weight; fi

if ! [ -d $resultDir/$data/$experiment ]; 
  then mkdir -p $resultDir/$data/$experiment
fi

policyDir=$training_files_base/policy/$data/$experiment
if ! [ -d $policyDir ]; then mkdir -p $policyDir; fi

echo " "
echo "logs: $resultDir/$data/$experiment"
echo "data: $data"
echo "exeu: $executable"
echo " "

# --------------------------------------------------muti start

numPolicy=1

tmp_fifofile="$$.fifo"
mkfifo $tmp_fifofile
exec 6<>$tmp_fifofile # FD6 to type of FIFO
rm $tmp_fifofile

for ((i=0;i<${thread_num};i++)); do
    echo
done >&6

for prob in `ls $datDir | sort -R`; do
  read -u6
  {
    base=`sed "s/$suffix//g" <<< $prob`
    prob=$datDir/$prob
    sol=$bPBSolutionDir/$data/$base.sol
    searchTrjIter=$trjDir/$base.search.trj.$numPolicy
    
    # SOLs文件的行数，行数小于等于1表示预求解结束找到了最优可行解
    nlines_sol=$(wc -l < $sol)
    
    echo "    " $base "  " $prob
    # 当前例子在预求解找到最优可行解，跳过
    if [ "$nlines_sol" -le "1" ]; then
      echo "    ---->" done $base $nlines_sol "continue"
    # 当前例子预求解未找到最优可行解，用oracle策略收集trjs
    else
      $executable -t $timelimit -s $set -f $prob -o $sol --nodesel oracle --nodeseltrj $searchTrjIter > $resultDir/$data/$experiment/$base.log
      echo "    ---->" done $base
    fi

    echo >&6
  } &
done

wait
exec 6>&- #shutdown FD6
echo "over"

# --------------------------------------------------muti end
