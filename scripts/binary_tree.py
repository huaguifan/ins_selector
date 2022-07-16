# ======================
# 脚本还需改进，暂时未使用
# ======================

import os
import networkx as nx
import matplotlib.pyplot as plt

class Node(object):
    def __init__(self, number=-1, lowerbound=-1.0, dualbound=-1.0, primalbound=-1.0, opt=-1, left=None, right=None, time=-1):
        """
        param:
        self.lb
        self.left
        self.right
        """
        self.number = number
        self.lowerbound = lowerbound
        self.dualbound = dualbound
        self.primalbound = primalbound
        self.opt = opt
        self.left = left
        self.right = right
        self.time = -1

class Tree(object):
    def __init__(self):
        self.root = None

def get_node(f_lines, line_idx):
    """
    根据一行日志，构建一个二叉树节点
    """
    if "checking" in f_lines[line_idx]:
        # print("return %s"% f_lines[line_idx])
        return Node()

    node = Node()
    splitted = f_lines[line_idx].rstrip("\n").split(" ")
    idx = splitted[6]

    try:
        lowerbound = splitted[10]
        primalbound = splitted[8]

        node.number = int(idx)
        node.lowerbound = float(lowerbound)
        node.primalbound = float(primalbound)
        node.time = int(splitted[-1])

        if line_idx + 2 < len(f_lines) and "checking" in f_lines[line_idx+1]:
            left_splitted = f_lines[line_idx+1].rstrip("\n").split(" ")
            right_splitted = f_lines[line_idx+2].rstrip("\n").split(" ")
            left_child = Node(number=int(left_splitted[4]))
            right_child = Node(number=int(right_splitted[4]))
            node.left = left_child
            node.right = right_child
            print(node.number, node.left.number, node.right.number)
    except:
        print("ERROR idx", line_idx)
        print(splitted)
        input()
        
    return node

def create_graph(G, node, pos={}, x=0, y=0, layer=1):
    """
    迭代画节点和边
    TODO:自动调整位置
    """
    pos[node.number] = (x, y)
    if node.left and node.left.number != -1:
        G.add_edge(node.number, node.left.number)
        l_x, l_y = x - (1 / 2) ** layer, y - 0.5
        print(l_x, l_y)
        # input()
        l_layer = layer + 1
        create_graph(G, node.left, x=l_x, y=l_y, pos=pos, layer=l_layer)
    if node.right and node.right.number != -1:
        G.add_edge(node.number, node.right.number)
        r_x, r_y = x + 1 / 2 ** layer, y - 0.5
        r_layer = layer + 1
        create_graph(G, node.right, x=r_x, y=r_y, pos=pos, layer=r_layer)
    return (G, pos)

def draw_tree(node):
    """
    画一棵二叉树
    """
    graph = nx.DiGraph()
    graph, pos = create_graph(graph, node)
    fig, ax = plt.subplots(figsize=(80, 100))
    nx.draw_networkx(graph, pos, ax=ax, node_size=100)
    plt.show()
    pass

if __name__ == "__main__":

    log_file_name = "./instance_31.log"

    os.system("grep -rEn 'final|checking' %s > %s_tmp" % (log_file_name, log_file_name))
    # os.system("rm %s_tmp" % log_file_name)
    log_file_lines = open(log_file_name+"_tmp", "r").readlines()

    # 创建node_list，此时左右子节点只有一个节点序号
    node_list = [Node()] * 1000
    for i in range(len(log_file_lines)):
        cur_node = Node()
        cur_node = get_node(log_file_lines, i)
        node_list[cur_node.number] = cur_node
        
    # 构建二叉树
    for node in node_list:
        if node.left != None:
            node.left = node_list[node.left.number]
            node.right = node_list[node.right.number]
            print(node.number, node.left.number, node.right.number )
        # print(node.number)
    
    draw_tree(node_list[1])