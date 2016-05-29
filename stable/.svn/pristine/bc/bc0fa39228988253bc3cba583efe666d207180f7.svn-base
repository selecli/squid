#!/usr/bin/python
# encoding: UTF-8
# Reference: http://sis.ssr.chinacache.com/rms/view.php?id=3832#bugnotes 
# writtern by GSP/chenqi @ Nov. 21, 2012

import sys,os,re
domain = sys.argv[1]; 
p = []

# store_url rewrite rules
# if new rule adds, put it into list p

#rule 1
p1 = re.compile("^http://"+domain+"/jifen_setup/2345_[0-9a-zA-Z]+_setup.exe$")
p1_result = "http://"+domain+"/jifen_setup/2345setup.exe"
p.append((p1,p1_result))

#rule 2
p2 = re.compile("^http://"+domain+"/jifen_setup/2345pack_[0-9a-zA-Z]+_(.*).exe$")
p2_result = "http://"+domain+"/jifen_setup/2345pack.exe"
p.append((p2,p2_result))

#rele 3
p3 = re.compile("^http://"+domain+"/jifen_setup/2345pack2_[0-9a-zA-Z]+_(.*).exe$")
p3_result = "http://"+domain+"/jifen_setup/2345pack2.exe"
p.append((p3,p3_result))

#rule 4
p4 = re.compile("^http://"+domain+"/jifen_haozip/2345_[a-zA-Z0-9]+_haozip.exe$")
p4_result = "http://"+domain+"/jifen_haozip/2345haozip.exe"
p.append((p4,p4_result))

#rule 5
p5 = re.compile("^http://"+domain+"/jifen_browser/2345_[a-zA-Z0-9]+_browser.exe$")
p5_result = "http://"+domain+"/jifen_browser/2345browser.exe"
p.append((p5,p5_result))

#rule 6
p6 = re.compile("^http://"+domain+"/jifen_desk/2345_[a-zA-Z0-9]+_desk.exe$")
p6_result = "http://"+domain+"/jifen_desk/2345desk.exe"
p.append((p6,p6_result))

#rule 7
p7 = re.compile("^http://"+domain+"/jifen_movie/2345_[a-zA-Z0-9]+_movie.exe$")
p7_result = "http://"+domain+"/jifen_movie/2345movie.exe"
p.append((p7,p7_result))

#rule 8
p8 = re.compile("^http://"+domain+"/jifen_lanmon/2345_[a-zA-Z0-9]+_setup.exe$")
p8_result = "http://"+domain+"/jifen_lanmon/2345setup.exe"
p.append((p8,p8_result))

#rule 9
p9 = re.compile("^http://"+domain+"/jifen_lanmon/2345pack_[a-zA-Z0-9]+_(.*).exe$")
p9_result = "http://"+domain+"/jifen_lanmon/2345pack.exe"
p.append((p9,p9_result))

#rule 10
p10 = re.compile("^http://"+domain+"/jifen_pic/2345pic_[a-zA-Z0-9]+_(.*).exe$")
p10_result = "http://"+domain+"/jifen_pic/2345pic.exe"
p.append((p10,p10_result))

#rule 11
p11 = re.compile("^http://"+domain+"/jifen_qq/qqpcmgr_[a-zA-Z0-9]+_(.*).exe$")
p11_result = "http://"+domain+"/jifen_qq/qqpcmgr.exe"
p.append((p11,p11_result))

#main loop
def main():
	while True:
		try:
			a_line = sys.stdin.readline()
		except EOFError:
			exit(-1)

		num = None
		if re.search("^\d+ http://"+domain+"/.*", a_line):
			num = a_line.split(" ")[0]
			line = a_line.split(" ")[1]
		else:
			sys.stdout.write(a_line)
			continue

		#rule 0
		p0 = re.compile("^http://"+domain+"/unionpic/2345pic_lm_[0-9]+_(.*).exe$")
		m0 = p0.match(line)
		if m0:
			ret = num + " " + "http://"+domain+"/unionpic/2345pic_lm_uid_" + m0.group(1) + ".exe" + "\n"
			sys.stdout.write(ret)
			sys.stdout.flush()
			continue

		#rule 1-11
		for item in p:
			if item[0].match(line):
				ret = num + " " + item[1] + "\n"
				sys.stdout.write(ret)
				break
		else:
			sys.stdout.write(a_line)

		sys.stdout.flush()


if __name__ == '__main__':
	main()
	exit(0)
