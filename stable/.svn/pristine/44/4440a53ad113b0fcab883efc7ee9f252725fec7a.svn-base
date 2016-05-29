#!/usr/bin/python
# encoding: UTF-8
#filename: sina.py
#sina mp3 antihijack
#VER1.2 0629
import hashlib
import hmac
import sys
import re
import time
import os 
import os.path
import logging
import urllib


class http_object:
	url = ''    
	client_ip = ''
	method = ''
	myip   = ''
	myport = ''
	headerHash = {};

class dealwith_Http:
	def __init__(self) :
		self.__httpobject = http_object()
	def printHttp(self):
		print self.__httpobject.headerHash
	def get_headerHash(self):
		return self.__httpobject.headerHash
	def get_http_object(self):
		return self.__httpobject
	def get_sort_AmzHeaders(self):
		dst= self.__httpobject
		header_value = ''	
		header_key   = ''
		header_hash=[]
		for item in dst.headerHash.items():
			header_key = item[0]	
			#print header_key
		  	if header_key.startswith("x-amz-"):
				header_hash.append(header_key)
			elif  header_key.startswith("x-sina-"):
				header_hash.append(header_key)
#				print header_key

		header_hash.sort()
		return header_hash

	def parsehttpHeader(self,parseStr):
		#parseStr = '''http://www.mydomain.com.sinastorage.com/abc/def.jpg?ssig=G6sBgBW1U5&Expires=1175139620&KID=sina,produc 192.168.1.1/- - GET - myip=192.168.100.135 myport=80 User-Agent:"curl/7.19.7 (i686-pc-linux-gnu) libcurl/7.19.7 OpenSSL/0.9.7a zlib/1.2.1.2 libidn/0.5.6" Host:"www.mydomain.com.sinastorage.com"  Accept:"*/*"  Proxy-Connection:"Keep-Alive" Content-Length: "11" Content-MD5:"XrY7u+Ae7tCTyyK7j1rNww==" Content-Type:"text/plaindddd" Content-Disposition:"attachment; filename=hello.txt" Content-Encoding:"utf-8" X-Sina-Info:"%E4%B8%AD%E6%96%87" Date:" Tue, 10 Aug 2010 16:08:08 +0000" Authorization:" SINA product:oKmcJOhDzR" Cookie:"netdiskcookie="ssig=hf/2arnW6y&Expires=1175139620&KID=sina,product""'''

		#print parseStr
		temList = parseStr.split()
		i = 0;
		dst = self.__httpobject
		header_value = ''
		header_key   = ''
		begin_header = 0
		for key in temList:
			if i == 0:
				dst.url = key
#				print key
			if i == 1: 
				pattern = re.compile(r'^(\d+)\.(\d+)\.(\d+)\.(\d+)')
				match = pattern.match(key)
				if match:
					dst.client_ip= match.group()
#					print dst.client_ip				
				else:
					dst.client_ip = key
#					print dst.client_ip				
			if i == 3:
				dst.method = key
#				print key
			if i == 5:
				dst.myip = key
#				print key
			if i == 6:
			    dst.myport = key
#				print key
			i = i + 1
			
			if key.startswith("myport="):
				begin_header = 1;
				continue
			if begin_header != 1:
				continue

			if key.endswith(":"):
				if len(header_key) > 1:
					dst.headerHash[header_key] = header_value
					header_key = ''
					header_value=''

				header_key = str(key)[0:len(key)-1]
				lower_header_key = header_key.lower();
				if lower_header_key.startswith("x-amz-"):
					header_key = lower_header_key
				elif  lower_header_key.startswith("x-sina-"):
					header_key = lower_header_key
			else:
				if len(header_value) == 0:
					header_value = key
				else:
					header_value = header_value + ' ' + key 
#		print dst.headerHash	 		
	#	print temList;
