#run with 
# gdb --batch --command=debug.gdb --args build/join_sim > debug.txt
# gdb --batch --command=debug.gdb --args build/join_sim 5 13 8 101 NULL NULL 0 0 0 > debug.txt

set width 0
set height 0
set verbose off
set print thread-events off
set debug libthread-db 0
set debug threads off

b main.cpp:446
commands 1
  
  printf "got here"
  continue
end

run