"""
Provisions EC2 instances and runs an experiment based on a
YAML-formatted experiment configuration.

Requirements:
- Amazon cli tools installed and working. Test with "aws ec2 describe-instances"
- SSH set up to use the provided key by default. Easiest way to do this is described below.
- "ubuntu" user with passwordless sudo on EC2 image (works in default ubuntu AMI)

SSH config:
- Make a key called "bench" in EC2 account
- Download the private key and put it in ~/.ssh
- Add the following to ~/.ssh/config:
    Host *.compute.amazonaws.com
    IdentityFile ~/.ssh/bench.pem

Basic usage:
    python experiment.py <experiment.yml> <name-of-experiment>
This creates a directory called <name-of-experiment> with complete logs of every 
process, by logical machine name.

To debug problems, do
    python experiment.py <experiment.yml> <name-of-experiment> --no-experiment

This will run the provisioning and setup scripts, then spin until killed with C-c. 
The script prints out the addresses of provisioned nodes, so you can SSH to them and debug problems.

Some example YAML files are in the experiments/ subdirectory.

"""

import time
import yaml
import json
import subprocess
import os
import sys
import shutil
import string
import pipes
from datetime import datetime
import argparse

USER = 'ubuntu'

INSTANCE_DEFAULTS = {
    'type': 't2.micro',
    'key' : 'bench',
    'image': 'ami-5189a661' # Canonical-provided Ubuntu AMI
}

def build_provision_command(type, image, key):
    return ['aws', 'ec2', 'run-instances',
            '--instance-type', type,
            '--image-id', image,
            '--key-name', key,
    ]

def start(type, image, key):
    cmd = build_provision_command(type, image, key)
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    (out, err) = proc.communicate()
    return json.loads(out)['Instances'][0]['InstanceId']

def terminate(instance_id):
    proc = subprocess.Popen(['aws', 'ec2', 'terminate-instances', '--instance-ids', instance_id], stdout=subprocess.PIPE)
    proc.communicate()

def scp(dirname, host):
    for i in range(10):
        proc = subprocess.Popen(['scp',
                                 '-o', 'UserKnownHostsFile=/dev/null',
                                 '-o', 'StrictHostKeyChecking=no',
                                 '-o', 'ConnectTimeout=2',
                                 '-q',
                                 '-r', dirname, '%s@%s:' % (USER, host)])
        time.sleep(4)
        res = proc.poll()
        if res not in [0, None]:
            print 'SCP connection to %s refused; retrying' % host
            continue
        proc.communicate()
        return
    else:
        print "Too many SCP retries, exiting"
        raise Timeout()
    
def get_names(instance_id):
    proc = subprocess.Popen(['aws', 'ec2', 'wait', 'instance-running', '--instance-id', instance_id], stdout=subprocess.PIPE)
    proc.communicate()
    proc = subprocess.Popen(['aws', 'ec2', 'describe-instances', '--instance-ids', instance_id], stdout=subprocess.PIPE)
    (out, err) = proc.communicate()
    instance_desc = json.loads(out)['Reservations'][0]['Instances'][0]
    return instance_desc['PrivateIpAddress'], instance_desc['PublicDnsName']

class Timeout(Exception):
    pass

def run_command(host, command, logfile):
    for i in range(10):
        proc = subprocess.Popen(['ssh', '-o', 'UserKnownHostsFile=/dev/null',
                                        '-o', 'StrictHostKeyChecking=no',
                                        '-o', 'ConnectTimeout=2',
                                 '%s@%s' % (USER, host), 'bash -x -c %s' % pipes.quote(command)],
                                stdout=logfile, stderr=logfile, bufsize=1)
        time.sleep(4)
        res = proc.poll()
        if res == 255:
            print 'SSH connection to %s refused; retrying' % host
            continue
        return proc
    else:
        print "Too many retries, exiting"
        raise Timeout()

def disjoint_merge(d1, d2, submerge_keys=[]):
    d = d1
    for k in d2:
        if k in submerge_keys:
            d1[k] = disjoint_merge(d1.get(k, {}), d2[k])
        elif k in d1:
            raise Exception("Maps not disjoint at %s" % k)
        else:
            d1[k] = d2[k]
    return d
        
def start_instances(config, experiment):
    for instance_name in config['instances']:
        instance = experiment['instances'][instance_name] = {}
        instance.update(INSTANCE_DEFAULTS)
        instance.update(config['instances'][instance_name] or {})
        print 'Starting %s as %s' % (instance_name, instance['type'])
        instance['instance_id'] = start(instance['type'], instance['image'], instance['key'])
    for instance_name in experiment['instances']:
        private, public = get_names(experiment['instances'][instance_name]['instance_id'])
        experiment['instances'][instance_name]['private'] = private
        experiment['instances'][instance_name]['public'] = public
        experiment['vars'][instance_name] = private
        print 'Instance %s at %s (private %s)' % (instance_name, public, private)
    print 'All instances started'

