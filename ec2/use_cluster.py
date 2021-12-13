import experiment as E
import argparse
import re
import yaml
import numpy as np
import sys

def read_config_and_experiment(config_filenames, instance_filename, extra_vars):
    config = {}
    config['vars'] = {}
    for config_filename in config_filenames:
        with open(config_filename) as f:
            E.disjoint_merge(config, yaml.safe_load(f), ['vars'])

    with open(instance_filename) as f:
        experiment = yaml.safe_load(f)

    for v in extra_vars:
        (var, _, val) = v.partition(':')
        experiment['vars'][var] = val
        
    return (config, experiment)

# ['experiments/ivyd-raft.yml', 'experiments/closed-loop.yml']
# 'current.yml'
def do_experiment(config_files, instance_file, threads=1, howlong='--requests 10000', keys=500):
    (cfg, exp) = read_config_and_experiment(config_files, instance_file,
                                            ['keys:%s' % keys, 'threads:%s' % threads, 'howlong:%s' % howlong])
    return E.do_experiment(cfg, exp)

def process_logfile(filename):
    with open(filename) as f:
        out = f.read()

    with open(filename) as f:
        lines = []
        for line in f:
            lines.append(line)
        for line in lines[-10:]:
            print line,

    time = float(re.search(r'Total time: (\d+\.?\d*)', out).group(1))
    tput = float(re.search(r'Throughput: (\d+\.?\d*)', out).group(1))
    glat = float(re.search(r'gets, avg = (\d+\.?\d*)', out).group(1))
    plat = float(re.search(r'puts, avg = (\d+\.?\d*)', out).group(1))
    
    return dict(logfile=filename, output=out, time=time, tput=tput, glat=glat, plat=plat)

def summary_stats(a):
    return "mean=%s std=%s" % (np.mean(a), np.std(a))

def analyze_results(results):
    analysis = []
    for threads, l in results:
        time = np.array([res['time'] for res in l])
        tput = np.array([res['tput'] for res in l])
        glat = np.array([res['glat'] for res in l])
        plat = np.array([res['plat'] for res in l])
        print "for %s threads" % threads
        print "  time: %s" % summary_stats(time)
        print "  tput: %s" % summary_stats(tput)
        print "  glat: %s" % summary_stats(glat)
        print "  plat: %s" % summary_stats(plat)
        analysis.append(((np.mean(tput), np.mean(glat)), (np.std(tput), np.std(glat))))

    print analysis
    return analysis


def main():
    parser = argparse.ArgumentParser(description="Use a running EC2 cluster to perform an experiment.")
    parser.add_argument('--config', help="YAML configuration file", action='append', dest='configs', default=[])
    parser.add_argument('--instance', help="YAML instance file", required=True, type=str)
    parser.add_argument('--threads', help="Number of client threads to use", action='append')
    parser.add_argument('--requests', type=int)
    parser.add_argument('--time', type=float)
    parser.add_argument('--iters', type=int, default=5)
    parser.add_argument('--keys', default=500, type=int)
    parser.add_argument('--terminate', help="Terminate the cluster", action='store_true')
    parser.add_argument('--use-processes', action='store_true')
    args = parser.parse_args()
    print args

    if args.terminate:
        (cfg, exp) = read_config_and_experiment(args.configs, args.instance, [])
        E.terminate_instances(exp)
        sys.exit(0)

    if args.threads is None:
        args.threads = [1]

    if args.requests is not None and args.time is not None:
        raise Exception("At most one of --requests and --time may be given")

    if args.requests is None and args.time is None:
        args.requests = 25000

    if args.requests is not None:
        howlong = '--requests %s' % args.requests

    if args.time is not None:
        howlong = '--time %s' % args.time

    if args.use_processes:
        # horrible hack to pass additional options to bench.py
        howlong += ' --use-processes'

    results = []
    for num_threads in args.threads:
        print 'with %s threads' % num_threads
        l = []
        results.append((num_threads, l))
        for i in range(args.iters):
            print '  iteration %s' % i
            log = do_experiment(args.configs, args.instance,
                                threads=num_threads, howlong=howlong, keys=args.keys)[0]
            print 'results from %s:' % log
            try:
                res = process_logfile(log)
                print 'time=%s tput=%s glat=%s plat=%s' % (res['time'], res['tput'], res['glat'], res['plat'])
                l.append(res)
            except Exception:
                print "experiment did not terminate normally"

    analyze_results(results)

    logs = []
    for num_threads, res_list in results:
        l = []
        logs.append((num_threads, l))
        for res in res_list:
            l.append(res['logfile'])

    print logs

if __name__ == '__main__':
    main()