#stringtosign = 'GET\n\n\n1175136020\n/yourproject/abc/def.jpg?ip=123.123.123.123'
#password = 'uV3F3YluFJax1cknvbcGwgjvx4QpvB+leU8dUj2o'
#ssig = hmac.new( password, stringtosign, hashlib.sha1 ).digest().encode('base64')[5:15]
#print ssig
#print hashlib.new("md5", "JGood is a handsome boy").hexdigest() 

#for line in sys.stdin.readline():
#	print line
def initlog():
	logfile = "/var/log/chinacache/redirect.log"
	logger = logging.getLogger()
	hdlr = logging.FileHandler(logfile)
	formatter = logging.Formatter('%(asctime)s %(levelname)s %(message)s')
	hdlr.setFormatter(formatter)
	logger.addHandler(hdlr)
	logger.setLevel(logging.NOTSET)
	return logger

class main_Http:
	def __init__(self,httpobj,dealwithhttp) :
		self.__httpobject = httpobj
		self.__dealwithhttp = dealwithhttp

	def getContentMd5(self,a):
	#s-sina-sha1  > Content-SHA1 > s-sina-md5 > Content-MD5
#		a['Content-MD5']= '2aae6c35c94fcfb415dbe95f408b9ce91ee846ed'
		if(self.__httpobject.headerHash.has_key('s-sina-sha1')):
			a['Content-Md5']= self.__httpobject.headerHash['s-sina-sha1']
		elif(self.__httpobject.headerHash.has_key('Content-SHA1')):
			a['Content-Md5']= self.__httpobject.headerHash['Content-SHA1']
		elif(self.__httpobject.headerHash.has_key('s-sina-md5')):
			a['Content-Md5']= self.__httpobject.headerHash['s-sina-md5']
		elif(self.__httpobject.headerHash.has_key('Content-MD5')):
			a['Content-Md5']= self.__httpobject.headerHash['Content-MD5']
		else:
			a['Content-Md5']= ""
		
	def getContentType(self,a):
		if(self.__httpobject.headerHash.has_key('Content-Type')):
			a['Content-Type']= self.__httpobject.headerHash['Content-Type']
	#	a['Content-Type']= self.__httpobject.headerHash['Content-Type'] 

	def getValueForSpecialFromSTR(self,str,key):
		#self.initlog().info(str+"\t"+key)
		value=''
		val_start = str.find(key) #Cookie  > URL
		if val_start != -1:
#		self.initlog().info("val_start"+str(val_start))
			tmp = str[val_start+len(key):]
			val_end = tmp.find("&") #Cookie  > URL
			if val_end != -1:
				value = tmp[0:val_end]
			else:
				value = tmp[0:]
		else:
			value = '0'
		return value
		
	def getExpires(self):
		if(self.__httpobject.headerHash.has_key('Cookie')):
			cookie_value = self.__httpobject.headerHash['Cookie']
			expires = self.getValueForSpecialFromSTR(cookie_value,'Expires=')		
			if expires == '0':
				expires = self.getValueForSpecialFromSTR(self.__httpobject.url,'Expires=') 

		else:
			expires = self.getValueForSpecialFromSTR(self.__httpobject.url,'Expires=') 

		return expires

	def getSSIG(self):
		if(self.__httpobject.headerHash.has_key('Cookie')):
			cookie_value = self.__httpobject.headerHash['Cookie']
			ssig = self.getValueForSpecialFromSTR(cookie_value,'ssig=')		
			if ssig == '0':
				ssig  = self.getValueForSpecialFromSTR(self.__httpobject.url,'ssig=') 
		else:
			ssig  = self.getValueForSpecialFromSTR(self.__httpobject.url,'ssig=') 
		return ssig

	def getLIMIT(self):
		limit  = self.getValueForSpecialFromSTR(self.__httpobject.url,'ip=') 
		return limit

		
	def getDate(self,a):
		expires =  self.getExpires()
		if(expires != ""):
			a['Date'] = expires  
		elif(self.__httpobject.headerHash.has_key('Date')):
			a['Date'] = self.__httpobject.headerHash['Date']
		else:
			a['Date'] = ""
	#	del(a['Date'])
	# Cookie expires > URL expires > PUT Headers

	def getCanonicalizedAmzHeaders(self,a):
	#"\n".join( allheaders which startswith "x-amz-" or "x-sina-" ) + '\n'
	# header lower ,use utf-8 and remove blank
	#	del(a['CanonicalizedAmzHeaders'])
