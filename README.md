# PLDI 2018 Artifact
[Marcel Taube's website](https://www.cs.tau.ac.il/~marcelotaube/modularity-for-decidability.html)
This virtual machine contains the artifact for PLDI 2018 paper:
"Modularity for Decidability: Implementing and Semi-Automatically
Verifying Distributed Systems".

The virtual machine was created using VirtualBox version 5.2.6.

The VM should be logged on with the user name `ivyuser` and password `ivy`.

Most of the evaluation of the artifact is done from the terminal,
which can be started by clicking on the terminal icon in the launcher
(top left of the screen). To read (and modify) source files, any text
editor can be used. The VM comes equipped with an Emacs mode for Ivy
code (Emacs can be launched from the terminal or the launcher).

The rest of this file explains the artifact and how to exercise
it.

# Directory structure

`~/z3/` contains a clone of Z3 from its master branch on github.

`~/ivy/` contains a clone of Ivy from its master branch on github.

`~/pldi18-artifact/` contains the artifact

`~/pldi18-artifact/examples/raft/` contains Ivy source files for the
verified Raft protocol implementation.

`~/pldi18-artifact/examples/multi-paxos/` contains Ivy source files for
the verified Multi-Paxos protocol implementation.

`~/pldi18-artifact/evaluation/` contains unverified C++ and Python
code that uses one of the verified protocols to create a distributed
fault tolerant key-value store, as well as a client that exercises the
server code. This directory also contains various scripts used for
performance evaluation and explained later.

`~/pldi18-artifact/ec2/` contains scripts that enable performance evaluation
of the key-value store on Amazon AWS.

# Verifying protocols with Ivy

## Raft

Ivy files contain both code and proofs (e.g., pre and post
conditions,inductive invariants), and are checked with the `ivy_check`
command, which internally uses Z3 to discharge verification
conditions. The Raft protocol implementation (and proof) are contained
in the `raft.ivy` file. It can be verified with ith `ivy_check` by
running the following commands in a terminal:

```
ivyuser@ivy-vm:~$ cd ~/pldi18-artifact/examples/raft/
ivyuser@ivy-vm:~/pldi18-artifact/examples/raft$ time ivy_check raft.ivy # should take several minutes
```

You should expect this to take a few minutes (2-3 on my laptop). It
will also print messages detailing the various verification conditions
being checked (the verification is split into several isolates, which
correspond to modules from the paper, and each isolate results in
multiple verification conditions). Once `ivy_check` terminates and
prints `OK`, you know that the file is verified.

You may wish to edit `raft.ivy`, for example by commenting out one of
the conjectures that make up the inductive invariant, and see that
`ivy_check` fails. If you run `ivy_check diagnose=true raft.ivy`, you
can also obtain a graphical interface that allows you to examine a
counterexample to induction (note that this interface is not part of
the contribution of the PLDI 2018 paper).

## Multi-Paxos

The source code of the Multi-Paxos verified protocol is split into two
files: `multi_paxos_protocol.ivy` contains the abstract protocol, and
`multi_paxos_system.ivy` contains the concrete implementation (and
uses the abstract protocol as ghost code for the correctness
proof). To check the Multi-Paxos implementation with `ivy_check`, run the following
commands in a terminal (this also checks the abstract protocol, which is used by the implementation):

```
ivyuser@ivy-vm:~$ cd ~/pldi18-artifact/examples/multi-paxos/
ivyuser@ivy-vm:~/pldi18-artifact/examples/multi-paxos$ time ivy_check multi_paxos_system.ivy # should take several minutes
...
```

As before, the above commands will print details about the
verification conditions being checked. `ivy_check` should take a few
minutes (7 on my laptop). As before, once `ivy_check` prints `OK` you
know that the protocol is verified. You can also edit the Ivy files to
break the verification, and run `ivy_check` to see it report the
errors, or run `ivy_check diagnose=true` to graphically examine
counterexamples to induction.

# Extracting verified C++ code

Ivy files can be compiled to C++, as explained in the paper. This is
done using the `ivy_to_cpp` command.

## Raft

To extract a verified implementation of the Raft protocol, run the following commands:

```
ivyuser@ivy-vm:~$ cd pldi18-artifact/examples/raft/
ivyuser@ivy-vm:~/pldi18-artifact/examples/raft$ ivy_to_cpp isolate=test target=class raft.ivy
```

This should take a few seconds, and generate the files `raft.cpp` and
`raft.h`, which contain a class `raft` with a verified implementation of the Raft protocol.

## Multi-Paxos

To extract a verified implementation of the Multi-Paxos protocol, run the following commands:

```
ivyuser@ivy-vm:~$ cd pldi18-artifact/examples/multi-paxos/
ivyuser@ivy-vm:~/pldi18-artifact/examples/multi-paxos$ ivy_to_cpp isolate=iso_impl target=class multi_paxos_system.ivy
```

This should take a few seconds, and generate the files
`multi_paxos_system.cpp` and `multi_paxos_system.h`, which contain a
class `multi_paxos_system` with a verified implementation of the
Multi-Paxos protocol.

# Evaluation

The process described above results in a a C++ class that implements
either Raft or Multi-Paxos. For evaluation purposes, we created a
server that can use any of these classes to implement a fault tolerant
distributed key-value store. This section explains how to build this
server and exercise it locally using a Python client.

## Building the server

To build the server using the verified Raft protocol, run:

```
ivyuser@ivy-vm:~$ cd ~/pldi18-artifact/evaluation/
ivyuser@ivy-vm:~/pldi18-artifact/evaluation$ make raft
```

To build it using the verified Multi-Paxos protocol instead, run:

```
ivyuser@ivy-vm:~/pldi18-artifact/evaluation$ make multipaxos
```

Boh the `make raft` command and the `make multipaxos` command create a
binary file called `build/server`. This allows the rest of the
evaluation (and the scripts it uses) to be uniform, since it simply
invokes `build/server`. Thus, the rest of the evaluation should be
executed twice: once after running `make raft` to evaluate a key-value
store that uses the verified Raft protocol, and once after running
`make multipaxos` to verify a key-value store that uses the verified
Multi-Paxos protocol.

In the rest of this section, all paths are relative to `~/pldi18-artifact/evaluation`.

## Running a cluster of servers

To run the distributed system locally, first build `./build/server`
using either `make raft` or `make multipaxos` as described
above. Then, start the cluster of servers by running several instances
of `./build/server` with appropriate command line arguments. For your
convenience, we have provided a script, `./scripts/start-tmux.sh`,
that will start a `tmux` session with multiple windows: one window for
each node running a server, and one extra window for interacting with
the cluster as a client (as explained next). The
`./scripts/start-tmux.sh` script takes a command line argument with
the number of servers in the cluster (with a default of 3).

If you want to set up the cluster manually, you can run several
instances the `./build/server` executable as in the following example
(a complete list of command line arguments is given by `./build/server
--help`):

```
ivyuser@ivy-vm:~/pldi18-artifact/evaluation$ ./build/server --log --node-id 0 --client-port 8000 --cluster localhost:4990,localhost:4991,localhost:4992
ivyuser@ivy-vm:~/pldi18-artifact/evaluation$ ./build/server --log --node-id 1 --client-port 8001 --cluster localhost:4990,localhost:4991,localhost:4992
ivyuser@ivy-vm:~/pldi18-artifact/evaluation$ ./build/server --log --node-id 2 --client-port 8002 --cluster localhost:4990,localhost:4991,localhost:4992
```

These are exactly the commands run by `./scripts/start-tmux.sh`. Each
server process listens for client connections on the given client
port, and talks to the servers at the given cluster addresses and
ports. The `--node-id` option tells a server which element of cluster
list contains its own address and port. The command
`./scripts/start-tmux.sh N` starts `N` servers, where
server `i` will use port `8000 + i` to communicate with clients, and
port `4990 + i` to communicate with the other servers.

If you start a tmux session with `./scripts/start-tmux.sh`, you should
make sure to properly close it before attempting to start a new
one. To do this, hit `CTRL+D` in the client window to close the shell,
and then `CTRL+C` once per server window. Continue until the entire
tmux session is terminated. This will be indicated by an `[exited]`
message, back in your original shell where you ran
`./scripts/start-tmux.sh` (which will now become visible again).

## Runnign an interactive client

After you launch a cluster of servers, you can interact with the
cluster by launching a client and sending `put` and `get` commands to
the key-value store. For example, you can make simple queries by
running `python` or `ipython` in the `bench/` directory and using the
provided Python API (note that keys and values are expected to be
strings):

```
from vard import Client

cluster = [("localhost", 8000), ("localhost", 8001), ("localhost", 8002)]

leader_host, leader_port = Client.find_leader(cluster)
c = Client(leader_host, leader_port)

c.put("x", "7")
c.put("y", "10")
c.get("x") # returns "7"
c.get("y") # returns "10"
c.get("z") # returns None
```

Make sure to change the `cluster` variable above to match with the
server cluster.

If you use IPython, you can use `!kill <pid>` to kill some of the
servers, in order to exercise the fault tolerance of the system. Each
server window starts with a line that includes its process id. For
example, you can try the following:

```
ivyuser@ivy-vm:~/pldi18-artifact/evaluation$ ./scripts/start-tmux.sh 5
# Inside the client window:
ivyuser@ivy-vm:~/pldi18-artifact/evaluation$ cd bench/
ivyuser@ivy-vm:~/pldi18-artifact/evaluation/bench$ ipython
...
In [1]: from vard import Client
In [2]: cluster = [("localhost", 8000), ("localhost", 8001), ("localhost", 8002), ("localhost", 8003), ("localhost", 8004)]
In [3]: leader_host, leader_port = Client.find_leader(cluster)
In [4]: c = Client(leader_host, leader_port)
In [5]: c.put("x", "7")
In [6]: c.put("y", "10")
In [7]: c.get("x") # returns "7"
In [8]: c.get("y") # returns "10"
In [9]: c.get("z") # returns None
In [10]: !kill 6757 # change 6757 to a process id from the window on the right
In [11]: c.get("x") # still returns '7'
In [12]: !kill 6762 # change 6762 to a process id from the window on the right
In [13]: c.get("x") # still returns '7'
In [14]: !kill 6748 # change 6748 to a process id from the window on the right
In [15]: c.get("x") # hangs, since too many servers have failed (no active quorum)
^C
```

When killing servers, you may note that if you kill the leader, the
next client request will result in an `ReceiveError` exception. To
recover, simply create a new client session by repeating the commands:
```
leader_host, leader_port = Client.find_leader(cluster)
c = Client(leader_host, leader_port)
```

The `Client` object represents a connection to a specific leader (the
client communicates only with the leader, and the leader uses Raft or
Multi-Paxos to communicate requests to other nodes and reach
agreement). In a realistic application, this sort of communication
failure would be caught and handled by automatically running the
`find_leader` method again, and establishing a new client connection.

Note that if you try to find a new leader while the leader-election
protocol has not terminated, you will receive an error.  Simply try
again a few seconds later.  Also note that restarting a failed server
is not supported.

## Running a benchmark client

You can also perform a local benchmarking run using the script
`./bench/bench.py` (note that you must start the servers before
running this script). If you first ran some simple queries, either
close the existing python session or open a new terminal and run the
following command. It may also be interesting to interact with the
cluster manually via the python API during or after benchmarks are
running.

If you started a cluster of 3 nodes, you can start `./bench/bench.py` with the following arguments:

```
ivyuser@ivy-vm:~/pldi18-artifact/evaluation$ python ./bench/bench.py --cluster localhost:8000,localhost:8001,localhost:8002 --requests 10 --threads 1 --keys 10 --put-percentage 50
```

The `--cluster` option specifies the addresses and *client* ports of
the servers in the cluster.  The rest of the options specify a
benchmark consisting of 10 randomly generated requests, running at
most one request in parallel, over a key space with 10 keys, where on
average 50% of requests are puts. You may wish to play with the number
of requests, threads, keys, and precentage of put requests (given by
command line arguments). Our paper reports measurements taken with 500
keys, 50 threads, and 10000 requests.

Each run of `./bench/bench.py` reports the average latency, as well as
a detailed log of all requests. This allows to examine whether the
latency increases with the size of the log (it should not). You can
also try to run `./bench/bench.py` several times, possibly killing
nodes between runs to exercise fault tolerance. You can also kill
nodes during a sufficiently long `./bench/bench.py` run, but notice
that if you kill the leader during a run, it would not recover. As
explained above, a real application would be able to recover by
starting a new session. To simulate this, you can kill the leader
between multiple `./bench/bench.py` runs.

Finally, note that the Multi-Paxos implementation has a known
issues: changing leader after a sufficiently long benchmark causes
nodes to try to send messages that do not fit in a UDP frame,
resulting in those nodes being terminated.

# Evaluation on Amazon EC2

If you have access to an Amazon AWS account, you can reproduce the
paper's evaluation by running our key-value store, as well as `vard`
and `etcd`, on EC2. Spinning up on-demand instances, cleaning them up,
and running the experiments is all automated, but some configuration
is necessary.

## EC2 Configuration

You'll need to do a couple of things in the AWS EC2 console:

- Ensure that port 22 (SSH) is open in your default security group
  (you can do this from "Security Groups" under "Network & Security")
- Create a key pair called "bench" (you can do this from "Key Pairs"
  under "Network & Security"). Download the resulting private key and
  put it in the VM as `/home/ivyuser/.ssh/bench.pem` and run `chmod
  600 /home/ivyuser/.ssh/bench.pem`.

Once you've done this, you can configure the VM to access your EC2
account by running (in the VM):

```
aws configure
```

and entering your Access Key ID, Secret Access Key, and default region
as appropriate (if you don't have a region preference, enter `us-west-2`).

## EC2 experiments

The scripts to run on EC2 are in `~/pldi18-artifact/ec2/`. The main
entry points are `experiment.py` and `use_cluster.py`. In what follows, we assume that you
have run `ivy_to_cpp` as explained above, so the C++ verified
implementations are already created. To run on EC2, `experiment.py`
copies C++ source files to EC2 instances and compiles them there. Note
that the EC2 instances themselves do not have Ivy (or Z3) installed,
since they are only used to run the resulting application.

To run experiments, you can start a cluster by running `experiment.py <yaml-script>
<experiment-label>`. For example, to start the Raft-based
key-value store experiment from the paper, run:

```
ivyuser@ivy-vm:~/pldi18-artifact/ec2$ python2 experiment.py -f experiments/ivyd-raft.yml raft-aec --no-experiment --no-cleanup --dump-instance-yaml raft-instances.yml
```

This will spin up 4 `m4.xlarge` EC2 instances, provision them as
necessary, and start the Raft-based key-value store on three of them.
You can expect this to take 5-10 minutes.

Then, you can use the cluster to perform benchmarks by running

```
ivyuser@ivy-vm:~/pldi18-artifact/ec2$ python2 use_cluster.py --config experiments/ivyd-raft.yml --config experiments/closed-loop.yml --instance raft-instances.yml --time 10 --iters 5 --use-processes --threads 1 --threads 2 --threads 4 --threads 8 --threads 16 --threads 32 --threads 64
```

This will run a benchmarking script from the fourth server to test the
performance of the cluster under various loads.  Each test runs the
benchmark script for 10 seconds with a certain number of client
processes and measures throughput and latency. Each
10-second experiment is repeated 5 times (specified by `--iters 5`).
The entire benchmark (5 iterations of ~10 seconds each for each number of threads)
should take about 10 minutes.

When the benchmark script finishes, you can terminate all the nodes
by running

```
ivyuser@ivy-vm:~/pldi18-artifact/ec2$ python2 use_cluster.py --config experiments/ivyd-raft.yml --config experiments/closed-loop.yml --instance vard-instances.yml --terminate
```

The logs from every process
(the provisioning script, the servers, and the client) are written to
a new directory in `experiment_logs` called
`raft-aec-<timestamp>`. You can also examine these files while the
experiment is running. The benchmark script's output will be in the
`experiment-client-<timestamp>.log` files in that directory. The end of the file
should contain the total time, throughput, average latency. The file
also contains a list of individual latencies for every request.

Each `.yml` file in the `experiments` directory corresponds to an
experiment from the paper:

- `ivyd-raft.yml`: The Raft-based key-value store
- `ivyd-multipaxos.yml`: The Multipaxos-based key-value store
- `etcd.yml`: The etcd key-value store
- `vard.yml`: The `vard` key-value store

The provisioning scripts for `vard` and `etcd` download sources from
their respective repositories. All provision scripts download the
benchmarking scripts from the `vard` repository, since we used the
`vard` benchmarks.

To try the other experiments, you can use the following commands:

```
python2 experiment.py -f experiments/ivyd-raft.yml raft-aec --no-experiment --no-cleanup --dump-instance-yaml raft-instances.yml
python2 experiment.py -f experiments/ivyd-multipaxos.yml multipaxos-aec --no-experiment --no-cleanup --dump-instance-yaml multipaxos-instances.yml
python2 experiment.py -f experiments/etcd.yml etcd-aec --no-experiment --no-cleanup --dump-instance-yaml etcd-instances.yml
python2 experiment.py -f experiments/vard.yml vard-aec --no-experiment --no-cleanup --dump-instance-yaml vard-instances.yml
```

And then run the `use_cluster.py` commands as above.

Note that the `vard` experiment takes significantly longer to set up the cluster since it
installs Verdi on the EC2 instances. Also note that if you see messages like `SSH connection to
... refused; retrying`, you can wait patiently and let the script
recover the connection and continue running. Should you become
impatient and interrupt the script, the instances will also be
terminated shortly. Overall, one should be patient with the
`experiment.py` script, since it performs some operations that can
take minutes to complete (e.g., cause a cloud instance to install
software from a remote repository).
