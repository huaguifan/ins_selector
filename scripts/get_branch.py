import os

class Branching():
    def __init__(self, time = 0.0, nvars = 0):
        self.time = time
        self.nvars = nvars

def get_branching(log_file_path):
    os.system("grep -rEn 'final branching' %s > %s_tmp" % (log_file_path, log_file_path))
    branching_lines = [n.strip('\n') for n in open("%s_tmp" % log_file_path, "r").readlines()] 
    os.system("rm %s_tmp" % log_file_path)
    
    time, nvars = 0.0, 0.0

    for line in branching_lines:
        nvars += int(line.split(' ')[5])
        time += float(line.split(' ')[7])
        
    return time, nvars