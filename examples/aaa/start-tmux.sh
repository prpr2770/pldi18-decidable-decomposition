#!/bin/bash

function start-server-command {
  echo -e "rlwrap" "./raft" "3" "$1" 

}

tmux new-session -d -s 'server'
tmux split-window -h "$(start-server-command 0)"
tmux split-window -v -p 66 "$(start-server-command 1)"
tmux split-window -v -p 50 "$(start-server-command 2)"
tmux select-pane -L
exec tmux attach
