#run with 
# gdb --batch --command=debug.gdb --args build/join_sim > debug.txt
# gdb --batch --command=debug.gdb --args build/join_sim 5 13 8 101 NULL NULL 0 0 0 > debug.txt

set width 0
set height 0
set verbose off
set print thread-events off
set debug libthread-db 0
set debug threads off

b main.cpp:334
command 1
  silent
  printf "invalid tuple1\n"
  continue
end

b main.cpp:351
command 2
  silent
  printf "invalid tuple2\n"
  continue
end


# b stages.h:97
# command 1
#   silent
#   printf "Pipe0 stalled\n"
#   continue
# end
# condition 1 temp0==0

# b stages.h:107
# command 2
#   silent
#   printf "Pipe1 stalled\n"
#   continue
# end
# condition 2 temp1==0

# b stages.h:117
# command 3
#   silent
#   printf "Pipe2 stalled\n"
#   continue
# end
# condition 3 temp2==0

# b stages.h:127
# command 4
#   silent
#   printf "Pipe3 stalled\n"
#   continue
# end
# condition 4 temp3==0





# b stages.h:404
# command 1
#   silent
#   printf "Pipe0 stalled\n"
#   continue
# end
# condition 1 wValid0==0

# b stages.h:412
# command 2
#   silent
#   printf "Pipe1 stalled\n"
#   continue
# end
# condition 2 wValid1==0

# b stages.h:421
# command 3
#   silent
#   printf "Pipe2 stalled\n"
#   continue
# end
# condition 3 wValid2==0

# b stages.h:430
# command 4
#   silent
#   printf "Pipe3 stalled\n"
#   continue
# end
# condition 4 wValid3==0



# b main.cpp:432
# command 1
#   printf "EOF count: %d\n", eof 
#   continue
# end


run

# backtrace