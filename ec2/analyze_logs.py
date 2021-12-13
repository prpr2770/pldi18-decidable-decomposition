from collections import defaultdict
import argparse
import re
import os
import numpy as np
import matplotlib.pyplot as plt

def read_log_file(filename):
    res = defaultdict(list)
    p = re.compile(r'REQUEST: THREAD (\d+) TIME (\d+\.?\d*) LATENCY (\d+\.?\d*)')
    for line in open(filename).read().splitlines():
        match = p.match(line)
        if match is not None:
            thread = int(match.group(1))
            time = float(match.group(2))
            latency = float(match.group(3))
            res[thread].append((time, latency))
    assert sorted(res.keys()) == range(len(res))
    res = [res[i] for i in range(len(res))]
    return res

# A graph for each of the 5 experiments for some configuration
def plot_config_separate(ks, ls, e):
    for i in range(5*e, 5*e+5):
        threads = len(ls[i])
        plt.figure()
        # plt.hold(True)
        plt.xlabel('Requests')
        plt.ylabel('Time [sec]')
        fn = ks[i]
        plt.title('{} threads, {}'.format(threads, fn))
        for j in range(threads):
            plt.plot(ls[i][j][:,0],'-')

def plot_config_together(ks, ls, e):
    plt.figure()
    #plt.hold(True)
    plt.xlabel('Requests')
    plt.ylabel('Time [sec]')
    threads = len(ls[5*e])
    fn = ks[5*e]
    plt.title('{} threads, {}'.format(threads, fn[-11:]))
    for i in range(5*e, 5*e+5):
        assert threads == len(ls[i])
        for j in range(threads):
            plt.plot(ls[i][j][:,0],'-')


def plot_cumulative(ks, ls, e):
    plt.figure()
    #plt.hold(True)
    plt.xlabel('Requests by all threads')
    plt.ylabel('Time [sec]')
    threads = len(ls[5*e])
    protocol = ks[5*e][16:40]
    plt.title('{} threads, {}'.format(threads, protocol))
    for i in range(5*e, 5*e+5):
        assert threads == len(ls[i])
        assert protocol == ks[i][16:40]
        all_times = np.concatenate(ls[i])[:,0]
        all_times.sort()
        fn = ks[i]
        plt.plot(all_times,'-', label=fn[-11:])
    plt.legend(loc='best')

def avg_throughput(ks, ls, e):
    s = 0.0
    for i in range(5*e, 5*e+5):
        for j in range(len(ls[i])):
            s += len(ls[i][j]) / (ls[i][j][-1][0] - ls[i][j][0][0])
    s /= 5.0
    return s

def avg_latency(ks, ls, e):
    s = 0.0
    n = 0
    for i in range(5*e, 5*e+5):
        for j in range(len(ls[i])):
            s += np.mean(ls[i][j][:,1])
            n += 1
    s /= n
    return s

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Analyze logs from bench.py.")
    parser.add_argument('files', help="Log file from bench.py", nargs='+')
    args = parser.parse_args()
    files = args.files
    logs = {}
    print "Parsing {} files".format(len(files))
    for f in files:
        print "Reading {}".format(f)
        logs[f] = read_log_file(f)
    print "Done"

    ks = sorted(logs.keys())
    ls = [[np.array(y) for y in logs[x]] for x in ks]


    e = 29
    plot_config_separate(ks, ls, e)


    # A graph for each configuration (all threads of all 5 experiments on the same graph)
    es = [0,1,2,3,5,7,9,12,14][:]
    #es = [15 + x for x in es]
    #es = es + [15 + x for x in es]
    #es = range(30)
    for e in es:
        plot_config_together(ks, ls, e)

    # Commulative throuhput traces - time vs. total number of requests processed by all threads
    # A graph for each configuration (all 5 experiments on the same graph)
    es = [0,1,2,3,5,7,9,12,14][3:]
    es = [15 + x for x in es]
    #es = es + [15 + x for x in es]
    #es = range(30)
    for e in es:
        plot_cumulative(ks, ls, e)