def provision_instances(config, experiment):
    if 'build_directory' in config:
        shutil.copytree(config['build_directory'], os.path.join(experiment['label'], 'build'))
        for instance_name in experiment['instances']:
            scp(os.path.join(experiment['label'], 'build'), experiment['instances'][instance_name]['public'])
    if 'provision' in config:
        procs = []
        for instance_name in experiment['instances']:
            logfile = open(os.path.join(experiment['label'], 'provision-%s.log' % instance_name), 'w')
            subs = dict(experiment['vars'])
            subs.update({'name': instance_name})
            subs.update(config['instances'][instance_name] or {})
            command = string.Template(config['provision']).substitute(subs)
            procs.append(run_command(experiment['instances'][instance_name]['public'], command, logfile))
        print "Provisioning started"
        for proc in procs:
            proc.wait()
    print "Provisioning complete"

def setup_instances(config, experiment):
    print "Setup started"
    for instance_name in config.get('setup', {}):
        logfile = open(os.path.join(experiment['label'], 'setup-%s.log' % instance_name), 'w')
        command = string.Template(config['setup'][instance_name]).substitute(experiment['vars'])
        run_command(experiment['instances'][instance_name]['public'], command, logfile)
    print 'Setup complete'

def do_experiment(config, experiment):
    procs = []
    lognames = []
    for instance_name in config.get('experiment', {}):
        logname = os.path.join(experiment['label'], 'experiment-%s-%s.log' % (instance_name, datetime.now().strftime('-%Y-%m-%d-%H%M%S')))
        logfile = open(logname, 'w')
        command = string.Template(config['experiment'][instance_name]).substitute(experiment['vars'])
        procs.append(run_command(experiment['instances'][instance_name]['public'], command, logfile))
        lognames.append(logname)
    for proc in procs:
        proc.wait()
    return lognames

def terminate_instances(experiment):
    print "Terminating instances"
    for instance_name in experiment['instances']:
        terminate(experiment['instances'][instance_name]['instance_id'])
    print "Done terminating instances"

def main(name, files, run_setup, run_experiment, run_cleanup, extra_vars, dump_yaml):
    print 'Starting experiment'
    experiment_label = os.path.join('experiment_logs', name + datetime.now().strftime('-%Y-%m-%d-%H%M%S'))
    os.mkdir(experiment_label)
    config = {}
    config['vars'] = {}
    for v in extra_vars:
        (var, _, val) = v.partition(':')
        config['vars'][var] = val
    for filename in files:
        shutil.copy(filename, experiment_label)
        with open(filename) as f:
            disjoint_merge(config, yaml.safe_load(f), ['vars'])

    experiment = {'label': experiment_label, 'vars': config.get('vars', {}), 'instances': {}}
    start_instances(config, experiment)
    if dump_yaml is not None:
        print "dumping yaml to %s" % dump_yaml
        with open(dump_yaml, 'w') as f:
            yaml.safe_dump(experiment, f)
    try:
        provision_instances(config, experiment)
        if run_setup:
            setup_instances(config, experiment)
        if run_experiment:
            time.sleep(10)
            do_experiment(config, experiment)
        elif run_cleanup:
            # if we're not running an experiment, but we are asked to terminate instances,
            # first wait for user to do whatever they're doing on the cluster.
            while True:
                # wait for kill
                time.sleep(1)
    finally:
        if run_cleanup:
            terminate_instances(experiment)
        else:
            print 'Warning: not running cleanup. Remember to manually terminate instances when done!'
        print 'Experiment complete. Results are in %s' % experiment['label']

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Run an experiment on EC2")
    parser.add_argument('name', help="Experiment name")
    parser.add_argument('-f', '--file', help="YAML experiment file", action='append', dest='files')
    parser.add_argument('--dump-instance-yaml', help="Print YAML with cluster information")
    parser.add_argument('--no-setup', help="Don't run setup. Implies --no-experiment.", action='store_true')
    parser.add_argument('--no-experiment', help="Don't run experiment", action='store_true')
    parser.add_argument('--no-cleanup', help="Don't run cleanup", action='store_true')
    parser.add_argument('--var', help="Add a variable:value mapping to the experiment", action='append', dest='extra_vars', default=[])
    args = parser.parse_args()
    print args
    if not args.files:
        print "No experiment files specified"
        sys.exit(1)
    else:
        if args.no_setup:
            args.no_experiment = True
        main(args.name, args.files, not args.no_setup, not args.no_experiment, not args.no_cleanup, args.extra_vars, args.dump_instance_yaml)
