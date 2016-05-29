file ./count.out
#b hash.c:134
#commands
#silent
#printf "%s\n", ((Url*)element)->store_url
#c
#end

#b main.c:691
#commands
#silent
#printf "%s %d\n", url_in_hash->store_url, url_in_hash->count
#c
#end

b main.c:702
commands
silent
printf "%s %d\n", url_in_hash->store_url, url_in_hash->count
c
end


r
