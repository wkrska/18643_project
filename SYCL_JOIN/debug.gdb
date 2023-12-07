#run with 
# gdb --batch --command=debug.gdb --args build/join_sim > debug.txt
# gdb --batch --command=debug.gdb --args build/join_sim 1 256 1 256 NULL NULL 0 0 0 > debug.txt

set width 0
set height 0
set verbose off
set print thread-events off
set debug libthread-db 0
set debug threads off

# b stages.h:267
# commands 1
#   printf "Key %d Val %d h0 %d\n", iTuple.key, iTuple.val, h0_avail 
#   continue
# end

b core.h:35
commands 1
  silent
  printf "Key %3d \tBucketKey %3d \tBucketFull %d \ts %3d\n", key, bucket[s].key, bucket[s].full, s 
  continue
end

b stages.h:262
commands 2
  silent
  printf "h0 %3d \th1 %3d\n", iTuple.h0, iTuple.h1
  continue
end

# b main.cpp:121
# commands 1
#   printf "Key %8d Hash %8d\n", key, hash 
#   continue
# end

# b main.cpp:126
# commands 2
#   printf "Key %8d Hash %8d\n", key, hash 
#   continue
# end

# b main.cpp:132
# commands 3
#   printf "Key %8d Hash %8d\n", key, hash 
#   continue
# end

# b main.cpp:142
# commands 4
#   printf "Key %8d Hash %8d\n", key_t, key
#   continue
# end

# b main.cpp:155
# commands 5
#   printf "Key %8d Hash %8d\n", key_t, key
#   continue
# end

# b main.cpp:167
# commands 6
#   printf "Key %8d Hash %8d\n", key_t, key
#   continue
# end

# b main.cpp:180
# commands 7
#   printf "Key %8d Hash %8d\n", key_t, key
#   continue
# end


run