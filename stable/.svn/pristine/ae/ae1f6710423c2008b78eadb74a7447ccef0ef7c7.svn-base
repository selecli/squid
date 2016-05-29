file ./count.out
set $c=0

b main.c:951
commands
silent
p "This!\n"
c
#awatch *(LinkedListNode*)0x804d2a8
end

def df
ab 804d2a8
end

def ab
p *(HotServer*)((LinkedListNode*)0x$arg0)->data
p ((HotServer*)((LinkedListNode*)0x$arg0)->data)->name
end

b main.c:721
commands
silent
printf "destroy server: %s\taddr: %x\n", server->name, p
c
end

b main.c:741
commands
silent
printf "destroy url: %s\taddr: %x\n", url->url, p
c
end

b SendSingleHot
commands
silent
set $c+=1
print $c
c
end

b 955
commands
silent
printf "amount=%d\n",amount
c
end

b main.c:826
commands
silent
printf "url=%s\taddr=%x\n", url->url, url
printf "server=%s\taddr=%x\n", ((HotServer*)((LinkedListNode*)start)->data)->name, start
c
end


r
