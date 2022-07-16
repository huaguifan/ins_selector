import os

class Node():
    def __init__(self, idx=-2, lowerbound=-1.00, primalbound=10000.00, dualbound=-1.00, branchvars=[], left=0, time=0.0, depth=0, obj_list=[10000]):
        self.idx = idx
        self.lowerbound = lowerbound
        self.primalbound = primalbound
        self.dualbound = dualbound
        self.branchvars = branchvars    # 历史分支变量
        self.left = left                # 当前优先队列中剩余节dian
        self.time = time                # 节点选择时间
        self.depth = depth
        self.obj_list = obj_list
        self.sol = 0.00
        self.bPB_time = 0.00
        self.nvars = 0
        self.varstime = 0.00

class InstanceFile():
    # TODO: 周末重新整理
    def __init__(self, log_file_path=""):
        self.bPB_time = 0.0
        self.log_file_lines = []
        self.max_node_number = 0
        self.presolving_time = 0
        self.best_primalbound = -1000000.0

        self.nodelist = self.get_all_nodes_from_log(log_file_path)

        self.bPB_time = self.get_bPB_time()
        self.sorted_nodelist = self.build_sorted_nodelist()

        self.presolving_time = self.get_presolving_time()
        self.best_primalbound_nodelist = self.get_multi_bPB() # 函数调用有顺序，不要随便打乱顺序

    def is_leaves(self, *nodes):
        for node in nodes:
            if node.idx == 1 or node.idx == -1:
                return False
        return True
    
    def get_presolving_time(self):
        if len(self.sorted_nodelist) <= 4:
            return self.sorted_nodelist[self.max_node_number].time
        else:
            return min(self.sorted_nodelist[2].time, self.sorted_nodelist[3].time)
        # p_time = -1.0
        # for node in self.nodelist:
        #     if node.idx == 2 or node.idx ==3:
        #         p_time = node.time
        #         return p_time

    def get_bPB_time(self):
        b_time = -1.0
        try:
            if abs(self.best_primalbound - self.nodelist[-1].primalbound) < 1:
                b_time = self.nodelist[-1].bPB_time
            else:
                b_time = self.nodelist[-1].time
        except:
            print("empty bPB")
        return b_time
        
    def get_all_nodes_from_log(self, log_file_path): 
        os.system("grep -rEn 'final selecting' %s | cut -f1 -d':' > %s_tmp" % (log_file_path, log_file_path))
        line_no = [int(n.strip("\n")) for n in open("%s_tmp" % log_file_path, "r").readlines()] # 1,2,...
        os.system("rm %s_tmp" % log_file_path)
        self.log_file_lines = open("%s" % log_file_path, "r").readlines()

        cur_nodelist = []
        for i in range(len(line_no)):
            # 每读一行，就定义一个node对象，并append到cur_nodelist            
            # examine line_no[i-1]+1 --- line_no[i]
            cur_node = self.get_scip_node(line_no[i-1], line_no[i])
            if cur_node != Node() or cur_node.primalbound != 10000.00 or cur_node.time == 0.0:
                cur_nodelist.append(cur_node)
                self.best_primalbound = max(cur_node.primalbound, self.best_primalbound)
                self.max_node_number = max(cur_node.idx, self.max_node_number)
                
        return cur_nodelist
    
    def get_new_sols(self, node1, node2):
        # node1 - node2
        obj_list_1 = node1.obj_list
        obj_list_2 = node2.obj_list
        
        diff = []
        obj_hash = {}

        for i, j in zip(obj_list_1, obj_list_2):
            if i in obj_hash.keys():
                obj_hash[i] += 1
            else:
                obj_hash[i] = 1
        for j in obj_list_2:
            if j in obj_hash.keys():
                obj_hash[j] -= 1
        for key in obj_hash:
            if obj_hash[key] >= 1:
                diff.extend([key]*int(obj_hash[key]))
        
        try:
            assert len(diff) == 1
            return diff[0]
        except:
            print("get_new_sols wrong")
            return min(diff)
        
    def get_multi_bPB(self):
        assert len(self.nodelist) != 0
        improvment_nodelist = []
        for i in range(1, len(self.nodelist)):
            
            if len(self.nodelist[i].obj_list) == 0:
                continue
            
            cur_obj = self.nodelist[i].obj_list
            pre_obj = self.nodelist[i-1].obj_list
            # 上一个节点处发现新的较好的可行解
            try:
                # print("tmp debug: ", self.nodelist[i].idx, self.nodelist[i].primalbound, self.best_primalbound)
                # input()
                if self.nodelist[i].primalbound == self.best_primalbound and self.nodelist[i-1].primalbound != self.best_primalbound:
                # if self.is_leaves(self.nodelist[i-1]) and (len(pre_obj) == 0 or cur_obj[0] != pre_obj[0]):
                    self.nodelist[i-1].sol = cur_obj[0]
                    improvment_nodelist.append(self.nodelist[i-1])
                    # print("tmp debug: in")
            except:
                print(len(cur_obj), len(pre_obj))
                print(cur_obj)
                print(pre_obj)
                input()

        # sorted(improvment_nodelist, key=lambda node:int(sum(node.obj_list)), reverse=True)
        
        return improvment_nodelist

    def get_scip_node(self, st, ed):
        # st, start line number
        # ed, end line number.   [st, ed]

        # if self.log_file_lines[ed-1].startswith("[src/scip/nodesel_estimate.c:") == True:
        #     return Node()
        
        node = Node()
        _splitted = self.log_file_lines[ed-1].strip().rstrip("\n").split()

        if _splitted[0] == "1:":
            splitted = _splitted[1:]
        else:
            splitted = _splitted
        idx = splitted[6]

        # ？？？？ why idx == 1
        if idx == -1 or idx == 1:
            return node
       
        node.idx = int(idx)
        node.primalbound = float(splitted[8])
        node.lowerbound = float(splitted[10])
        node.dualbound = float(splitted[12])

        if len(splitted) > 13:
            node.time = float(splitted[14])
            node.depth = int(splitted[16])
            node.left = int(splitted[18])
            try:
                node.bPB_time = float(splitted[20])
            except:
                node.bPB_time = 0
        if len(splitted) > 20:
            # nsols = int(splitted[20])
            node_obj_list = []
            for i in range(25, len(splitted), 1):
                if "obj" in splitted[i-1]:
                    node_obj_list.append(float(splitted[i]))
            node.obj_list = node_obj_list
            
        node_branchvars = []
        for i in range(ed-2, st-2, -1):
            f_l = self.log_file_lines[i].strip("\n")
            if f_l.startswith("<"):
                try:
                    t_var, rel, value = f_l.split(" ")
                except:
                    print("[bad t_var], %s" % f_l)
                var = {}
                if rel == "<=" and value == "0.0":
                    var["name"] = self.process_t_var(t_var)
                    var["value"] = int(0)
                elif rel == ">=" and value == "1.0":
                    var["name"] = self.process_t_var(t_var)
                    var["value"] = int(1)
                if var:
                    node_branchvars.append(var)
            elif f_l.startswith("[src/scip/nodesel"):
                break
            else:
                continue
            
        node.branchvars = node_branchvars
        # print(splitted)
        # print(node.idx, node.branchvars, node.bPB_time, node.time, node.primalbound)
        # input()
        return node
    
    # by node number
    def build_sorted_nodelist(self):
        """
        把节点列表按节点序号作为索引，重
        """
        sorted_nodelist = [Node()]*(self.max_node_number+1)
        for node in self.nodelist:
            try:
                sorted_nodelist[node.idx] = node
            except:
                print("Wrong, build_sorted_nodelist %d %d %d" %( len(sorted_nodelist), node.idx, self.max_node_number))
        
        return sorted_nodelist

    def process_t_var(self, t_var):
        assert t_var[0] == "<"
        assert t_var[-1]== ">"
        tv = t_var[1:-1]
        while tv.startswith("t_"):
            tv = tv.replace("t_", "")
        return tv

