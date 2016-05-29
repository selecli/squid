request----发一个http请求，保存请求的结果

共有三种命令行参数：
1、-h，显示命令行参数规则
2、命令行输入参数。可以输入的参数为：
	-d display header，是否显示响应头记录。如果为1，打印到标准输出，默认不打印；2打印所有的头
	-w filename，相应保存的文件名，默认为资源的文件名；
	-l length，限制响应的长度；如果没有，为不限制长度，如果为0，不输出响应；
	--length length，同上；
	-t timer，一个请求最大时间长度；
	--timer timer，同上；
	--ip ip，服务器ip地址，如果没有，从主机名得到；
	--port port，服务器端口，默认为80；
	--type request，请求类型，默认为get；
	--host hostname，主机名。如果没有，从url里取；如果也没有url，程序失败；
	--file filename，文件名。如果没有，从url里取；如果也没有url，默认为/；
	--header Header，头的内容，可以没有。如果没有，默认为Connection:close
			（否则会等待一些秒才返回）；
	--content post content，post请求的内容；
	--url url，如果没有主机名，从这里取，如果没有文件名，从这里取；
			如果为stdin，表示从标准输入读请求，为下一种模式；
	-i argv，监控一个请求各个阶段的时间。参数：dns--如果没有ip、取ip的时间，
			connect--tcp三次握手的时间，send--发请求的时间，
			receive_first--收到第一个字节的时间，receive--接收到所有内容的时间。
			all--显示所有的时间。
			默认监控总的时间。
	--inspect argv，同上。

3、从标准输入读请求。
	--url stdin，表示是这种模式，必须。
	-d display header，是否显示响应头记录。如果为1，打印到标准输出，默认不打印；2打印所有的头
	-w filename，相应保存的文件名，默认为资源的文件名；
	-l length，限制响应的长度；如果没有，为不限制长度，如果为0，不输出响应；
	--length length，同上；
	-t timer，一个请求最大时间长度；
	--timer timer，同上；
	-i argv，监控一个请求各个阶段的时间。参数：dns--如果没有ip、取ip的时间，
			connect--tcp三次握手的时间，send--发请求的时间，
			receive_first--收到第一个字节的时间，receive--接收到所有内容的时间。
			all--显示所有的时间。
			默认监控总的时间。
	--inspect argv，同上。

程序对应每一个请求，至少有一行输出，如果没有-i选项，只输出一行：
	成功并且要求打印头----输出这个请求用的时间+空格+头。
	其它----输出时间。
	其中时间单位为秒，保留小数点后六位。
如果有-i选项，如果成功的话，每一个选项对应一行输出。
	dns----DNS Lookiup:+空格+时间
	connect----Initial Connection Time:+空格+时间
	send----Send Request:+空格+时间
	receive_first----1st Byte:+空格+时间
	receive----Content Download:+空格+时间
	最后输出默认输出。


10月25日的修改，标准输入增加一项--读取文件的长度，放在header前面。


标准输入的格式
ip port type filename host length[ header content]
ip----要侦测的ip，如果为0，由dns决定
port----端口
type----请求类型（不取分大小写，目前只支持get、post、head）
filename----要请求的文件名
host----请求的主机
length----请求的文件长度（只取这么长，如果为-1，取默认，就是命令行那个参数）
header----请求的头
content----post的内容
例如：
#ip				port	type	filename	host				length	header
59.151.18.18	80		GET		/			www.love21cn.com	500		Connection:close
