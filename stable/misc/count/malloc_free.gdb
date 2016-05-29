set $c_string=0
b main.c:161
commands
silent
set $c_string+=1
printf "m %s\n", string
c
end
b main.c:536
commands
silent
set $c_string+=1
printf "m %s\n", value
c
end
b main.c:576
commands
silent
set $c_string+=1
printf "m %s\n", value
c
end
b main.c:266
commands
silent
set $c_string-=5
printf "f %s\n", conf->log_file_name
printf "f %s\n", conf->bak_file_name
printf "f %s\n", conf->data_file_name
printf "f %s\n", conf->tmp_file_name
printf "f %s\n", conf->call_system
c
end
b main.c:721
commands
silent
set $c_string-=1
printf "f %s\n", server->name
c
end

set $c_Table2D=0
b Table2DInit
commands
silent
set $c_Table2D+=2
c
end
b main.c:181
commands
silent
set $c_Table2D-=2
c
end

set $c_Url=0
b main.c:352
commands
silent
set $c_Url+=2
c
end
b UrlDestroy
commands
silent
set $c_Url-=2
c
end

set $c_HotServer=0
b main.c:801
commands
silent
set $c_HotServer+=1
c
end
b main.c:991
commands
silent
set $c_HotServer-=1
c
end

set $c_HotUrl=0
b 820
commands
silent
set $c_HotUrl+=1
printf "M %x\n", url
c
end
b main.c:747
commands
silent
set $c_HotUrl-=1
printf "F %x\n", p
c
end
#b main.c:984
#commands
#silent
#set $c_HotUrl-=1
#printf "f %x\n", events[i].data.ptr
#c
#end

set $c_LinkedList=0
b LinkedListCreate
commands
silent
set $c_LinkedList+=1
c
end
b LinkedListDestroy
commands
silent
set $c_LinkedList-=1
c
end

set $c_LinkedListNode=0
b LinkedListCycleInsert
commands
silent
set $c_LinkedListNode+=1
c
end
b LinkedListDelete
commands
silent
set $c_LinkedListNode-=1
c
end

def pm
printf "$c_string=%d\n",$c_string
printf "$c_Table2D=%d\n",$c_Table2D
printf "$c_Url=%d\n",$c_Url
printf "$c_HotServer=%d\n",$c_HotServer
printf "$c_HotUrl=%d\n",$c_HotUrl
printf "$c_LinkedList=%d\n",$c_LinkedList
printf "$c_LinkedListNode=%d\n",$c_LinkedListNode
end

r
pm
