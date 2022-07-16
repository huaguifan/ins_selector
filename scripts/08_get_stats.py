# ==========================
# 用于获得一组日志文件的统计数据
# ==========================

# 使用示例：python ./scripts/08_get_stats.py -t cauctions -d test_200_1000 -e 0627_scip3_afsb_oracle_12_single -k 20

from errno import errorcode
import os
import argparse
import datetime

from functools import reduce
from utils import is_file_over
from get_scip_node import Node
from subprocess import check_output
from get_scip_node import InstanceFile
from get_branch import get_branching

def get_column(matrix, i):
    return [row[i] for row in matrix]

def gmean(row):
    """
    获得向量的几何平均值
    :param row: 向量
    """
    tmp_list = [max(1, x) for x in row]
    res = reduce(lambda x,y:x*y, tmp_list) ** (1.0/len(tmp_list))
    return res

def write_to_file(filebase, ins_stat, result_dir):
    """
    将ins_stat写入result_dir中的stat
    :param filebase：待写入数据所属instance
    :param ins_stat：待写入数据
    :param result_dir：输出文件的路径
    """
    # 加入表头，覆盖原文件
    output_stat = os.path.join(result_dir, "stats")
    headers = [
        "problem",      # 实例名字
        "nnodes",       # 求解总节点数
        "time",         # 求解总时间
        "DB",           # 求解DB
        "PB",           
        "opt",          # 该实例精确解值
        "ogap",         
        "igap",
        "bpb-T",        # 找到最优可行解时间
        "pre-T",        # 预求解时间
        "brch",         # 所有变量分支的总时间
        "nvars"         # 总的分支变量个数
    ]
    
    if filebase == "":
        fout = open(output_stat, 'w')
        str_to_write = "%-10s" % headers[0]
        for i in range(1, len(headers)):
            str_to_write += "%7s" % headers[i]
        fout.write("%s\n" % str_to_write)
    else:
        fout = open(output_stat, 'a')
        str_to_write = "%-10s %6d %6.0f" % (filebase, ins_stat[0], ins_stat[1])
        
        for i in range(2, len(ins_stat)):
            try:
                str_to_write += " %6.2f" % ins_stat[i]
            except:
                print("write_to_file wrong")
                input()
        fout.write("%s\n" % (str_to_write))
    fout.close()
    
def get_log_stat(log_path, sol_path):
    """
    从一个instance的log和sol中提取该instance的节点数，求解时间等信息
    :param log_path
    :param sol_path
    """
    if is_file_over(log_path) == False:
        # print("ins not over")
        return [0]*10
    
    igap_line = check_output("grep \"^Gap\" %s | tail -n 1" % log_path, shell=True, encoding="utf-8")
    try:
        # igap = float(igap_line.strip('\n').split()[2])
        igap = float(float(igap_line.strip('\n').split()[2]))
    except:
        print("no igap")
        print(igap_line)
        assert igap_line.strip('\n').split()[2] == "infinite"

    time_line = check_output("grep \"Solving Time\" %s | tail -n 1" % log_path, shell=True, encoding="utf-8")
    nnodes_line = check_output("grep \"Solving Nodes\" %s | tail -n 1" % log_path, shell=True, encoding="utf-8")
    dualbound_line = check_output("grep \"Dual Bound\" %s | tail -n 1" % log_path, shell=True, encoding="utf-8")
    primalbound_line = check_output("grep \"Primal Bound\" %s | tail -n 1" % log_path, shell=True, encoding="utf-8")
    
    time = float(time_line.strip('\n').split()[4])
    nnodes = int(nnodes_line.strip('\n').split()[3])
    db = float(dualbound_line.strip('\n').split()[3])
    pb = float(primalbound_line.strip('\n').split()[3])

    try:
        # opt_line = check_output("grep \"objective value\" %s" % sol_path, shell=True, encoding="utf-8")
        opt_line = check_output("grep \"objective value\" %s | head -n 2 | tail -n 1" % log_path, shell=True, encoding="utf-8")
        opt = float(opt_line.strip('\n').split()[2])
        ogap = abs(opt-pb)
    except:
        opt = -1.0
        ogap = -1.0
    
    ins_file_obj = InstanceFile(log_path)
    bPB_time = ins_file_obj.bPB_time
    presolving_time = ins_file_obj.presolving_time

    branching_time, nvars = get_branching(log_path)
    
    # try:
    #     error_line = check_output("grep \" comp error rate\" %s" %log_path, shell=True, encoding="utf-8")
    #     wrong = int(error_line.strip('\n').split()[4].split('/')[0])
    #     total = int(error_line.strip('\n').split()[4].split('/')[1])
    #     return nnodes, time, db, pb, opt, ogap, igap, bPB_time, presolving_time, wrong, total
    # except:
    #     return nnodes, time, db, pb, opt, ogap, igap, bPB_time, presolving_time
    return nnodes, time, db, pb, opt, ogap, igap, bPB_time, presolving_time, branching_time, nvars
    
def get_dir_stats(data_path, solution_dir, result_dir, first_k=20):
    filelist = sorted(os.listdir(data_path), key=lambda x:int(x.split('.')[0].split('_')[1]))
    ins_stat_list = []
    write_to_file("", [], result_dir)
    for i, ins_file in enumerate(filelist):
        insnum = int(ins_file.split('.')[0].split('_')[1])
        if i >= first_k:
            continue
        base = ins_file.split('.')[0]
        l_path = os.path.join(result_dir, f'{base}.log')
        s_path = os.path.join(solution_dir, f'{base}.sol')
        i_stat = get_log_stat(l_path, s_path)
        # write to file
        if True:
            write_to_file(base, i_stat, result_dir)
            ins_stat_list.append(i_stat)
            print(i, ins_file, i_stat)

    avg_stat = []
    try:
        for j in range(len(ins_stat_list[0])):
            c_list = get_column(ins_stat_list, j)
            avg_stat.append(gmean(c_list))
        write_to_file("average", avg_stat, result_dir)
    except:
        print("continue")

if __name__ == "__main__":

    now_time = datetime.datetime.now().strftime('%Y-%m-%d-%H-%M')

    parser = argparse.ArgumentParser()
    parser.add_argument(
       '-t', '--dat_type',
       help='data type',
       type=str,
       default='setcover',
    )
    parser.add_argument(
        '-d', '--data',
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
       '-k', '--first_k',
       help='experiment name',
       type=int,
       default=20,
    )
    
    args = parser.parse_args()

    fk = args.first_k
    d_name = args.data
    dat_type = args.dat_type
    experiment = args.experiment
    
    training_files_base="/home/xuliming/daggerSpace/training_files/scip-dagger"
    data_dir=os.path.join("/home/xuliming/daggerSpace/training_files/scip-dagger/dat", dat_type)

    d_path = os.path.join(data_dir, d_name)
    s_dir = os.path.join(training_files_base, "bfs_solution", dat_type, d_name)
    r_dir=os.path.join(training_files_base, "clip-scratch", "result", dat_type, d_name, experiment)
    
    get_dir_stats(d_path, s_dir, r_dir, fk)