class InstanceFileOld():
    # TODO: 周末重新整理�?
    def __init__(self, log_file_path=""):
        self.log_file_lines = []
        self.best_primalbound = 10000
        self.max_node_number = 0
        self.bPB_time = 0

        self.nodelist = self.get_all_nodes_from_log(log_file_path)
        self.best_primalbound_nodelist = self.get_multi_bPB()
        self.sorted_nodelist = self.build_sorted_nodelist()

    def is_leaves(self, *nodes):
        for node in nodes:
            if node.idx == 1 or node.idx == -1:
                return False
        return True
        
    def get_all_nodes_from_log(self, log_file_path): 
        os.system("grep -rEn 'final selecting' %s | cut -f1 -d':' > %s_tmp" % (log_file_path, log_file_path))
        line_no = [int(n.strip("\n")) for n in open("%s_tmp" % log_file_path, "r").readlines()] # 1,2,...
        os.system("rm %s_tmp" % log_file_path)
        self.log_file_lines = open("%s" % log_file_path, "r").readlines()

        cur_nodelist = []
        for i in range(len(line_no)):
            # examine line_no[i-1]+1 --- line_no[i]
            cur_node = self.get_scip_node(line_no[i-1], line_no[i])
            if cur_node != Node() and cur_node.primalbound != -1:
                cur_nodelist.append(cur_node)
                self.best_primalbound = min(cur_node.primalbound, self.best_primalbound)
                self.max_node_number = max(cur_node.idx, self.max_node_number)
                
        return cur_nodelist
    
    def get_new_sols(self, node1, node2):
        # node1 - node2
        obj_list_1 = node1.obj_list
        obj_list_2 = node2.obj_list
        
        diff = []
        obj_hash = {}

        for i, j in zip(obj_list_1, obj_list_2):
            if i in obj_hash.keys():
                obj_hash[i] += 1
            else:
                obj_hash[i] = 1
        for j in obj_list_2:
            if j in obj_hash.keys():
                obj_hash[j] -= 1
        for key in obj_hash:
            if obj_hash[key] >= 1:
                diff.extend([key]*int(obj_hash[key]))
        
        try:
            assert len(diff) == 1
            return diff[0]
        except:
            print("get_new_sols wrong")
            return min(diff)
        
    def get_multi_bPB(self):
        assert len(self.nodelist) != 0
        improvment_nodelist = []    
        for i in range(1, len(self.nodelist)):
            
            if len(self.nodelist[i].obj_list) == 0:
                continue
            
            cur_obj = self.nodelist[i].obj_list
            pre_obj = self.nodelist[i-1].obj_list
            
            # 上一个节点处发现新的较好的可行解
            if self.is_leaves(self.nodelist[i-1]) and cur_obj[0] != pre_obj[0]:
                self.nodelist[i-1].sol = cur_obj[0]
                improvment_nodelist.append(self.nodelist[i-1])

        sorted(improvment_nodelist, key=lambda node:int(sum(node.obj_list)), reverse=False)
        # for node in improvment_nodelist:
        #     print(node.idx, node.sol, node.branchvars)
        #     input()
        return improvment_nodelist

    def get_scip_node(self, st, ed):
        # st, start line number
        # ed, end line number.   [st, ed]

        # assert self.log_file_lines[ed-1].startswith("[src/scip/nodesel_bfs.c:"), "Wrong selecting node number line!"
        if self.log_file_lines[ed-1].startswith("[src/scip/nodesel_estimate.c:") == True:
            # print("Wrong, get_scip_node: %s" % self.log_file_lines[ed-1])
            return Node()

        node = Node()
        _splitted = self.log_file_lines[ed-1].strip().rstrip("\n").split()

        if _splitted[0] == "1:":
            splitted = _splitted[1:]
        else:
            splitted = _splitted
        idx = splitted[6]

        if idx == -1 or idx == 1:
            return node
        node.idx = int(idx)
        node.primalbound = float(splitted[8])
        node.lowerbound = float(splitted[10])
        node.dualbound = float(splitted[12])
        if len(splitted) > 13:
            node.time = float(splitted[14])
            node.depth = int(splitted[16])
            node.left = int(splitted[18])
            # print(splitted, node.bPB_time)
            # input()

        if len(splitted) > 20:
            # nsols = int(splitted[20])
            node_obj_list = []
            for i in range(25, len(splitted), 1):
                if "obj" in splitted[i-1]:
                    node_obj_list.append(int(splitted[i]))
            node.obj_list = node_obj_list
            
        node_branchvars = []
        for i in range(ed-2, st-2, -1):
            f_l = self.log_file_lines[i].strip("\n")
            if f_l.startswith("<"):
                try:
                    t_var, rel, value = f_l.split(" ")
                except:
                    print("[bad t_var], %s" % f_l)
                var = {}
                if rel == "<=" and value == "0.0":
                    var["name"] = self.process_t_var(t_var)
                    var["value"] = int(0)
                elif rel == ">=" and value == "1.0":
                    var["name"] = self.process_t_var(t_var)
                    var["value"] = int(1)
                if var:
                    node_branchvars.append(var)
            elif f_l.startswith("[src/scip/nodesel"):
                break
            else:
                continue
            
        #node.time = int(splitted[18])
        #node.branchvars = node_branchvars
        # print(splitted)
        # print(node.idx, node.time)
        # input()
        return node
    
    # by node number
    def build_sorted_nodelist(self):
        """
        把节点列表按节点序号作为索引，重�??
        """
        sorted_nodelist = [Node()]*(self.max_node_number+1)
        for node in self.nodelist:
            try:
                sorted_nodelist[node.idx] = node
            except:
                print("Wrong, build_sorted_nodelist %d %d %d" %( len(sorted_nodelist), node.idx, self.max_node_number))
        
        pblist = self.best_primalbound_nodelist
        if len(pblist) != 0 and pblist[-1].sol == self.best_primalbound:
            self.bPB_time = self.best_primalbound_nodelist[-1].time - self.nodelist[1].time
        else:
            self.bPB_time = self.nodelist[-1].time - self.nodelist[1].time
        # print(self.bPB_time, self.max_node_number)
        
        return sorted_nodelist

    def process_t_var(self, t_var):
        assert t_var[0] == "<"
        assert t_var[-1]== ">"
        tv = t_var[1:-1]
        while tv.startswith("t_"):
            tv = tv.replace("t_", "")
        return tv
