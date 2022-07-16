# =========================
# run scip to get solution 
# =========================

# 使用示例：bash ./scripts/01_muti_run.sh -d cauctions -i test_100_730 -e 0629_scip3_afsb_bfs_multi_12 -x .lp -s ./sets/allfullstrong_bfs.set -t 36000 -u ./bin/scipdagger-0621-2 -n 100

data=setcover
data=${data%/}
suffix=.lp

dat=/home/xuliming/daggerSpace/training_files/scip-dagger/dat
training_files_base="/home/xuliming/daggerSpace/training_files/scip-dagger"

resultDir=$training_files_base/clip-scratch/result
solutionDir=$training_files_base/bfs_solution       # He SOLs
bPBSolutionDir=$training_files_base/bPB_solution    # bPB SOLs

experiment=bfs_scip

usage() {
  echo "Usage: $0 -d <data_path_under_dat> -i <subdir under data> -e <experiment> -x <suffix> -s <set> -t <timelimit> -n <thread_num> -u<executable>"
}

suffix=".lp"
freq=1
dagger=0
thread_num=1
set=./sets/allfullstrong_bfs.set

while getopts ":hd:i:e:x:s:t:n:u:" arg; do
  case $arg in
    h)
      usage
      exit 0
      ;;
    d)
      data=${OPTARG%/}
      echo "test data: $data"
      ;;
    i)
      subdir=${OPTARG%/}
      echo "subdir: $subdir"
      ;;
    e)
      experiment=${OPTARG}
      echo "experiment: $experiment"
      ;;
    x)
      suffix=${OPTARG}
      echo "data suffix: $suffix"
      ;;
    s)
      set=${OPTARG}
      echo "scip set: $set"
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

dir=$dat/$data
for fold in $subdir; do
  if ! [ -d $resultDir/$data/$fold/$experiment ]; 
    then mkdir -p $resultDir/$data/$fold/$experiment
  fi
  if ! [ -d $solutionDir/$data/$fold ]; then 
    mkdir -p $solutionDir/$data/$fold
  fi
  if ! [ -d $bPBSolutionDir/$data/$fold ]; then 
    mkdir -p $bPBSolutionDir/$data/$fold
  fi
done

echo "Writing logs in $resultDir/$data/$fold/$experiment"
echo "Writing scip solutions in $solutionDir/$data"

echo " "
echo "data: $data"
echo "exeu: $executable"
echo " "

# --------------------------------------------------muti start
tmp_fifofile="$$.fifo"
mkfifo $tmp_fifofile
exec 6<>$tmp_fifofile # FD6 to type of FIFO
rm $tmp_fifofile

for ((i=0;i<${thread_num};i++)); do
    echo
done >&6

for fold in $subdir; do
  echo "Solving problems in $dir/$fold ..."
  # for file in `ls $dir/$fold | sort -R`; do
  for file in `ls $dir/$fold`; do
    read -u6
    {
      base=`sed "s/$suffix//g" <<< $file`

      if ! [ -e $solutionDir/$data/$fold/$base.sol ]; then
        echo "     sols" $base
        $executable -t $timelimit -f $dir/$fold/$file --sol $solutionDir/$data/$fold/$base.sol -s $set > $resultDir/$data/$fold/$experiment/$base.log
        echo "    ---->" done $base
      # 注意：这一项基本没用，跑这个脚本之前最好确认一下进的是哪个条件
      else
        echo "     normal" $base
        $executable -t $timelimit -f $dir/$fold/$file -s $set > $resultDir/$data/$fold/$experiment/$base.log
        echo "    ---->" done $base
      fi
      echo >&6
    } &
  done
done

wait
exec 6>&- #shutdown FD6
echo "over"

# --------------------------------------------------end
