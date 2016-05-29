file ./count.out
#set print pretty on

#b main.c:776 if !strcmp(server_name, "www.06.com" )
#commands
#disable 1-99
#end

b main.c:759
commands
silent
printf "new: %s %x\n", server_name, start
c
end


b 768
commands
silent
printf "p %s %x\n", name, p
c
end

b main.c:770
commands
silent
printf "existed!\n"
c
end

b main.c:778
commands
silent
printf "create server name\n"
c
end

b main.c:803
commands
silent
printf "create url\n"
c
end

b linked_list.c:140
commands
silent
printf "cycle insert\n"
c
end

b main.c:846
commands
silent
printf "destroy\n"
end

b main.c:829
commands
silent
printf "init completed\n"
end


