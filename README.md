## 准备工作

### 安装scipoptsuite-3.1.0
1. 下载scipoptsuite-3.1.0.tgz并解压
2. 将本文件夹内的scip-3.1.0.tgz拷贝到scipoptsuite-3.1.0
3. 安装scipoptsuite-3.1.0，安装命令make OPT=dbg READLINE=false -j16
4. 显示build complete表示安装成功

### 安装SCIPDAGGER
1. cd ./src
2. 安装SCIPDAGGER，安装命令make OPT=dbg READLINE=false -j16
3. 安装成功

## 开始实验
```
# 运行SCIP得到SOL文件
bash ./scripts/01_muti_run.sh -d cauctions -i test_100_730 -e 0629_scip3_afsb_bfs_multi_12 -x .lp -s ./sets/allfullstrong_bfs.set -t 36000 -u ./bin/scipdagger-0621-2 -n 100
    # -i setcover下数据文件名
    # -e 实验名
    # -s scip设置文件，指定分支及子问题选择规则等
    # -t scip运行时间限制
    # -n 多进程最大进程数
    # -u 可执行文件名，默认为./bin/scipdagger-0622

# 运行oracle策略，得到trj训练数据
bash ./scripts/02_muti_run_oracle.sh -d facilities/train_200_100_5 -s ./sets/allfullstrong_bfs.set -x .lp -e 0624_scip3_afsb_oracle_11_multi -t 36000 -n 90 -u ./bin/scipdagger-0622

# 03_make_data.py: 将上一步用oracle策略求解原始问题得到的trj训练数据整理成训练所需的格式
python ./scripts/03_make_data.py

# 开始训练，需指明训练数据文件所在路径
python ./scripts/04_train.py -t cauctions -d train2-0_200_1000 -e 0601_scip3_afsb_oracle_11_12 --train_file_path ~/daggerSpace/training_files/scip-dagger/clip-scratch/training/trj/cauctions/train2-0_200_1000/0601_scip3_afsb_oracle_11_12/train_pickle_json/2022-06-20-16-03_nodelist.pickle

# 测试得到的模型参数在数据集上的结果
# 运行进程间通信服务端
python ./scripts/06_server.py ~/daggerSpace/training_files/scip-dagger/trained_models/cauctions/train2-0_200_1000/0601_scip3_afsb_oracle_11_12/final_2022-06-20-16-03_nodelist/bak_insL200_trjL5e7/
# 开始测试
python ./scripts/05_run_diff_policy.py -a scip -t cauctions -d test_100_620 -e 0629_scip3_afsb_bfs_12_single -s ./sets/allfullstrong_bfs.set -k 500

# 08_get_stats.py: 统计测试集上的统计信息，平均求解时间、平均节点数、找到最优解的时间等
python ./scripts/08_get_stats.py -t cauctions -d test_200_1000 -e 0627_scip3_afsb_oracle_12_single -k 20