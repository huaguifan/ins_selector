# =================================================
# 用于与05文件的Policy策略多进程通信，需指定策略所在文件夹
# =================================================

# 使用示例：python ./scripts/06_server.py ~/daggerSpace/training_files/scip-dagger/trained_models/cauctions/train2-0_200_1000/0601_scip3_afsb_oracle_11_12/final_2022-06-20-16-03_nodelist/bak_insL200_trjL5e7/

#!/usr/bin/env python3
# http://weifan-tmm.blogspot.kr/2015/07/a-simple-turorial-for-python-c-inter.html
import os
import time
import sysv_ipc
import numpy as np
from email import policy

import argparse
import xgboost as xgb
from itertools import groupby
from type_definitions import *


# C发�?36维度feat，接收double
# pyhton发送double，接收feat
# 每隔一段时间更新模型参�?

FEATURE_SIZE = 20

def receive_from_c(client):
    message, mtype = client["receiver"].receive()
    numpy_message = np.frombuffer(message, dtype=np.double)
    # print(numpy_message)
    # input()

    length = len(numpy_message) - 1
    try:
        assert length == FEATURE_SIZE
    except:
        print(FEATURE_SIZE, length)

    message = numpy_message[0:length]
    numPolicy = numpy_message[length]
    
    return message, numPolicy


def send_to_c(data, client):
    client["sender"].send(data.tobytes(), block=True, type=TYPE_ARRAY)


def load_model(policy_id, policy_dir):
    
    policy_path = os.path.join(policy_dir, f'searchPolicy.{policy_id}.bin')
    model = xgb.Booster(model_file=policy_path)
    
    return model


def calc_score(input_trj, model):

    # model = xgb.Booster(model_file='searchPolicy.1.bin')
    # predict

    test_list = []
    test_list.append(input_trj)

    test_data = np.asarray(test_list)
    test_data = xgb.DMatrix(test_data)
    rank_score = model.predict(test_data)
    res = []
    res.append(0)
    res.append(rank_score[0])

    return np.array(res)

def get_experiment(experiment):

    if experiment == "":
        ticks = time.localtime(time.time())
        experiment = f'{ticks[0]}0{ticks[1]}{ticks[2]}'
    
    return experiment

def reverse_num(num):
    res_s = str(num)[::-1]
    res = int(res_s)
    if res > 1000000 or res < -1000000:
        res = 0
    return res

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-i', '--ipc_id',
        help='ipc id',
        type=int,
        default="1234",
    )
    args = parser.parse_args()

    rcv_id = args.ipc_id
    snd_id = reverse_num(rcv_id)

    c_client = {
        "receiver": sysv_ipc.MessageQueue(rcv_id, sysv_ipc.IPC_CREAT),
        "sender": sysv_ipc.MessageQueue(snd_id, sysv_ipc.IPC_CREAT)
    }

    # scip3+afsb+bfs
    policy_dir = "/home/xuliming/daggerSpace/training_files/scip-dagger/trained_models/setcover/2500_train_1000r_1000c_0.05d/2022-02-24-17-50_nodelist/insL200_trjL5e7"
    
    training_files_base = "/home/xuliming/daggerSpace/training_files/scip-dagger"
    
    numPolicy = -1
   
    while True:

        receive_feat, new_numPolicy = receive_from_c(c_client)
        print(receive_feat)
        print("policy ", int(numPolicy), int(new_numPolicy))

        if numPolicy != int(new_numPolicy):
            
            numPolicy = new_numPolicy
            model = load_model(int(new_numPolicy), policy_dir)
            print(f'Get a new policy: {policy_dir}searchPolicy.{int(numPolicy)}.bin')

        out = calc_score(receive_feat, model)

        print("NN result:", out)
        send_to_c(out, c_client)
