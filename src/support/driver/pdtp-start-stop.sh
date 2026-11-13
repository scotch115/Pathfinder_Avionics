#!/bin/bash
case "$1" in
  start)
    echo "Starting pdtp server"
    modprobe cdc-acm
    start-stop-daemon -S -n pdtpserver -a /usr/bin/pdtpserver -- -d
    ;;
  stop)
    echo "Stopping pdtp server"
    start-stop-daemon -K -n pdtpserver --signal TERM
    ;;
  *)
    echo "Usage $0 {start|stop}"
    exit 1
esac
