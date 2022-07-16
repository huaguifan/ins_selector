# ============================================
# 用于绘制求解过程中的曲线图，主函数中参数均需自己指定
# ============================================

import os
import argparse
import datetime
import matplotlib
import matplotlib.pyplot as plt
from get_scip_node import InstanceFile

def plt_pdgap(diff_ins_nodelist, time_list, titles, fig_path, ins_file, opt):
    min_x = 1000
    min_y = 1000
    max_x = 0
    max_y = 0

    ins_lb_list = []
    ins_up_list = []
    ins_db_list = []
    ins_tm_list = []

    fig_size = len(time_list)

    matplotlib.rcParams['font.sans-serif'] = ['SimHei']
    matplotlib.rcParams['font.family']='sans-serif'
    matplotlib.rcParams['axes.unicode_minus'] = False
    matplotlib.fontsize='30'
    
    fig, _ = plt.subplots(fig_size, 1, figsize=(10,2.5*fig_size), sharey=True)
    # 自己调整图片中标题的空白
    plt.subplots_adjust(wspace =0, hspace =0)
    
    for i, nodelist in enumerate(diff_ins_nodelist):
        node_time = []
        node_lowerbound = []
        global_upperbound = []
        global_lowerbound = []
        
        # get x, y
        for i, node in enumerate(nodelist):
            # 去掉预处理
            if node.idx == -1 or node.lowerbound < 100 or node.time < 3:
                continue
            node_time.append(node.time)
            node_lowerbound.append(node.lowerbound)
            global_lowerbound.append(node.dualbound)
            global_upperbound.append(node.primalbound)

            min_x = min(node.time, min_x)
            max_x = max(node.time, max_x)
            min_y = min(min_y, node.lowerbound)
            max_y = max(max_y, node.primalbound)

        ins_tm_list.append(node_time)
        ins_lb_list.append(node_lowerbound)
        ins_up_list.append(global_upperbound)
        ins_db_list.append(global_lowerbound)
    
    i = 0
    for lowerbound, upperbound, dualbound, xtime in zip(ins_lb_list, ins_up_list, ins_db_list, ins_tm_list):
        # plot
        plt.subplot(fig_size,1,i+1) #两行一列，第一个图
        plt.xlim(min_x, max_x + 5)
        # plt.xlim(min_x, 1000 + 5)
        plt.ylim(min_y, max_y + 5)
        plt.ylabel("Bounds", fontsize='30')
        
        plt.xticks(fontsize=20)
        plt.yticks(fontsize=14)

        plt.scatter(xtime, lowerbound, s=35, alpha=0.8, c='#C7E0FF')
        plt.scatter(xtime, upperbound, s=35, alpha=0.8, c='#FC7307')
        plt.scatter(xtime, dualbound, s=35, alpha=0.8, c='#2D7CD7')
        
        if i == 0:
            plt.title(ins_file + "\n" +titles[i], fontsize='30')
            plt.legend(("Node Lowerbound", "Global Upperbound", "Global Lowerbound"), loc="lower right", prop={'size':13.5})
        else:
            plt.title(titles[i], fontsize='30')
        i += 1
        
    fig.tight_layout()
    plt.xlabel("Time (seconds)", fontsize='30', loc='right')
    plt.savefig(fig_path, dpi=600, bbox_inches = 'tight')
    # 
    plt.cla()
    plt.close(fig)

def make_pgap(dat_dir, log_dir, figs_name, data_name, exps, titles):
    
    pngs_save_path = f'/home/xuliming/nodeselector/png/{data_name}/{figs_name}'
    print(pngs_save_path)
    os.makedirs(pngs_save_path, exist_ok=True)
    
    dat_list = sorted(os.listdir(dat_dir), key=lambda x:int(x.split('.')[0].split('_')[1]))

    for i, ins_file in enumerate(dat_list):
        insnum = int(ins_file.split('.')[0].split('_')[1])

        print(i, ins_file)
        
        base = ins_file.split('.')[0]
        fig_path = os.path.join(pngs_save_path, f'{base}.png')    
        
        opt = -1.0
        bPB_time_list = []
        diff_ins_nodelist = []
        for exp in exps:
            l_path = os.path.join(log_dir, exp, f'{base}.log')
            obj = InstanceFile(l_path)

            if opt == -1.0:
                opt = obj.best_primalbound
            bPB_time_list.append(obj.bPB_time)
            diff_ins_nodelist.append(obj.nodelist)
        
        plt_pdgap(diff_ins_nodelist, bPB_time_list, titles, fig_path, ins_file, opt)

if __name__ == "__main__":
    
    # 需要手动指定的参数如下 ---------------------------------
    figs_name = "0630"
    d_type = "setcover"
    d_name = "transfer_1000r_1000c_0.05d_easy"
    training_files_base = "/home/xuliming/daggerSpace/training_files/scip-dagger"
    data_dir = "/home/xuliming/daggerSpace/training_files/scip-dagger/dat/setcover"

    exps = [
        # "scip3_afsb_oracle_11",
        "scip3_afsb_bes_13_single",
        "scip3_afsb_dfs_13_single",
        "scip3_afsb_bfs_13_single",
        "0317_bPB_afsb_insL200_trjL5e7/policy.8"
    ]
    titles = [
        # "Oracle",
        "BES",
        "DFS",
        "BFS",
        "INS"
    ]
    # --------------------------------------------------------

    d_dir = os.path.join(data_dir, d_name)
    l_dir = os.path.join(training_files_base, "clip-scratch", "result", d_type, d_name)

    make_pgap(d_dir, l_dir, figs_name, d_name, exps, titles)