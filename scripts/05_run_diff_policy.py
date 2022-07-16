# =============================================
# 本脚本适用于单线程求解实验 (Policy, SCIP, Oracle)
# =============================================

# 使用示例：python ./scripts/05_run_diff_policy.py -a scip -t cauctions -d test_100_620 -e 0629_scip3_afsb_bfs_12_single -s ./sets/allfullstrong_bfs.set -k 500

from ast import arg
import os
import argparse
import datetime
import signal
import sys
import time

def signal_handler(sig, frame):
    print('Pressed Ctrl-C!')
    sys.exit()

def run_diff_policy(dat_dir, sol_dir, policy_dir, result_dir, set_path, exe="bin/scipdagger-0622", timelimit=3600, first_k=20):
    signal.signal(signal.SIGINT, signal_handler)
    
    policy_list = sorted(os.listdir(policy_dir), key=lambda x:int(x.split('.')[1]))             # policy_list为所有的策略名
    file_list = sorted(os.listdir(dat_dir), key=lambda x:int(x.split('.')[0].split('_')[1]))    # 所有原始问题文件名

    print(policy_dir)
    print(sol_dir)
    print(result_dir)

    for i, policy in enumerate(policy_list):
        numPolicy = int(policy.split('.')[1])

        # 指定策略序号
        # if numPolicy != 3:
        #     continue
        print(policy, numPolicy)
        
        for j, ins_file in enumerate(file_list):
            # 超出指定的K值，循环停止
            if j >= first_k: 
                break
            
            base = ins_file.split('.')[0]
            print("numpolicy %d %d %s" % (numPolicy, j, ins_file))  

            f_path = os.path.join(dat_dir, ins_file)                        # 原始问题LP文件位置
            p_path = os.path.join(policy_dir, policy)                       # 策略文件位置
            s_path = os.path.join(sol_dir, f'{base}.sol')                   # SOL文件位置
            r_policy_dir = os.path.join(result_dir, f'policy.{numPolicy}')  # LOG日志文件夹
            r_path = os.path.join(r_policy_dir, f'{base}.log')              # 当前实例的日志文件位置
            
            # 判断日志路径是否存在，若不存在创建新的文件夹
            if os.path.isdir(r_policy_dir) == False:
                os.makedirs(r_policy_dir, exist_ok=False)
            
            # run scipdagger
            try:
                os.system("bin/scipdagger-0622 -t %d -f %s -s %s -o %s --nodesel dagger %s > %s" % (timelimit, f_path, set_path, s_path, p_path, r_path))
            except KeyboardInterrupt:
                print("numpolicy %d %d %s stopped" % (numPolicy, j, ins_file))
                continue

def run_scip(dat_dir, sol_dir, policy_dir, result_dir, set_path, exe="collect_multi_trjs_new", timelimit=3600, first_k=20, dagger="scip"):
    signal.signal(signal.SIGINT, signal_handler)

    file_list = sorted(os.listdir(dat_dir), key=lambda x:int(x.split('.')[0].split('_')[1]))    
    print(policy_dir)
    print(sol_dir)
    print(result_dir)

    # 判断日志路径是否存在
    if os.path.isdir(result_dir) == False:
        os.makedirs(result_dir, exist_ok=False)
    for j, ins_file in enumerate(file_list):
        try:
            base = ins_file.split('.')[0]
            print("%d %s" % (j, base))

            if j >= first_k:
                continue
            f_path = os.path.join(dat_dir, ins_file)
            s_path = os.path.join(sol_dir, f'{base}.sol')
            r_path = os.path.join(result_dir, f'{base}.log')

            if dagger == "scip":
                os.system("bin/scipdagger-0622 -t %d -f %s -s %s > %s" % (timelimit, f_path, set_path, r_path))
            # 策略Oracle
            else:
                os.system("bin/scipdagger-0622 -t %d -f %s -s %s -o %s --nodesel oracle > %s" % (timelimit, f_path, set_path, s_path, r_path))              
        except KeyboardInterrupt:
            sys.exit()

if __name__ == "__main__":

    now_time = datetime.datetime.now().strftime('%Y-%m-%d-%H-%M')

    parser = argparse.ArgumentParser()
    parser.add_argument(
       '-a', '--dagger',
       help='dagger',
       choices=['policy', 'scip', 'oracle'],
    )
    parser.add_argument(
       '-t', '--dat_type',
       help='data type',
       type=str,
       default='setcover',
    )
    parser.add_argument(
        '-d', '--dat_name',
        help='input data dir',
        type=str,
        default="",
    )
    parser.add_argument(
       '-e', '--experiment',
       help='experiment name',
       type=str,
       default=f'tmp_{now_time}',
    )
    parser.add_argument(
       '-s', '--set',
       help='set name',
       type=str,
       default=f'./sets/allfullstrong_bfs.set',
    )
    parser.add_argument(
       '-k', '--first_k',
       help='experiment name',
       type=int,
       default=20,
    )
    parser.add_argument(
       '-p', '--p_dir',
       help='policy dir',
       type=str,
       default='',
    )
    
    args = parser.parse_args()
    dat_type = "setcover"

    fk = args.first_k               # 运行数据集中前K个例子
    p_dir = args.p_dir              # 如果使用INS策略，则需传入策略所在文件位置
    set_name = args.set             # 使用的set文件
    dat_type = args.dat_type        # 数据集类型 (setcover, cauctions)
    dat_name = args.dat_name        # 数据集名称 (train, test)
    experiment = args.experiment    # 日志存储位置（当前实验中使用SCIP求解实例生成的日志）

    training_files_base="/home/xuliming/daggerSpace/training_files/scip-dagger" # 所有的文件都将存储于training_files_base文件中

    d_path = os.path.join(training_files_base,"dat",dat_type, dat_name)
    s_dir = os.path.join(training_files_base, "bPB_solution", dat_type, dat_name)
    r_dir = os.path.join(training_files_base, "clip-scratch", "result", dat_type, dat_name, experiment)
    
    if args.dagger == 'policy':
        run_diff_policy(dat_dir=d_path, sol_dir=s_dir, policy_dir=p_dir, result_dir=r_dir, set_path=set_name, first_k=fk)
    else:
        run_scip(dat_dir=d_path, sol_dir=s_dir, policy_dir=p_dir, result_dir=r_dir, set_path=set_name, first_k=fk, dagger=args.dagger)