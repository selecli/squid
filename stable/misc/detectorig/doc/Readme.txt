

程序log文件为/var/log/chinacache/detectorig.log
每一次程序启动写log：---------------------------------------------和进程开始时间
如果配置文件有错误，程序把错误行和原因写入log，然后退出。（全检查一遍再退出）

程序分为三部分
第一部分，读配置文件
配置文件格式：host、info、detect、length、good_ip、modify、backup、method、code、ip。中间空格或者\t分隔
host（域名）----对应anyhost文件的主机名
info----这条记录写到哪里。如果info为N，这条记录属于anyhost文件；否则这条记录属于anyhost.info文件。
detect（是否进行侦测）----yes为侦测，否则认为这个域名的ip都是可靠的，不需要侦测了。
			程序用第一个字符是否为y或者Y来判断
length----进行探测时取源站内容的最大长度。默认值500
good_ip----最后选用几个好的ip，默认值为2
modify（是否根据侦测结果进行修改）----yes为根据侦测结果进行修改，否则只进行侦测，侦测结果写在log文件里。
			程序用第一个字符是否为y或者Y来判断
backup（失败处理）----比较复杂，详见流程图
method（请求方法和请求url）----例子：get:www.wsg.com/wsg.jpg，请求字段是必须的，只允许get和head，
			其余的字段都可选。如果只写yes或者no，默认为get。
			请求和其他部分的分割可以用:或|，如果没有主机名，默认为第一个字段（host）。
			如果没有文件名，默认为:当前时间（秒格式）.jpg。
code（成功代码）----可以有多个，用|分隔（程序里没有这个限制）。
			例如2**|404（*通配，要求第一个字符必须为数字）。
			响应的代码如果与其中一个匹配，认为成功。
ip----要检测的ip，可以有多个，|分隔（也可以是:）。如果ip为0，直接认为这个ip是不通的（优先级高于detect和modify）。
如果一行的第一个字符为#，认为是注释，程序忽略它。空行也忽略。
配置文件不允许两行的host项相同。
配置文件的一个例子
#hostname			info	detect	length	modify	backup		method				code	ip
*					y		no		500		y		1.0.0.1		GET:hostname		200		0
*.cn.				y		no		500		y		no			GET:hostname		200		1.0.0.1|2.0.0.1
*.cw.				y		no		500		no		wild.		GET:hostname		200		1.0.0.1
*.com.				y		yes		500		y		1.2.3.4		GET:hostname		200		0.0.0.1
#www.sohu.com.		y		yes		500		yes		false		GET:hostname/ll.jpg	2**|400	61.135.150.114

第二部分进行侦测
程序按照命令行参数启动几个子进程，子进程的名字是request，负责进行源站探测。如果程序正常结束，是可以杀掉子进程的。
然后程序给子进程分配探测任务（一个任务一行，通过管道传递），如果发现子进程都忙，就等待一个空闲并处理这个子进程的返回结果（子进程处理完一个任务返回一行）。
分配完任务后，等待所有的子进程都空闲，并处理返回结果。


第三部分写anyhost文件
规则见流程图，分为三部分
1、扫描anyhost文件
anyhost文件例子如下
;------
test
;------
www.wsg2.com.                   IN      A       192.168.1.1 ; response_time:0.022331 response_code:200
;?www.wsg2.com.                   IN      A       192.168.1.1 ; response_time:0.022331 response_code:403
www.wsg.com.                    IN      A       192.168.1.1 ; no_detect
;www.wsg.com.                   IN      A       192.168.1.1 ; no_detect
www.wsg3.com.                    IN      A       192.168.1.1 ; ip_down response_time:3.000001
;@@@@@@@@@@@@@@
其中;------之间的表示必须有的内容，如果不是主机行，保留；如果是，但是配置文件没有，或者有但是不修改，保留；否则删掉；
;@@@@@@@@@@@@@@之后的内容为侦测不通的情况，只是提示。如果backup是ip且不通记&，如果有不通的ip记#，如果都不通，记##。
例如：
;&#     www.wsg3.com.   123.45.7.89     ip_down
表示backup是ip且不通，并ip有通的，也有不通的。这条记录是ip
;&#     www.wsg3.com.   123.45.7.89     ip_down backup
表示这条记录是backup

中间的内容为侦测情况
www.wsg2.com.                   IN      A       192.168.1.1 ; response_time:0.022331 response_code:200
表示这个ip是通的，后面为响应时间和相应代码
;?www.wsg2.com.                   IN      A       192.168.1.1 ; response_time:0.022331 response_code:403
表示这个ip探测失败，应该是返回代码不符合要求
www.wsg.com.                    IN      A       192.168.1.1 ; no_detect
表示这个ip没有经过探测
;www.wsg.com.                   IN      A       192.168.1.1 ; no_detect
表示这个ip应该是好的（因为不需要探测），但是数量超过要求写的了
www.wsg3.com.                    IN      A       192.168.1.1 ; ip_down response_time:3.000001
表示这个ip侦测失败了，但是因为某种原因（配置文件的要求），还是记录它了


2、写侦测结果
这个过程比较复杂
首先比较info，看这个主机是要写anyhost文件还是anyhost.info文件
然后判断ip是否为0，为0的优先级最高
	如果为0，判断backup是否为ip，如果是ip，写这个ip，否则判断是否为yes或no，如果不是，写CNAME；否则扔掉；
如果ip不为0，然后判断是否需要修改（配置文件的modify），
	如果不需要，把侦测结果写到log；
如果需要修改，然后判断是否需要侦测（配置文件的detect）
	如果不需要，写要求的ip（good_ip个），其余的写成注释。标记为no_detect
如果需要侦测，把侦测结果进行排序
然后判断侦测结果。
	如果侦测结果有好的ip，写good_ip个好的ip，其余好的写注释，坏的也写注释（坏的前面加一个?）
如果没有好的ip，然后判断backup是否是ip，
	如果是ip，判断是否是好的
		如果是好的，写这个ip，侦测坏的ip写注释
		如果是坏的，看侦测ip是否有坏的
			如果有坏的，选一个最好的，其它的写注释；
			如果没有坏的，判断backup是否是坏的
				如果是坏的，写这个backup的ip
				否则写所有的侦测ip
如果backup不是ip
	如果backup是yes，坏的ip写成注释
	如果backup是no，
		如果有坏的ip，写一个，其它的写注释
		否则写所有的ip
如果backup是字符串，写CNAME记录，坏的ip写注释


3、写侦测错误
写;@@@@@@@@@@@@@@标记
然后写各个侦测失败的情况

