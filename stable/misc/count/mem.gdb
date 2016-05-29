shell rm gdb.txt -f
set logging on
set $a = 0
set $b = 0

b main.c:192
commands
set $a += 1
shell echo -e "malloc "
p UrlPrint(url)
c
end

b main.c:115
commands
set $b += 1
shell echo -e "free   "
p UrlPrint(url)
c
end

b main.c:598
commands
p HashReport( hash )
p HashPrint( hash )
c
end
r
p $a
p $b
p $a - $b
