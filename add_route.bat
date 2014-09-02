@echo off 
ipconfig /all | awk -f wlgw.awk