#		a['CanonicalizedAmzHeaders'] = 'x-amz-meta-tag:test\nx-sina-info:\xe4\xb8\xad\xe6\x96\x87\n' 
		#print self.__dealwithhttp.get_sort_AmzHeaders() 
		for header in self.__dealwithhttp.get_sort_AmzHeaders():
			a['CanonicalizedAmzHeaders'] += header + ":"+ self.__httpobject.headerHash[header] 

	def get_sub_host(self,host,father_domain):
		for domain in father_domain:
			if host.find(domain) != -1:
				host = host[0:host.find(domain)-1]
		return host

	def get_uri_and_subresource(self,url):
		res_calu = ""
		count_res = 0
		if(self.__httpobject.headerHash.has_key('Host')):
			uri = url[url.find(self.__httpobject.headerHash['Host'])+len(self.__httpobject.headerHash['Host']):]
		else:
			host_start = url[url.find("://")+3:]
			uri = host_start[host_start.find("/")+1:]
		
		if uri.find('?')!= -1:
			resource = uri[uri.find('?')+1:]
			uri_pure = uri[0:uri.find('?')] 
			res_list = resource.split( '&')
			ignore_part = ['Expires=','ssig=','KID=']
			for item in res_list:
				if item.find(ignore_part[0])==-1 and item.find(ignore_part[1])==-1 and item.find(ignore_part[2]):
					if count_res == 0:
						res_calu = '?'+item
						count_res =count_res + 1
					else:
						res_calu += '&'+ item
			return uri_pure + res_calu
	def getCanonicalizedResource(self,a):
	#	del[a['CanonicalizedResource']]
	# [ "/" + Project ] +<HTTP-Request-URI, from the protocol name up to the query string> +[ sub-resource, if present. For example "?acl", "?location", "?logging", "?relax", "?meta", "?torrent" or "?uploadID=...", "?ip=..." ];
	#	host.[yourproject_name]+uri+sub_resource
	#	a['CanonicalizedResource'] = '/yourproject/abc/def.jpg?relax'
		father_domain = ['sinastorage.com','s3.sinaapp.com','s3.amazonaws.com']
		host2 =''
		if(self.__httpobject.headerHash.has_key('Host')):
			host = self.__httpobject.headerHash['Host']
			host2 = self.get_sub_host(host,father_domain)
		uri_subresource = self.get_uri_and_subresource(self.__httpobject.url)
		a['CanonicalizedResource'] = "/"+host2+uri_subresource

	def makeNewHeaders(self,a):
		self.getContentMd5(a)
		self.getContentType(a)
		self.getDate(a)
		self.getCanonicalizedAmzHeaders(a) 
		self.getCanonicalizedResource(a)
	#	for name, address in a.items():
	#		print '[%s ]=>[ %s]' %(name, address)
	
	def check_ssig(self,a,b):
		return cmp(a,b)
	
	def check_ip(self,a,b):
		return cmp(a,b)

	def Isquerystring(self,ipaddr):
		nums   = ipaddr.split( '.')
		if len(nums)!=4:
			return -1
		elif  nums[3] == "":
			return 0
		else:
			return 1

	def check_querystring(self,ipaddr,ip_seg):
		nums_1   = ipaddr.split( '.')
		nums_2   = ip_seg.split( '.')
		if len(nums_1)!=4 or len(nums_2)!=4:
			return -1
		elif (nums_1[0] == nums_2[0])and(nums_1[1] == nums_2[1])and((nums_1[2] == nums_2[2])):
			return 1
		else:
			return 0
	
def main(): 
	logger = initlog()

	while True:
		line = sys.stdin.readline()
#		line = '''http://www.mydomain.com.sinastorage.com/abc/def.jpg?ip=1175135000,123.123.123.123&ssig=G6sBgBW1U5&Expires=1175139620&KID=sina,produc&relax 123.123.123.123/- - GET -myip=192.168.100.135 myport=80 User-Agent: curl/7.19.7 (i686-pc-linux-gnu) libcurl/7.19.7 OpenSSL/0.9.7a zlib/1.2.1.2 libidn/0.5.6  x-amz-meta-Tag: test Host: www.mydomain.com.sinastorage.com  Accept: */*  Proxy-Connection: Keep-Alive Content-Length: 11 Content-MD5: XrY7u+Ae7tCTyyK7j1rNww== Content-Type: text/plainddxx Content-Disposition: attachment; filename=hello.txt Content-Encoding: utf-8 X-Sina-Info: %E4%B8%AD%E6%96%87 Date: Tue, 10 Aug 2010 16:08:08 +0000 Authorization: SINA product:oKmcJOhDzR Cookie: netdiskcookie="ssig=gx3NWB8VnQ&Expires=1175139621&KID=sina,product";'''
#		line = '''http://s3.music.sina.com.cn/1001014.mp3?KID=sina,music&ssig=gQB5GjmVFo&Expires=1307010921&ip=1307009301,218.30.126.204 1.1.1.1/- - GET -myip=192.168.100.135 myport=80 User-Agent: curl/7.19.7  Host: s3.music.sina.com.cn'''
		
		t = dealwith_Http()
		t.parsehttpHeader(line + " END:")
	#	t.printHttp()
		httpobj = t.get_http_object()
		mainhttp = main_Http(httpobj,t);
	#	print httpobj.url
	#	httphash = t.get_headerHash()
	#	print httphash
	#	headerList = t.get_sort_AmzHeaders()
	#	print headerList	

#		logger = mainhttp.initlog()
		logger.info(line)

		strlist = line.split(' ')
#		url = strlist[0]
#		ip = strlist[1]
		url = httpobj.url
		ip = httpobj.client_ip
		method = httpobj.method
#		logger.info("url:["+url+"]\tip["+ip+"]\tmethod["+mehtod+"]")
#		print method

#		expires_start = url.find("Expires=") #Cookie  > URL
#		print expires_start
#		expires = url[expires_start+8:-1]
#		print expires
#		expires_end = expires.find("&") #Cookie  > URL
#		print expires_end
#		if(expires_end != 0):
#			expires =  expires[0:expires_end]
		logger.info(mainhttp.getExpires())
		expires = int(mainhttp.getExpires())
#		print expires

#		SSIG_start = url.find("ssig=") #Cookie  > URL
#		print SSIG_start
#		SSIG = url[SSIG_start+5:-1]
#		print SSIG 
#		SSIG_end = SSIG.find("&") #Cookie  > URL
#		print SSIG_end 
#		if(SSIG_end != 0):
#			SSIG = SSIG[0:SSIG_end]
		SSIG = mainhttp.getSSIG()
		
		limit_ip_start = url.find("ip=") #Cookie  > URL
		ip_verify_type = "IP_NONE"
		querystring = ''
		start_time = 0
		limit_ip_str = ""
		limit_ip_str = mainhttp.getLIMIT()
		logger.info("limit_ip_time:\t"+limit_ip_str)
		if len(limit_ip_str) != 0 and limit_ip_str != '0':	
			ip_list = limit_ip_str.split(',')
			if(len(ip_list)!=2):
				logger.error("FAIL403: ?ip section format wrong\n"+line)
				print "403:"+line
				sys.stdout.flush()
				return

			querystring = ip_list[1]  #make sure ip_list[1] exsit
			start_time = int(ip_list[0])
			result = mainhttp.Isquerystring(querystring)
			if(result == 0):
				ip_verify_type = "IP_SEG"
			elif(result == 1):
				ip_verify_type = "IP_SINGLE"
			else:
				ip_verify_type = "IP_NONE"


		headerToSign  = {		'HTTP-Verb' : 'GET',
				'Content-MD5' : '',
				'Content-Type' : '',
				'Date' : '',
				'CanonicalizedAmzHeaders':'',
				'CanonicalizedResource':'/www.mydomain.com/abc/def.jpg'
		}
		mainhttp.makeNewHeaders(headerToSign)

		StringToSign = headerToSign['HTTP-Verb'] + "\n" +\
		               headerToSign['Content-MD5'] + "\n" +\
					   headerToSign['Content-Type'] + "\n" +\
					   headerToSign['Date'] + "\n" +\
					   headerToSign['CanonicalizedAmzHeaders'] +\
					   headerToSign['CanonicalizedResource']
#		print StringToSign
		logger.info("STRING_TO_SIGN:"+StringToSign)
#		password = 'uV3F3YluFJax1cknvbcGwgjvx4QpvB+leU8dUj2o'
		password = "ij42IR2qwSuwc6qRE1x80JX+1DzyexLYk3NrBnRM"
		ssig1 = hmac.new( password, StringToSign, hashlib.sha1 ).digest().encode('base64')[5:15]
		ssig = urllib.quote(ssig1)
		ssig = ssig.replace('/','%2F')
		ssig = ssig.replace('+','%2B')

		now = int(time.time())

		if now < expires:
			logger.info("expire  caculate:["+str(expires)+"]\t>\t"+"now:["+str(now)+"]")
			if mainhttp.check_ssig(SSIG,ssig) == 0:
				logger.info("ssig caculate:["+ssig+"]\t==\t"+"SSIG:["+SSIG+"]")
				if(ip_verify_type == "IP_SINGLE"):
					if  now > start_time and mainhttp.check_ip(ip,querystring) != 0:
						logger.info("IP_SINGLE403\t"+"ip["+ip+"]\tquerystring["+querystring+"]"+"now["+str(now) +"]\tstart_time["+ str(start_time)+"]")
						print "403:"+line
						sys.stdout.flush()
					else:
						logger.info("IP_SINGLE200\t"+"ip["+ip+"]\tquerystring["+querystring+"]"+"now["+str(now) +"]\tstart_time["+ str(start_time)+"]")
						print line
						sys.stdout.flush()

				elif(ip_verify_type == "IP_SEG"):
					result = mainhttp.check_querystring(ip,querystring)
					if now > start_time and result <= 0:
						logger.info("IP_SEG403\t"+"ip["+ip+"]\tquerystring["+querystring+"]"+"now["+str(now) +"]\tstart_time["+ str(start_time)+"]")
						print "403:"+line
						sys.stdout.flush()
					else:
						logger.info("IP_SEG200\t"+"ip["+ip+"]\tquerystring["+querystring+"]"+"now["+str(now) +"]\tstart_time["+ str(start_time)+"]")
						print line
						sys.stdout.flush()
				else:
					logger.info("IP_NONE\t"+"ip["+ip+"]\tquerystring["+querystring+"]")
					print line
					sys.stdout.flush()
					
			else:	
				logger.info("ssig caculate:["+ssig+"]\t!=\t"+"SSIG:["+SSIG+"]")
				print "403:"+line
				sys.stdout.flush()
		else:
			logger.info("expire  caculate:["+str(expires)+"]\t<\t"+"now:["+str(now)+"]")
			print "403:"+line
			sys.stdout.flush()


if __name__=='__main__':
	main()
